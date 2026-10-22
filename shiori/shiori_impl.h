//---------------------------------------------------------------------------
//
// あれ以外の何か以外の何か
// shiori.dllインターフェース
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.02.03  Phase 0.3     ノーコメント
//  2001.03.15  Phase 0.42    unload予約
//  2001.04.25  Phase 0.50a1  inverse
//  2001.04.27  Phase 0.50a2  SHIORI/2.1対応
//  2001.05.12  Phase 0.50    SHIORI/1.1切り捨て
//  2001.07.10  Phase 6.0     getmoduleversion追加
//
//---------------------------------------------------------------------------
#ifndef SHIORI_IMPL_H
#define SHIORI_IMPL_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#if 0
// こんなの用意してください
#define SHIORIVER	"2.0"
#include <string>
using namespace std;
class TNS_ANI {
public:
	// SHIORI/2.0 API

	// DLLロード時に呼ばれる
	// const string& datapath : DLLのディレクトリパス
	// 戻り値 bool : 成功でtrue
	bool Load(const string& datapath);

	// DLLアンロード時に呼ばれる
	// 戻り値 bool : 成功でtrue
	bool Unload(void);

	// 偽AIリクエスト
	// const string& type : リクエストの種類
	// const map<string,string>& request : リクエストヘッダ
	// 戻り値 string : レスポンスデータ
	string Request(const string& type,const map<string,string>& request);


	// SHIORI for POSIX 2.4

	// 偽AIモジュールのバージョン番号を返す
	// 戻り値 string : "基本名称[.補助名称[.補助名称]]/バージョン番号"
	// ex. "KAWARI/6.0"         華和梨 Phase6.0
	// ex. "KAWARI.sky/5.4.1"   酔狂版 華和梨 Phase5.4改
	string GetModuleVersion(void);

};
#endif
//---------------------------------------------------------------------------
// 使用するクラスを変更するときは以下を書き換える
#include "libkawari/kawari.h"
#define TShioriInterface TNS_KawariANI
//---------------------------------------------------------------------------
#endif
