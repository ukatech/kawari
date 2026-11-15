//---------------------------------------------------------------------------
//
// 本体(SSP, ninixなど)との通信 (Direct SSTP)
//
//  2026.10.18  Phase 9.0.0   新設
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/dsstp.h"
#include "misc/charset.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
#if defined(KAWARI_WIN32)
#	include <windows.h>
#	define DSSTP_WIN32
#	if defined(_MSC_VER) && (_MSC_VER < 1300)
		// VC6付属の古いSDKには無い。32bit専用なので32bit整数と同じ扱いにする
		typedef DWORD DWORD_PTR;
		typedef DWORD ULONG_PTR;
		typedef long LONG_PTR;
#		define GWLP_USERDATA GWL_USERDATA
#		define GetWindowLongPtrW GetWindowLongW
#		define SetWindowLongPtrW SetWindowLongW
#	endif
#elif defined(KAWARI_POSIX) && !defined(__EMSCRIPTEN__)
#	include <climits>
#	include <cstdint>
#	include <cstring>
#	include <cstdlib>
#	include <fcntl.h>
#	include <semaphore.h>
#	include <sys/mman.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <sys/time.h>
#	include <sys/types.h>
#	include <sys/un.h>
#	include <unistd.h>
#	include <sstream>
#	define DSSTP_NINIX
#endif
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
namespace dsstp {
//---------------------------------------------------------------------------
// NOTIFY hwnd の Reference0 から設定する
bool TBasewareLink::SetHandleFromReference(const wstring &ref)
{
	// "123" または "123\x01456" (バイト値1区切りで各スコープ)。先頭のスコープを使う
	wchar_t *end=NULL;
	unsigned long h=wcstoul(ref.c_str(), &end, 10);
	if ((end==ref.c_str())||(h==0)) return false;
	handle=h;
	return true;
}
//---------------------------------------------------------------------------
// ヘッダの値に書けない文字を含むか
bool HasForbiddenChar(const wstring &s)
{
	return (s.find_first_of(wstring(L"\r\n\0", 3))!=wstring::npos);
}
//---------------------------------------------------------------------------
// EXECUTE SSTP の要求を作る
string BuildExecuteRequest(const wstring &command, const vector<wstring> &refs)
{
	wstring request=L"EXECUTE SSTP/1.1\r\nCharset: UTF-8\r\nSender: kawari\r\nCommand: "+command+L"\r\n";
	for (unsigned int i=0; i<refs.size(); i++)
		request+=L"Reference"+IntToString((int)i)+L": "+refs[i]+L"\r\n";
	request+=L"\r\n";
	return WtoUTF8(request);
}
//---------------------------------------------------------------------------
// ヘッダの並び(ステータス行から空行の前の改行まで)から、名前(小文字で末尾に':')の値を探す
// 無ければ空
static string FindHeader(const string &headers, const char *name)
{
	const string key=string("\r\n")+name;
	for (string::size_type pos=0; pos+key.size()<=headers.size(); pos++){
		string::size_type n=0;
		for (; n<key.size(); n++){
			char c=headers[pos+n];
			if ((c>='A')&&(c<='Z')) c=(char)(c-'A'+'a');
			if (c!=key[n]) break;
		}
		if (n==key.size()){
			string::size_type start=pos+key.size();
			string::size_type end=headers.find("\r\n", start);
			if (end==string::npos) end=headers.size();
			while ((start<end)&&((headers[start]==' ')||(headers[start]=='\t'))) start++;
			while ((end>start)&&((headers[end-1]==' ')||(headers[end-1]=='\t'))) end--;
			return headers.substr(start, end-start);
		}
	}
	return string();
}
//---------------------------------------------------------------------------
// EXECUTE SSTP の応答を解析する
int ParseExecuteResponse(const string &response, vector<wstring> &lines)
{
	lines.clear();

	// 末尾に\0が付いてくる
	string bytes=response;
	string::size_type nul=bytes.find('\0');
	if (nul!=string::npos) bytes.erase(nul);

	// ステータス行 : SSTP/1.x 200 OK
	string::size_type status_end=bytes.find("\r\n");
	if (status_end==string::npos) status_end=bytes.size();
	if (bytes.compare(0, 5, "SSTP/")!=0) return 0;
	string::size_type p=bytes.find(' ');
	if ((p==string::npos)||(p>=status_end)) return 0;
	p++;
	int code=0;
	int digits=0;
	while ((p<status_end)&&(bytes[p]>='0')&&(bytes[p]<='9')&&(digits<3)){
		code=code*10+(bytes[p]-'0');
		p++;
		digits++;
	}
	if (digits!=3) return 0;

	// ヘッダ (ステータス行の後から空行まで)
	string::size_type header_end=bytes.find("\r\n\r\n", status_end);
	if (header_end==string::npos) return code;	// 追加データなし
	string headers=bytes.substr(0, header_end+2);

	// Charsetが無ければ、送ったときと同じUTF-8とみなす
	TKawariCharset cs=CharsetFromName(AtoW(FindHeader(headers, "charset:")));
	if (cs==KC_NULL) cs=KC_UTF8;

	string body=bytes.substr(header_end+4);
	string length=FindHeader(headers, "content-length:");
	bool has_length=!length.empty();
	if (has_length){
		string::size_type n=(string::size_type)strtoul(length.c_str(), NULL, 10);
		if (n<body.size()) body.erase(n);
	}

	// 行に分ける。Content-Lengthが無いときは空行で終わる
	wstring data=MBtoW(body, cs);
	wstring::size_type start=0;
	while (start<data.size()){
		wstring::size_type end=data.find(L'\n', start);
		if (end==wstring::npos) end=data.size();
		wstring::size_type len=end-start;
		if ((len>0)&&(data[start+len-1]==L'\r')) len--;
		if ((len==0)&&(!has_length)) break;
		lines.push_back(data.substr(start, len));
		start=end+1;
	}
	// Content-Lengthがあるときも、末尾の空行は追加データに含めない
	while ((!lines.empty())&&(lines.back().empty())) lines.pop_back();
	return code;
}
//---------------------------------------------------------------------------
#if defined(DSSTP_WIN32)
//---------------------------------------------------------------------------
namespace {
	const ULONG_PTR DIRECT_SSTP_PORT=9801;

