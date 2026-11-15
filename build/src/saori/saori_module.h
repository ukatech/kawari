//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース
//
//      Programed by Suikyo.
//
//  2002.09.05  Phase 8.1.0   導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (パスはwstring。Requestは生バイト列のまま)
//                            SAORI over DirectSSTP用にRequestMessageを追加
//
//---------------------------------------------------------------------------
#ifndef SAORI_MODULE_H
#define SAORI_MODULE_H
//---------------------------------------------------------------------------
#include "config.h"
#include "libkawari/kawari_log.h"
#include "misc/phttp.h"
#include "misc/charset.h"
#include "misc/dsstp.h"
//---------------------------------------------------------------------------
#include <stddef.h>
#include <string>
#include <vector>
#include <map>
//---------------------------------------------------------------------------
namespace saori{
// ライブラリのハンドル(HMODULE等のポインタ)を入れるので、ポインタと同じ幅が要る。
// 64bit Windows(LLP64)では unsigned long は32bitでハンドルが切り詰められる
typedef size_t SAORI_HANDLE;
//---------------------------------------------------------------------------
// SAORIモジュールを管理するファクトリ
class IModuleFactory{
public:
	// モジュールの検索と生成
	// 戻り値: 生成に成功した場合、モジュール。失敗した場合、NULL。
	virtual class TModule *CreateModule(const std::wstring &path)=0;

	// モジュールの完全破棄
	// ライブラリの場合はFreeLibraryすること。
	virtual void DeleteModule(class TModule *module)=0;

	// コンストラクタ
	IModuleFactory(TKawariLogger &lgr) : logger(lgr) {}

	// 何もしないデストラクタ
	virtual ~IModuleFactory() {}

	// ロガーを返す
	TKawariLogger &GetLogger(void) { return logger; }
private:
	TKawariLogger &logger;
};
//---------------------------------------------------------------------------
class TModuleFactoryMaster : public IModuleFactory{
public:
	// link : 本体への接続 (SAORI over DirectSSTPで使う)
	TModuleFactoryMaster(class TKawariLogger &lgr, const dsstp::TBasewareLink *link);

	// モジュールの検索と生成
	// 戻り値: 生成に成功した場合、モジュール。失敗した場合、NULL。
	class saori::TModule *CreateModule(const std::wstring &path);

	// モジュールの完全破棄
	// ライブラリの場合はFreeLibraryすること。
	void DeleteModule(saori::TModule *module);

	virtual ~TModuleFactoryMaster();

protected:
	std::vector<saori::IModuleFactory *> factory_list;
};
//---------------------------------------------------------------------------
// SAORIモジュールインターフェース
// TModuleは、同一パスについて複数生成されることがある。
// 唯一性の確保は、SAORI_HANDLEに基づいて上位レイヤーで行われる。
class TModule{
public:
	// 初期化
	virtual bool Initialize(void)=0;
	// SAORI/1.0 Load
	virtual bool Load(void)=0;
	// SAORI/1.0 Unload
	virtual bool Unload(void)=0;
	// SAORI/1.0 Request
	virtual std::string Request(const std::string &reqstr)=0;

	// SAORI/1.0 Request (メッセージのまま処理するモジュール用)
	// DLLを介さず、リクエストをシリアライズ前に受け取りたいモジュール
	// (SAORI over DirectSSTP) だけが実装する。
	// 戻り値 : 処理したらtrue (response, rcsを設定すること)。falseならRequestが使われる
	//   rcs : 応答のCharset (無い/不明ならKC_NULL)
	virtual bool RequestMessage(const TPHMessage &request, TPHMessage &response, TKawariCharset &rcs) {
		return false;
	}

	saori::SAORI_HANDLE GetHandle(void) {
		return handle;
	}

	// 何もしないデストラクタ
	// delete TModuleされた場合、Libraryは解放しないこと。
	virtual ~TModule (void){}

	virtual saori::IModuleFactory &GetFactory(void) {
		return factory;
	}

	TModule(saori::IModuleFactory &fac, const std::wstring &p, saori::SAORI_HANDLE hdl)
		 : factory(fac), path(p), handle(hdl) {}

protected:

	saori::IModuleFactory &factory;
	std::wstring path;
	saori::SAORI_HANDLE handle;
};
//---------------------------------------------------------------------------
} // namespace saori
//---------------------------------------------------------------------------
#endif // SAORI_MODULE_H
//---------------------------------------------------------------------------
