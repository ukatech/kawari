//---------------------------------------------------------------------------
//
// あれ以外の何か以外の何か
// 共有ライブラリエントリ
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.02.03  Phase 0.3     ノーコメント
//  2001.03.15  Phase 0.42    construct/destructの使用を止める
//  2001.04.25  Phase 0.50a1  inverse
//  2001.04.27  Phase 0.50a2  SHIORI/2.1対応
//  2001.05.09  Phase 0.50    SHIORI/2.2対応
//  2001.05.22  Phase 5.10    SHIORI/2.3対応
//  2001.07.10  Phase 6.0     Cygwin対応
//  2001.07.10  Phase 6.1     getmoduleversion追加
//  2001.07.19  Phase 6.2     Mingw対応
//  2002.01.07  Phase 7.3     getversion削除(著作者表示対策)
//  2003.02.25  Phase 8.1.0   プラットフォーム共通化
//  2026.10.18  Phase 9.0.0   wchar_t化 (loadu追加)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "shiori/kawari_shiori.h"
#include "include/shiori.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstring>
using namespace std;
//---------------------------------------------------------------------------
namespace {
	inline MEMORY_HANDLE string2memory(const string &s, long *len){
		*len=static_cast<long>(s.length());
		MEMORY_HANDLE reth=(MEMORY_HANDLE)SHIORI_MALLOC(*len);
		if(!reth) {
			*len=0;
			return NULL;
		}
		memcpy(reth, s.c_str(), *len);
		return reth;
	}
	// 例外をextern "C"の境界の外(ホスト)に出さないため、各エントリはこれで包む
	inline unsigned int CreateInstanceSafe(const wstring &path){
		try {
			return TKawariShioriFactory::GetFactory().CreateInstance(path);
		} catch(...) {
			return 0;
		}
	}
	inline bool CreateLegacySafe(const wstring &path){
		try {
			return TKawariShioriFactory::GetFactory().CreateLegacy(path);
		} catch(...) {
			return false;
		}
	}
	// 生きているインスタンスが1つも無くなったら、ファクトリを破棄する
	// (従来のインスタンスと multi_* のインスタンスの全てが対象)
	inline void DisposeFactoryIfEmpty(void){
		if (TKawariShioriFactory::GetFactory().Empty())
			TKawariShioriFactory::DisposeFactory();
	}
	// multi_load / multi_loadu の共通処理。失敗は -1
	inline long MultiLoad(const MEMORY_HANDLE h, long len, bool utf8){
		wstring path;
		try {
			string bytes((const char*)h, len);
			path=utf8 ? UTF8toW(bytes) : ACPtoW(bytes);
		} catch(...) {
		}
		SHIORI_FREE(h);
		unsigned int id=CreateInstanceSafe(path);
		if (id==0) return -1;
		return static_cast<long>(id);
	}
}
//---------------------------------------------------------------------------
// 偽AIモジュールのバージョン番号を返す (華和梨拡張)
SHIORI_EXPORT MEMORY_HANDLE SHIORI_CALL getmoduleversion(long *len)
{
	try {
		string modver=TKawariShioriFactory::GetModuleVersion();
		return string2memory(modver, len);
	} catch(...) {
		*len=0;
		return NULL;
	}
}
//---------------------------------------------------------------------------
// モジュール読み込み直後に呼ばれる
// パスはOSの既定コードページ(ACP)
SHIORI_EXPORT BOOL SHIORI_CALL load(const MEMORY_HANDLE h,long len)
{
	wstring path;
	try {
		path=ACPtoW(string((const char*)h, len));
	} catch(...) {
	}
	SHIORI_FREE(h);
	return CreateLegacySafe(path);
}
//---------------------------------------------------------------------------
// モジュール読み込み直後に呼ばれる (パスがUTF-8)
SHIORI_EXPORT BOOL SHIORI_CALL loadu(const MEMORY_HANDLE h,long len)
{
	wstring path;
	try {
		path=UTF8toW(string((const char*)h, len));
	} catch(...) {
	}
	SHIORI_FREE(h);
	return CreateLegacySafe(path);
}
//---------------------------------------------------------------------------
// モジュール切り離し直前に呼ばれる
// multi_load* で作ったインスタンスには触れない (残っていればファクトリも残す)
SHIORI_EXPORT BOOL SHIORI_CALL unload(void)
{
	try {
		TKawariShioriFactory::GetFactory().DisposeLegacy();
		DisposeFactoryIfEmpty();
	} catch(...) {
		return FALSE;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
SHIORI_EXPORT MEMORY_HANDLE SHIORI_CALL request(const MEMORY_HANDLE h,long *len)
{
	string resstr;
	try {
		resstr=TKawariShioriFactory::GetFactory().RequestLegacy(
			string((const char*)h, *len));
	} catch(...) {
		// 応答を作れなかった (不正なハンドルのときと同じく空で返す)
		resstr="";
	}

	SHIORI_FREE(h);
	try {
		return string2memory(resstr, len);
	} catch(...) {
		*len=0;
		return NULL;
	}
}
//---------------------------------------------------------------------------
// マルチインスタンス用 (任意の関数。ukadoc-spec の spec/dll.md を参照)
//  - id は1以上。0以下は無効で、従来の load / request / unload のインスタンスとは
//    別枠 (どの id でもそのインスタンスには届かない)
//  - 失敗したときは、-1 (multi_load*) / FALSE (multi_unload) / NULL (multi_request)
//  - h は、成功でも失敗でも必ずここで解放する
//  - 呼び出しは1つのスレッドから順に来る前提 (ファクトリは排他制御しない)
//---------------------------------------------------------------------------
// 新しいインスタンスを作る (パスはOSの既定コードページ)
SHIORI_EXPORT long SHIORI_CALL multi_load(const MEMORY_HANDLE h,long len)
{
	return MultiLoad(h, len, false);
}
//---------------------------------------------------------------------------
// 新しいインスタンスを作る (パスがUTF-8)
SHIORI_EXPORT long SHIORI_CALL multi_loadu(const MEMORY_HANDLE h,long len)
{
	return MultiLoad(h, len, true);
}
//---------------------------------------------------------------------------
// id で指したインスタンスを終了する
SHIORI_EXPORT BOOL SHIORI_CALL multi_unload(long id)
{
	if (id<=0) return FALSE;
	try {
		TKawariShioriFactory &factory=TKawariShioriFactory::GetFactory();
		bool ok=factory.DisposeInstance(static_cast<unsigned int>(id));
		DisposeFactoryIfEmpty();
		return ok ? TRUE : FALSE;
	} catch(...) {
		return FALSE;
	}
}
//---------------------------------------------------------------------------
// id で指したインスタンスへリクエストを渡す
SHIORI_EXPORT MEMORY_HANDLE SHIORI_CALL multi_request(long id,const MEMORY_HANDLE h,long *len)
{
	string resstr;
	bool exist=false;
	if (id>0) {
		try {
			TKawariShioriFactory &factory=TKawariShioriFactory::GetFactory();
			exist=factory.HasInstance(static_cast<unsigned int>(id));
			if (exist)
				resstr=factory.RequestInstance(static_cast<unsigned int>(id),
					string((const char*)h, *len));
		} catch(...) {
			// 応答を作れなかった (空で返す)
			resstr="";
		}
	}

	SHIORI_FREE(h);
	if (!exist) {
		*len=0;
		return NULL;
	}
	try {
		return string2memory(resstr, len);
	} catch(...) {
		*len=0;
		return NULL;
	}
}
//---------------------------------------------------------------------------