	// 応答を受け取る窓。応答はWM_COPYDATAでこの窓に返ってくる
	// (GWLP_USERDATAに受け取り先のstringを持たせる)
	LRESULT CALLBACK ReceiverProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (message==WM_COPYDATA){
			const COPYDATASTRUCT *cds=(const COPYDATASTRUCT *)lparam;
			string *response=(string *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if ((response)&&(cds)&&(cds->lpData)&&(cds->dwData==DIRECT_SSTP_PORT)){
				response->assign((const char *)cds->lpData, cds->cbData);
				return TRUE;
			}
			return FALSE;
		}
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}
//---------------------------------------------------------------------------
bool TBasewareLink::Send(const string &request, string &response, unsigned int timeout_ms) const
{
	response.erase();

	// ウィンドウハンドルは32bitで表せる値。64bit版では符号拡張してポインタにする
	HWND target=(HWND)(INT_PTR)(LONG)handle;
	if ((handle==0)||(!::IsWindow(target))) return false;

	const wchar_t *windowname=L"kawari_dsstp_receiver";
	HINSTANCE hInstance=::GetModuleHandleW(NULL);

	WNDCLASSEXW windowClass;
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.cbSize=sizeof(windowClass);
	windowClass.hInstance=hInstance;
	windowClass.lpszClassName=windowname;
	windowClass.lpfnWndProc=ReceiverProc;
	::RegisterClassExW(&windowClass);

	HWND receiver=::CreateWindowW(windowname, windowname, 0, 0, 0, 100, 100, NULL, NULL, hInstance, NULL);
	if (receiver==NULL){
		::UnregisterClassW(windowname, hInstance);
		return false;
	}
	::SetWindowLongPtrW(receiver, GWLP_USERDATA, (LONG_PTR)&response);

	COPYDATASTRUCT cds;
	cds.dwData=DIRECT_SSTP_PORT;
	cds.cbData=(DWORD)request.size();
	cds.lpData=(PVOID)request.data();

	// 相手はWM_COPYDATAの処理中に、こちらの窓へWM_COPYDATAで返信してくる。
	// 待っている間も送られてきたメッセージを処理できるよう、SMTO_BLOCKは付けない
	DWORD_PTR ret=0;
	::SendMessageTimeoutW(target, WM_COPYDATA, (WPARAM)receiver, (LPARAM)&cds,
		SMTO_ABORTIFHUNG, timeout_ms, &ret);

	::DestroyWindow(receiver);
	::UnregisterClassW(windowname, hInstance);

	return !response.empty();
}
//---------------------------------------------------------------------------
#elif defined(DSSTP_NINIX)
//---------------------------------------------------------------------------
namespace {
	// ninix が共有メモリで公開するFMO
	struct shm_t {
		uint32_t size;
		sem_t sem;
		char buf[PATH_MAX];
	};

	const int BUFFER_SIZE=1024;

