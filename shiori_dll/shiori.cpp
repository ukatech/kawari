//---------------------------------------------------------------------------
//
// あれ以外の何か以外の何か
// DLLエントリ
// (ここだけ環境依存)
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
//
//---------------------------------------------------------------------------
#include <windows.h>	// Mingw対策で真っ先にinclude
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
using namespace std;
//---------------------------------------------------------------------------
#include "shiori/shiori_interface.h"
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
	// Borland C++
	#define DLL_API extern "C"
#else
	// Visual C++ / Cygwin32 / Mingw32
	#define DLL_API extern "C" __declspec(dllexport)
#endif
//---------------------------------------------------------------------------
// インターフェース規格のバージョン番号を返す
#if 0
DLL_API HGLOBAL __cdecl getversion(long *len)
{
	string verstr=Shiori_GetVersion();

	*len=verstr.size();
	HGLOBAL reth;
	reth=GlobalAlloc(GMEM_FIXED,*len);
	memcpy(reth,verstr.c_str(),*len);

	return(reth);
}
#endif
//---------------------------------------------------------------------------
// 偽AIモジュールのバージョン番号を返す
DLL_API HGLOBAL __cdecl getmoduleversion(long *len)
{
	string verstr=Shiori_GetModuleVersion();

	*len=verstr.size();
	HGLOBAL reth;
	reth=GlobalAlloc(GMEM_FIXED,*len);
	memcpy(reth,verstr.c_str(),*len);

	return(reth);
}
//---------------------------------------------------------------------------
// モジュール読み込み時に呼ばれる
DLL_API BOOL __cdecl load(HGLOBAL h,long len)
{
	string datapath((char*)h,len);
	GlobalFree(h);

	return(Shiori_Load(datapath));
}
//---------------------------------------------------------------------------
// モジュール切り離し時に呼ばれる
DLL_API BOOL __cdecl unload(void)
{
	return(Shiori_Unload());
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
DLL_API HGLOBAL __cdecl request(HGLOBAL h,long *len)
{
	string requeststr((char*)h,*len);
	GlobalFree(h);

	string aistr=Shiori_Request(requeststr);

	*len=aistr.size();
	HGLOBAL reth;
	reth=GlobalAlloc(GMEM_FIXED,*len);
	memcpy(reth,aistr.c_str(),*len);

	return(reth);
}
//---------------------------------------------------------------------------
