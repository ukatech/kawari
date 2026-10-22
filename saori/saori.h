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
#ifndef SAORI_H
#define SAORI_H
//---------------------------------------------------------------------------
#include "config.h"
#include "misc/phttp.h"
//---------------------------------------------------------------------------
#include <string>
#include <map>
//---------------------------------------------------------------------------
// 参照カウントはハンドルに基づいてBindingが管理すること
// WindowsはOSが管理している。
#if defined(WIN32) || defined(_WIN32)
#	include "saori/saori_win32.h"
typedef TSaoriBindingW32 TSaoriBinding;
#elif defined(linux) || defined(__linux__)
#	include "saori/saori_linux.h"
typedef TSaoriBindingLnx TSaoriBinding;
#endif

//---------------------------------------------------------------------------
class TSaoriModule{
public:
	enum LOADTYPE {
		PRELOAD,
		LOADONCALL,
		NORESIDENT
	};

	// SAORI/1.0 request
	bool Request(const TPHMessage &request, TPHMessage &response);

	~TSaoriModule();
private:
	LOADTYPE type;
	std::string libpath;
	TSaoriBinding *bind;

	TSaoriModule(const std::string &path, const LOADTYPE t);
		
	void Attatch(void);
	void Detatch(void);

	// ついにやっちまった。
	friend class TSaoriPark;
};
//---------------------------------------------------------------------------
class TSaoriPark {
public:
	// モジュールの登録
	void RegisterModule(const std::string &aliasname, const std::string &path, const TSaoriModule::LOADTYPE type);

	// モジュール登録の削除
	void EraseModule(const std::string &aliasname);

	// モジュールを得る
	TSaoriModule * const GetModule(const std::string &alias);

	~TSaoriPark();
private:
	// alias -> Module
	std::map<std::string, TSaoriModule *> aliasmap;
};
//---------------------------------------------------------------------------
#endif // SAORI_H
