//---------------------------------------------------------------------------
//
// あれ以外の何か以外の何か
// SharedObjectエントリ
// (ここだけ環境依存)
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.22  Phase 5.10    SHIORI/2.3対応
//  2001.07.10  Phase 6.0     getmoduleversion追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
using namespace std;
//---------------------------------------------------------------------------
#include "shiori/shiori_interface.h"
//---------------------------------------------------------------------------
#define DLL_API extern "C"
//---------------------------------------------------------------------------
// インターフェース規格のバージョン番号を返す
DLL_API const char *getversion(void)
{
	static string verstr;

	verstr=Shiori_GetVersion();

	return(verstr.c_str());
}
//---------------------------------------------------------------------------
// 偽AIモジュールのバージョン番号を返す
DLL_API const char *getmoduleversion(void)
{
	static string verstr;

	verstr=Shiori_GetModuleVersion();

	return(verstr.c_str());
}
//---------------------------------------------------------------------------
// モジュール読み込み時に呼ばれる
DLL_API int load(const char *datapath)
{
	return(Shiori_Load(datapath));
}
//---------------------------------------------------------------------------
// モジュール切り離し時に呼ばれる
DLL_API int unload(void)
{
	return(Shiori_Unload());
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
DLL_API char *request(const char *requeststr)
{
	string aistr=Shiori_Request(requeststr);

	char *reth=(char*)malloc(aistr.size());
	memcpy(reth,aistr.c_str(),aistr.size());

	return(reth);
}
//---------------------------------------------------------------------------