	// UNIXソケットで要求を送り、応答を受け取る。失敗したら空
	// has_header : 応答の先頭に4バイトの長さが付く (FMOの取得)
	string SendDataUsingUnixSocket(const string &path, const string &request, bool has_header, unsigned int timeout_ms)
	{
		sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		if (path.length()>=sizeof(addr.sun_path)) return "";
		int soc=socket(AF_UNIX, SOCK_STREAM, 0);
		if (soc==-1) return "";
		addr.sun_family=AF_UNIX;
		// null-terminatedも書き込ませる
		strncpy(addr.sun_path, path.c_str(), path.length()+1);

		struct timeval tv;
		tv.tv_sec=timeout_ms/1000;
		tv.tv_usec=(timeout_ms%1000)*1000;
		setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(soc, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr))==-1){
			close(soc);
			return "";
		}
		// 相手が先に閉じても SIGPIPE でホストごと落ちないようにする
#if defined(MSG_NOSIGNAL)
		const int sendflags=MSG_NOSIGNAL;
#else
		const int sendflags=0;
#	if defined(SO_NOSIGPIPE)
		{
			int on=1;
			setsockopt(soc, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
		}
#	endif
#endif
		if (send(soc, request.data(), request.size(), sendflags)!=static_cast<ssize_t>(request.size())){
			close(soc);
			return "";
		}
		shutdown(soc, SHUT_WR);

		char buffer[BUFFER_SIZE];
		string data;
		uint32_t remain=0;
		if (has_header){
			// 4バイトが分かれて届くこともあるので揃うまで読む
			size_t got=0;
			while (got<sizeof(uint32_t)){
				ssize_t r=read(soc, buffer+got, sizeof(uint32_t)-got);
				if (r<=0){
					close(soc);
					return "";
				}
				got+=static_cast<size_t>(r);
			}
			memcpy(&remain, buffer, sizeof(remain));
			if (remain<=0x1000000) data.reserve(remain);	// 相手の申告した長さをそのまま確保しない
		}
		while (true){
			ssize_t ret=read(soc, buffer, BUFFER_SIZE);
			if (ret==-1){
				close(soc);
				return "";
			}
			if (ret==0){
				close(soc);
				break;
			}
			if (!has_header){
				data.append(buffer, ret);
			}else{
				uint32_t n=(remain<static_cast<uint32_t>(ret))?remain:static_cast<uint32_t>(ret);
				data.append(buffer, n);
				remain-=n;
			}
		}
		return data;
	}

	// ninix のFMOから、hwnd が target のゴーストのソケットのパスを探す
	// FMOは「ID.キー\x01値」の行の並び
	bool FindNinixSocket(unsigned long target, string &path, unsigned int timeout_ms)
	{
		int fd=shm_open("/ninix", O_RDWR, 0);
		if (fd==-1) return false;
		shm_t *shm=static_cast<shm_t *>(mmap(NULL, sizeof(shm_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
		close(fd);
		if (shm==MAP_FAILED) return false;
		if (sem_wait(&shm->sem)==-1){
			munmap(shm, sizeof(shm_t));
			return false;
		}
		string sockdir(shm->buf, (shm->size<sizeof(shm->buf))?shm->size:sizeof(shm->buf));
		int post_result=sem_post(&shm->sem);
		munmap(shm, sizeof(shm_t));
		if (post_result==-1) return false;

		string fmo=SendDataUsingUnixSocket(sockdir+"ninix", "GetFMO\r\n", true, timeout_ms);
		istringstream iss(fmo);
		string line;
		while (getline(iss, line)){
			if ((!line.empty())&&(line[line.size()-1]=='\r')) line.erase(line.size()-1);
			string::size_type dot=line.find('.');
			if (dot==string::npos) continue;
			string::size_type sep=line.find('\x01', dot+1);
			if (sep==string::npos) continue;
			if (line.compare(dot+1, sep-dot-1, "hwnd")!=0) continue;
			if (strtoul(line.c_str()+sep+1, NULL, 10)==target){
				path=sockdir+line.substr(0, dot);
				return true;
			}
		}
		return false;
	}
}
//---------------------------------------------------------------------------
bool TBasewareLink::Send(const string &request, string &response, unsigned int timeout_ms) const
{
	response.erase();
	if (handle==0) return false;
	string path;
	if (!FindNinixSocket(handle, path, timeout_ms)) return false;
	response=SendDataUsingUnixSocket(path, request, false, timeout_ms);
	return !response.empty();
}
//---------------------------------------------------------------------------
#else
//---------------------------------------------------------------------------
// 本体と通信できない環境 (js など)
bool TBasewareLink::Send(const string &request, string &response, unsigned int timeout_ms) const
{
	response.erase();
	return false;
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
} // namespace dsstp
//---------------------------------------------------------------------------
