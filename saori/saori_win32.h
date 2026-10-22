//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//
//---------------------------------------------------------------------------
#ifndef SAORI_WIN32_H
#define SAORI_WIN32_H
//---------------------------------------------------------------------------
#include <windows.h>
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
// さおりたん縛り
class TSaoriBindingW32{
private:
	HMODULE	hModule;
	BOOL	(__cdecl *func_load)(HGLOBAL, long);
	HGLOBAL	(__cdecl *func_request)(HGLOBAL, long *);
	BOOL	(__cdecl *func_unload)();
	std::string dllpath;

	TSaoriBindingW32 ()
		: hModule(NULL), func_load(NULL), func_request(NULL), func_unload(NULL){}

	bool Load (const std::string &path);

public:

	// SAORIモジュールのロード
	// 引数  path : 共有ライブラリの絶対パス
	// 戻り値 : 成功すればオブジェクトへのポインタ。失敗時はNULL
	static TSaoriBindingW32 *Bind(const std::string &path){
		TSaoriBindingW32 *bind=new TSaoriBindingW32();
		if(bind->Load(path)){
			return bind;
		}else{
			delete bind;
			return NULL;
		}
	}

	// SAORI/1.0 request
	std::string Request(const std::string &request);

	// 同ハンドルのライブラリのロードカウントを得る
	unsigned int GetLoadCount(void);

	// アンロード
	~TSaoriBindingW32 ();
};
//---------------------------------------------------------------------------
#endif // SAORI_WIN32_H
