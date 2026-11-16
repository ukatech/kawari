//---------------------------------------------------------------------------
//
// ファイルI/O・ディレクトリ・環境変数 (パスは常にstd::wstring)
//
//  2026.10.18  Phase 9.0.0   新設 (wchar_t化。_dirent.*を置き換え)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/wfile.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
//---------------------------------------------------------------------------
#ifdef KAWARI_WIN32
#	include <windows.h>
#else
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <dirent.h>
#endif
//---------------------------------------------------------------------------
using namespace std;
#include <cwchar>
//---------------------------------------------------------------------------
std::string PathToNative(const std::wstring &path)
{
	return WtoUTF8(path);
}
//---------------------------------------------------------------------------
FILE* kawari_wfopen(const std::wstring &path, const wchar_t *mode)
{
#ifdef KAWARI_WIN32
	return _wfopen(path.c_str(), mode);
#else
	string m=WtoA(mode);
	return fopen(PathToNative(path).c_str(), m.c_str());
#endif
}
//---------------------------------------------------------------------------
bool ReadWholeFile(const std::wstring &path, std::string &out)
{
	out.erase();
	FILE *fp=kawari_wfopen(path, L"rb");
	if(!fp) return false;

	char buf[4096];
	size_t n;
	while((n=fread(buf, 1, sizeof(buf), fp))>0) out.append(buf, n);

	bool ok=!ferror(fp);
	fclose(fp);
	return ok;
}
//---------------------------------------------------------------------------
bool WriteWholeFile(const std::wstring &path, const std::string &data, bool append)
{
	FILE *fp=kawari_wfopen(path, append ? L"ab" : L"wb");
	if(!fp) return false;

	bool ok=true;
	if(!data.empty()) ok=(fwrite(data.data(), 1, data.size(), fp)==data.size());
	if(fclose(fp)!=0) ok=false;
	return ok;
}
//---------------------------------------------------------------------------
bool ReadTextFile(const std::wstring &path, std::wstring &out, TKawariCharset *cs_out, bool *hasbom)
{
	out.erase();
	string raw;
	if(!ReadWholeFile(path, raw)) return false;

	bool bom=HasUTF8BOM(raw);
	TKawariCharset cs=bom ? KC_UTF8 : DetectCharset(raw);
	if(bom) raw.erase(0, 3);

	out=MBtoW(raw, cs);
	if(cs_out) *cs_out=cs;
	if(hasbom) *hasbom=bom;
	return true;
}
//---------------------------------------------------------------------------
bool WriteTextFile(const std::wstring &path, const std::wstring &text, TKawariCharset cs, bool bom, bool append)
{
	string data;
	if(bom&&(cs==KC_UTF8)) data+="\xef\xbb\xbf";
	data+=WtoMB(text, cs);
	return WriteWholeFile(path, data, append);
}
//---------------------------------------------------------------------------
bool ListDirectory(const std::wstring &dir, std::vector<std::wstring> &names)
{
	names.clear();
#ifdef KAWARI_WIN32
	wstring spath=dir;
	if(spath.empty()||((spath[spath.size()-1]!=L'\\')&&(spath[spath.size()-1]!=L'/')))
		spath+=L'\\';
	spath+=L'*';

	WIN32_FIND_DATAW fd;
	HANDLE h=FindFirstFileW(spath.c_str(), &fd);
	if(h==INVALID_HANDLE_VALUE) return false;

	do {
		wstring name=fd.cFileName;
		if((name!=L".")&&(name!=L"..")) names.push_back(name);
	} while(FindNextFileW(h, &fd));
	FindClose(h);
	return true;
#else
	DIR *d=opendir(PathToNative(dir).c_str());
	if(!d) return false;

	struct dirent *ent;
	while((ent=readdir(d))!=NULL) {
		string name=ent->d_name;
		if((name!=".")&&(name!="..")) names.push_back(UTF8toW(name));
	}
	closedir(d);
	return true;
#endif
}
//---------------------------------------------------------------------------
bool PathStat(const std::wstring &path, bool *isdir, bool *isfile)
{
#ifdef KAWARI_WIN32
	DWORD attr=GetFileAttributesW(path.c_str());
	if(attr==0xFFFFFFFF) return false;
	bool d=((attr&FILE_ATTRIBUTE_DIRECTORY)!=0);
	if(isdir) *isdir=d;
	if(isfile) *isfile=!d;
	return true;
#else
	struct stat buf;
	if(stat(PathToNative(path).c_str(), &buf)) return false;
	if(isdir) *isdir=S_ISDIR(buf.st_mode);
	if(isfile) *isfile=S_ISREG(buf.st_mode);
	return true;
#endif
}
//---------------------------------------------------------------------------
bool GetEnvironmentW(const std::wstring &name, std::wstring &value)
{
#ifdef KAWARI_WIN32
	// _wgetenv は CRT の複製を見るため、ホストが後から SetEnvironmentVariable した値が見えない
	DWORD n=::GetEnvironmentVariableW(name.c_str(), NULL, 0);
	if(n==0) return false;
	std::vector<wchar_t> buf(n);
	DWORD r=::GetEnvironmentVariableW(name.c_str(), &buf[0], n);
	if((r==0)&&(::GetLastError()==ERROR_ENVVAR_NOT_FOUND)) return false;
	if(r>=n) return false;	// 間に値が伸びた
	value.assign(&buf[0], r);
	return true;
#else
	const char *p=getenv(PathToNative(name).c_str());
	if(!p) return false;
	value=UTF8toW(p);
	return true;
#endif
}
//---------------------------------------------------------------------------
