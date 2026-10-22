//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース Win32版(汎用)
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "config.h"
#include "saori/saori_win32.h"
#ifdef __KAWARI__
#	include "libkawari/kawari_log.h"
using namespace kawari_log;
#endif
//---------------------------------------------------------------------------
//#include <ios>
#include <iostream>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
namespace {
	FARPROC getpadd(HMODULE hm, const char *p){
		FARPROC	lpfn;
	
		lpfn = GetProcAddress(hm, p + 1);
		if (!lpfn) lpfn = GetProcAddress(hm, p);
		return lpfn;
	}
	map<HMODULE, unsigned int> loadcount;
}
//---------------------------------------------------------------------------
// SAORIモジュールのロード
bool TSaoriBindingW32::Load (const string &path){
	if (hModule) return false;

	string fn(path);
	for (unsigned int i=0; i<fn.length(); i++)
		if(fn[i]=='/') fn[i]='\\';
	dllpath=fn;

	HMODULE hm=LoadLibrary(fn.c_str());
	if (!hm){
#ifdef __KAWARI__
		Logger.GetStream(LOG_ERROR) << "[SAORI Win32] DLL ("+fn+") load failed." << endl;
#endif
		return false;
	}

	func_load=(BOOL (__cdecl *)(HGLOBAL, long))getpadd(hm, "_load");
	func_request=(HGLOBAL (__cdecl *)(HGLOBAL, long *))getpadd(hm, "_request");
	func_unload=(BOOL (__cdecl *)())getpadd(hm, "_unload");

	if (func_request==NULL){
#ifdef __KAWARI__
		Logger.GetStream(LOG_ERROR) << "[SAORI Win32] import 'request' from ("+fn+") failed." << endl;
#endif
		FreeLibrary(hm);
		return false;
	}

	hModule=hm;
	if (loadcount.count(hModule)){
		loadcount[hModule] ++;
	}else{
		loadcount[hModule] = 1;
	}

	if (func_load){
		string basepath;
		unsigned int pos=fn.find_last_of('\\');
		if (pos==string::npos){
			basepath=fn+"\\";
		}else{
			basepath=fn.substr(0, pos+1);
		}
		long len=basepath.size();
		HGLOBAL h=GlobalAlloc(GMEM_FIXED, len);
		if (!h) return false;
		basepath.copy((char *)h, len);
		if (!(func_load(h, len)))
			return false;
	}
#ifdef __KAWARI__
	Logger.GetStream(LOG_INFO) << "[SAORI Win32] DLL ("+fn+") loaded. loadcount=" << GetLoadCount() << endl;
#endif
	return true;
}
//---------------------------------------------------------------------------
// アンロード
TSaoriBindingW32::~TSaoriBindingW32 (){
	if (hModule){
		if (func_unload)
			(func_unload)();
		FreeLibrary(hModule);
		if (loadcount.count(hModule)){
			--loadcount[hModule];
			if (!loadcount[hModule])
				loadcount.erase(hModule);
		}
	}
#ifdef __KAWARI__
	Logger.GetStream(LOG_INFO) << "[SAORI Win32] DLL ("+dllpath+") unloaded. loadcount=" << GetLoadCount() << endl;
#endif
}
//---------------------------------------------------------------------------
// SAORI/1.0 request
string TSaoriBindingW32::Request(const string &req){
	if (!func_request) return ("");

	string	res;
	HGLOBAL	h;
	long	len;
	
	len = (long)(req.size());
	h = GlobalAlloc(GMEM_FIXED, len);
	if (!h) return ("");
	req.copy((char *)h, len);
	
	h = func_request(h, &len);
	
	if (h) {
		res.assign((const char *)h, len);
		GlobalFree(h);
	}

	return res;
}
//---------------------------------------------------------------------------
// 同ハンドルのライブラリのロードカウントを得る
unsigned int TSaoriBindingW32::GetLoadCount(void){
	if (loadcount.count(hModule))
		return loadcount[hModule];
	else
		return 0;
}
//---------------------------------------------------------------------------
