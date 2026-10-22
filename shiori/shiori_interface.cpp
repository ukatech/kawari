//---------------------------------------------------------------------------
//
// あれ以外の何か以外の何か
// SHIORI/2.4インターフェース
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.22  Phase 5.1     SHIORI/2.3対応
//  2001.06.09  Phase 5.3     GET Version対策
//  2001.06.10  Phase 5.3.1   バージョン番号一括管理
//  2001.07.10  Phase 6.0     getmoduleversion追加
//  2001.10.29  Phase 7.0.2   GET Versionを再度kawari.cppへ移管
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <strstream>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "shiori/shiori_interface.h"
#include "shiori/shiori_impl.h"	// ここでバージョン番号(SHIORIVER)を定義
//---------------------------------------------------------------------------
static TShioriInterface *ShioriInterface=0;
//---------------------------------------------------------------------------
// インターフェース規格のバージョン番号を返す
string Shiori_GetVersion(void)
{
	return("SHIORI/"SHIORIVER);
}
//---------------------------------------------------------------------------
// 偽AIモジュールのバージョン番号を返す
string Shiori_GetModuleVersion(void)
{
	return(ShioriInterface->GetModuleVersion());
}
//---------------------------------------------------------------------------
// モジュール読み込み時に呼ばれる
bool Shiori_Load(const string& dllpath)
{
	ShioriInterface=new TShioriInterface;

	return(ShioriInterface->Load(dllpath));
}
//---------------------------------------------------------------------------
// モジュール切り離し時に呼ばれる
bool Shiori_Unload(void)
{
	bool status=ShioriInterface->Unload();

	delete ShioriInterface;

	return(status);
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
string Shiori_Request(const string& requeststr)
{
	TMMap<string,string> requestdata;
	requestdata.Add("ORIGINAL",requeststr);		// デバッグ用

	istrstream *ifs=new istrstream(requeststr.c_str());
	string buff;
	string::size_type pos;

	// リクエストの種類を切り出す
	// ex. 「GET Sentence SHIORI/2.0」->「GET Sentence」
	getline(*ifs,buff,'\x0a');
	pos=buff.find(" SHIORI/");
	string type=buff.substr(0,pos);

	string response;
	if(!ShioriInterface) {
		// 偽AIが初期化されていない
		response=Shiori_GetVersion()+" 500 Internal Server Error\x0d\x0a\x0d\x0a";
	} else {
		// リクエストヘッダを分解してmapに積む
		while(getline(*ifs,buff,'\x0a')) {
			if(buff.size()==0) break;
			if(buff[buff.size()-1]=='\x0d') buff.erase(buff.size()-1);	// 0x0dを取り除く
			if(buff.size()==0) break;

			pos=buff.find(':');
			string key=buff.substr(0,pos);

			pos++;
			while(buff[pos]==' ') pos++;

			requestdata.Add(key,buff.substr(pos));
		}

		response=ShioriInterface->Request(type,requestdata);
	}

	delete ifs;

	return(response);
}
//---------------------------------------------------------------------------
