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
	unsigned int handle=0;
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
	handle=CreateInstanceSafe(path);
	return (handle!=0);
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
	handle=CreateInstanceSafe(path);
	return (handle!=0);
}
//---------------------------------------------------------------------------
// モジュール切り離し直前に呼ばれる
SHIORI_EXPORT BOOL SHIORI_CALL unload(void)
{
	try {
		TKawariShioriFactory::GetFactory().DisposeInstance(handle);
		handle=0;
		TKawariShioriFactory::DisposeFactory();
	} catch(...) {
		handle=0;
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
		resstr=TKawariShioriFactory::GetFactory().RequestInstance(
			handle, string((const char*)h, *len));
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
