//---------------------------------------------------------------------------
//
// KOSUI -- Kawari cOnSole Use Interpritor --
// 幸水 -- コンソール版華和梨インタープリタ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.30  Phase 5.1     コンソールインタープリタ
//  2001.08.06  Phase 6.2     KawariDebuggerに合わせてデフォルトをecho-modeに
//  2001.08.12  Phase 6.2.1   KawariDebuggerと統合
//  2002.01.08  番外          NothingIsDone向け
//  2002.03.12  Phase 7.9.0   Attach入力の所のちんまいバグ修正
//  2002.04.12  Phase 8.0.0   インターフェース切り直し
//  2008.01.23  Phase 8.2.5   quietモード追加
//  2026.10.18  Phase 9.0.0   wchar_t化 (コンソール入出力はACP)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#	define _POSIX_
#	include <limits.h>
#endif
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "tool/kawari_kosui.h"
#include "misc/misc.h"
#include "misc/charset.h"
#include "tool/wargs.h"
#ifdef USEKDB
#	include "tool/kdb.h"
#	include "tool/kosui_dsstp.h"
#endif
#if defined(KAWARI_POSIX)
#	include <unistd.h>
#	include <limits.h>
#else
#	include <direct.h>
#endif
#ifdef _MSC_VER
#define PATH_MAX _MAX_PATH
#endif
//---------------------------------------------------------------------------
// 接続するゴーストを選ぶ
TKawariInterface_base* AttachTarget(const wstring& datapath,const wstring& inifile,const wstring& event)
{
	cout << "[" << 0 << "] : " << " Interpreter mode." << endl;

	#ifdef USEKDB
		// 華和梨デバッガを組み込む場合
		map<HWND,wstring> ghostname;
		GetGhostList(ghostname);

		if(ghostname.size()>0) {
			vector<HWND> ghostlist;
			map<HWND,wstring>::iterator it;
			for(it=ghostname.begin();it!=ghostname.end();it++) {
				ghostlist.push_back(it->first);
				cout << "[" << ghostlist.size() << "] : " << WtoACP(it->second) << endl;
			}

			unsigned int no;
			while(true) {
				cout << "Attach : ";
				string buff;
				getline(cin,buff);
				no=atoi(buff.c_str());
				if(no<=ghostlist.size()) break;
			}

			if(no>0) return(new TKosuiDSSTPInterface(ghostlist[no-1],event));
		}
	#endif

	return(new TKawariKosuiAdapter(datapath,inifile));
}
//---------------------------------------------------------------------------
// JSON 文字列のエスケープ
static wstring JsonEscape(const wstring &s)
{
	wstring r;
	for (wstring::size_type i=0; i<s.size(); i++) {
		wchar_t c=s[i];
		switch (c) {
		case L'"':  r+=L"\\\""; break;
		case L'\\': r+=L"\\\\"; break;
		case L'\n': r+=L"\\n"; break;
		case L'\r': r+=L"\\r"; break;
		case L'\t': r+=L"\\t"; break;
		default:
			if (c<0x20) {
				wostringstream o;
				o << L"\\u" << hex << setw(4) << setfill(L'0') << (unsigned int)c;
				r+=o.str();
			} else {
				r+=c;
			}
		}
	}
	return r;
}
//---------------------------------------------------------------------------
// 診断コードの表記 (K-E0001 / K-W0001)
static wstring DiagCodeName(const TDiagRecord &r)
{
	wostringstream o;
	o << L"K-" << (r.iserror ? L"E" : L"W") << setw(4) << setfill(L'0') << r.code;
	return o.str();
}
//---------------------------------------------------------------------------
// --check の結果を JSON で書く
static void WriteCheckJson(wostream &os, const TDiagCollector &c)
{
	os << L"{\"diagnostics\":[";
	for (vector<TDiagRecord>::const_iterator it=c.records.begin(); it!=c.records.end(); ++it) {
		if (it!=c.records.begin()) os << L",";
		os << L"\n{\"severity\":\"" << (it->iserror ? L"error" : L"warning") << L"\"";
		os << L",\"code\":\"" << DiagCodeName(*it) << L"\"";
		if (it->hasloc) {
			os << L",\"file\":\"" << JsonEscape(it->file) << L"\"";
			if (it->line>0) os << L",\"line\":" << it->line; else os << L",\"line\":null";
		} else {
			os << L",\"file\":null,\"line\":null";
		}
		os << L",\"message\":\"" << JsonEscape(it->message) << L"\"";
		os << L",\"chain\":[";
		for (vector<wstring>::const_iterator ci=it->chain.begin(); ci!=it->chain.end(); ++ci) {
			if (ci!=it->chain.begin()) os << L",";
			os << L"\"" << JsonEscape(*ci) << L"\"";
		}
		os << L"]}";
	}
	os << L"\n],\"errors\":" << c.errors << L",\"warnings\":" << c.warnings << L"}" << endl;
}
//---------------------------------------------------------------------------
int main(int argc,char *argv[])
{
	vector<wstring> args=GetWideArgs(argc, argv);

	wstring datapath;
#ifdef KAWARI_WIN32
	wchar_t cwd[PATH_MAX+1];
	if(NULL==_wgetcwd(cwd, PATH_MAX+1)){
#else
	char cwd[PATH_MAX+1];
	if(NULL==getcwd(cwd, PATH_MAX+1)){
#endif
		datapath=CanonicalPath(L"./");
	}else{
#ifdef KAWARI_WIN32
		datapath=cwd;
#else
		datapath=UTF8toW(string(cwd));
#endif
		if(datapath.length()&&(datapath[datapath.length()-1]!=FILE_SEPARATOR))
			datapath+=FILE_SEPARATOR;
	}
	wstring inifile=L"kawarirc.kis";
	wstring event=L"ShioriEcho";
	bool quietmode=false;

	// --check のオプション
	bool checkmode=false;
	bool checkjson=false;
	bool checkwerror=false;
	bool checkdecl=false;
	bool checkseed=false;
	bool checklint=true;
	unsigned int seedvalue=0;
	vector<wstring> evals;

	if(args.size()>1) {
		for (int i=1; i<(int)args.size(); i++){
			wstring param=args[i];
			if (param==L"--check"){
				checkmode=true;
			}else if (param==L"--werror"){
				checkwerror=true;
			}else if (param==L"--decl"){
				checkdecl=true;
			}else if (param==L"--no-lint"){
				checklint=false;
			}else if ((param==L"--format")||(param.compare(0, 9, L"--format=")==0)){
				wstring fmt;
				if (param.size()>9) fmt=param.substr(9);
				else if (++i<(int)args.size()) fmt=args[i];
				if (fmt==L"json") checkjson=true;
				else if (fmt!=L"text") { cerr << "kosui: unknown format (text or json)" << endl; return 3; }
			}else if (param==L"--eval"){
				if (++i<(int)args.size()) evals.push_back(args[i]);
			}else if (param==L"--seed"){
				if (++i<(int)args.size()) { checkseed=true; seedvalue=(unsigned int)StringToInt(args[i]); }
			}else if ((param==L"--event")||(param==L"-e")){
				if (++i<(int)args.size()){
					param=args[i];
					if (param.size())
						event=param;
				}
			}else if (param==L"--norc"){
				inifile=L"";
			}else if (param==L"--quiet"){
				quietmode=true;
			}else{
				inifile=param;
				wstring::size_type pos=inifile.rfind(L'/');
				if(pos==wstring::npos) pos=inifile.rfind(L'\\');
				if(pos!=wstring::npos)
					datapath=CanonicalPath(inifile.substr(0,pos+1));
			}
		}
		if(checkmode){
			// 検査モード : 安全モードで辞書を読み、出た診断を報告して終了する。
			// 終了コード 0 = 問題なし, 1 = 警告のみ, 2 = エラーあり (--werror なら警告も 2), 3 = 実行できない
			// AttachTarget は通さない (動作中のゴーストに接続してしまうため)。
			TDiagCollector collector;
			collector.keep=checkjson;
			TWideOStream textout(&cout, KC_ACP);
			unsigned int level=kawari_log::LOG_ERROR|kawari_log::LOG_WARNING;
			if (checkdecl) level|=kawari_log::LOG_DECL;
			bool loaded=false;
			{
				TKawariKosuiAdapter adapter(datapath, inifile, checkjson ? NULL : &textout, &collector,
											level, checkseed, seedvalue, loaded);
				if (loaded) {
					// 静的検査は --eval の前に行う (--eval が作るエントリで定義済みと見なさないため)
					if (checklint) adapter.Lint();
					for (vector<wstring>::const_iterator it=evals.begin(); it!=evals.end(); ++it)
						adapter.Parse(*it);
				}
			}
			if (!loaded) {
				cerr << "kosui: cannot read " << WtoACP(inifile) << endl;
				return 3;
			}
			if (checkjson) {
				TWideOStream jsonout(&cout, KC_UTF8);
				WriteCheckJson(jsonout, collector);
			} else {
				textout << L"check: " << collector.errors << L" error(s), " << collector.warnings << L" warning(s)" << endl;
			}
			if (collector.errors) return 2;
			if (collector.warnings) return checkwerror ? 2 : 1;
			return 0;
		}
		if(quietmode){
			// quietモード(inifileを実行したらすぐに終了)
			TKawariInterface_base *Kawari=new TKawariKosuiAdapter(datapath,inifile);
			delete Kawari;
			return(0);
		}
	}

	cout << "K O S U I -- Kawari cOnSole Use Interpritor --" << endl;
	cout << "2001 Programed by NAKAUE.T (Meister)" << endl << endl;

	TKawariInterface_base *Kawari=AttachTarget(datapath,inifile,event);

	cout << endl;
	cout << WtoACP(Kawari->GetInformation()) << endl << endl;

	cout << "'.' is mode change command." << endl;

	bool commode=false;
	while(true) {
		if(commode) cout << "command-mode > " << flush;
		 else cout << "echo-mode > " << flush;

		string bytes;
		if(!getline(cin,bytes)) break;	// EOF
		// 標準入力はACP
		wstring buff=ACPtoW(bytes);

		if(buff.size()==0) continue;

		if(buff==L"exit") break;
		if(buff==L".") {
			commode=!commode;
			continue;
		}

		if(commode) cout << WtoACP(Kawari->Parse(wstring(L"$(")+buff+L")")) << endl;
		 else cout << WtoACP(Kawari->Parse(buff)) << endl;
	}

	delete Kawari;

	return(0);
}
//---------------------------------------------------------------------------

