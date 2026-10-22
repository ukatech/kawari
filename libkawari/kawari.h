//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 偽AI代用品
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.02.03  Phase 0.3     ノーコメント
//  2001.02.11  Phase 0.31    再帰定義実装
//  2001.02.23  Phase 0.4     卒論戦争終戦祈念
//                            ～安らかにお眠り下さい、過ちは繰り返しません～
//                                                                バージョン
//                            kawari.ini導入
//                            複数辞書ファイル
//                            sentence.txt廃止
//  2001.03.15  Phase 0.42    unload予約
//  2001.04.25  Phase 0.50a1  レスポンス対応
//  2001.04.27  Phase 0.50a2  SHIORI/2.1対応
//  2001.04.28  Phase 0.50a3  COMMUNICATE戦争
//       |
//  2001.05.02
//  2001.05.03  Phase 0.50a4  インラインスクリプト
//  2001.05.12  Phase 0.50    Piroject-X 完結編
//                            SHIORI/2.2対応
//                            SHIORI/1.1切り捨て
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//  2001.05.31  Phase 5.2     保守的piro
//  2001.06.10  Phase 5.3.1   GET Version対策
//                            偽SHIORI/2.4実装 (^_^;
//  2001.06.18  Phase 5.4     インラインスクリプト大幅強化
//  2001.07.10  Phase 6.0     getmoduleversion追加
//  2001.07.14  Phase 6.1     BCCメモリ浪費問題対策
//  2001.07.19  Phase 6.2     SHIORI/2.5実装
//  2001.08.08  Phase 6.2     幸水用にLoadSubを新設
//  2001.08.25  Phase 6.3     セキュリティーホール対策
//  2001.08.25  Phase 7.0     セキュリティ対策(WriteProtect)
//  2001.09.23  Phase 7.0.1   セキュリティ対策をredo34以降の方式に対応
//                            Sender経路情報追加(System.Sender.Path)
//                            3エントリ以上でのAND検索バグを修正
//                            SHIORI/2.3b対応
//                            インラインスクリプト強化
//                            起動速度の高速化
//                            ハイフンを使用する旧エントリの対応停止
//  2001.10.29  Phase 7.0.2   GhostExヘッダ対応
//                            GET Versionを再度kawari.cppに移管
//                            SHIORI/2.6仮対応(現状Shift_JISに固定)
//                            内部イベント追加(OnEventUnknown)
//  2001.10.31  Phase 7.0.3   SecurityLevelヘッダ対応
//  2001.11.21  Phase 7.1     エントリランダム呼び出しのバグ修正
//                            擬似乱数にMTアルゴリズムを採用
//  2001.11.25  Phase 7.1.1   ランダムの初期値に時刻を使うよう変更
//                            [幸水] helpコマンドのメッセージを(嘘臭い)英語に
//                            SecurityLevelを0～3に変更(4を廃止)
//  2001.12.08  Phase 7.1.2   kawari.iniでテキストファイル読み込み対応
//  2001.12.16  Phase 7.2     TMMapの値が復活するバグを修正
//                            (Thanks: しの、酔狂)
//  2002.01.07  Phase 7.3     著作者表示用ヘッダ追加(Thanks: えびさわ)
//                            伝達経路情報をSecurityLevelのみで判別
//  2002.01.08  Phase 7.3.1   BalloonOffsetヘッダ対応
//  2002.05.07  Phase 7.4.0   修正BSDライセンスに統一
//  2003.02.16  Phase 7.5.0   I18N(Thanks:Sanori,whoami)
//
//---------------------------------------------------------------------------
#ifndef KAWARI_H
#define KAWARI_H
//---------------------------------------------------------------------------
// 華和梨のバージョン番号はここに書く
#define KAWARI_NAME	"KAWARI.kdt.i18n"
#define KAWARI_AUTHOR	"KawariDevelopmentTeam"	// 著作者(英数字だけ)
//#define KAWARI_NAME	"KAWARI.sky"
#define KAWARI_MAJOR	"7"		// メジャー番号(ピリオド無し、数字だけ)
#define KAWARI_MINOR	"5"		// マイナー番号(ピリオド無し、数字だけ)
#define KAWARI_SUBMINOR	"0"		// さらにマイナー(ピリオド無し、数字だけ)
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
#include <fstream>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "misc/phttp.h"
#include "misc/mmap.h"
//---------------------------------------------------------------------------
#define SHIORIVER	"2.6"
//---------------------------------------------------------------------------
#define SENDER_SYSTEM	0
#define SENDER_LOCAL	1
#define SENDER_UNKNOWN	2
#define SENDER_EXTERNAL	3
//---------------------------------------------------------------------------
class TNS_KawariANI {
private:
public:

	// 華和梨エンジン
	TKawariEngine *Engine;

	// ログファイル出力用
	ofstream *LogFS;

	// 識別用のゴースト名
	// NOTIFYで送られてくる名前
	string GhostName;

	// 他のゴーストのリスト
	set<string> OtherGhostName;
	// 他のゴーストのリスト(サーフェス番号付)
	set<string> OtherGhostExName;

	// セキュリティレベル
	class TSecurityLevel {
	private:
		unsigned int Level;
	public:
		// セキュリティレベルを設定する
		// 3 : LOCALを許可する
		// 2 : LOCAL, UNKNOWNを許可する
		// 1 : (予約,とりあえず2と同じ)
		// 0 : LOCAL, UNKNOWN, EXTERNALを許可する
		bool Set(unsigned int lv)
		{
			Level=(lv<=3)?lv:Level;
			return(Level==lv);
		}

		unsigned int Get(void)
		{
			return(Level);
		}

		// Senderが許可されているかどうかチェックする
		bool Check(const unsigned int path)
		{
			switch(Level) {
//			case 4: return(path==SENDER_SYSTEM);
			case 3: return(path==SENDER_LOCAL);
			case 2: return(path<=SENDER_UNKNOWN);
			case 1: return(path<=SENDER_UNKNOWN);
			case 0: return(path<=SENDER_EXTERNAL);
			default: return(path<=SENDER_UNKNOWN);
			}
		}

		TSecurityLevel(unsigned int lv)
		{
			Set(lv);
		}
	} SecurityLevel;


	// リクエストに応じて返答文を生成する
	unsigned int RequestGetSentence(TMMap<string,string>& request,map<string,string> &response);

	// Communicateの相手を探す
	bool SearchCommunicateTarget(const string &targetghost,map<string,string> &response);

	// エントリに一致するランダム単語を返す
	string GetSentence(const string& key);

	// 文章に返答するランダム単語を返す
	string GetResponse(string sentence);

	// データファイル読み込み
	bool LoadFile(const string &filename,const string &type);

	// SAKURAフォーマットデータファイル読み込み
	bool LoadSakuraFile(const string &filename,const string &type);

	// iniファイル読み込み
	bool LoadINI(const string &filename,const string &datapath,vector<string> &dictfiles);


public:

	TNS_KawariANI(void);

	~TNS_KawariANI();

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
	// const TMMap<string,string>& request : リクエストヘッダ
	// 戻り値 string : レスポンスデータ
	string Request(const string& type,TMMap<string,string>& request);


	// SHIORI for POSIX 2.4

	// 偽AIモジュールのバージョン番号を返す
	// 戻り値 string : "基本名称[.補助名称[.補助名称]]/バージョン番号"
	// ex. "KAWARI/6.0"         華和梨 Phase6.0
	// ex. "KAWARI.sky/5.4.1"   酔狂版 華和梨 Phase5.4改
	string GetModuleVersion(void)
	{
		return(KAWARI_NAME "/" KAWARI_MAJOR "." KAWARI_MINOR "." KAWARI_SUBMINOR);
	}


	// 幸水用

	// 初期化とファイル読み込み
	// const string& datapath : DLLのディレクトリパス
	// const string& inifile : kawari.iniファイルのパス
	// 戻り値 bool : 成功でtrue
	bool LoadSub(const string& datapath,const string& inifile);

};
//---------------------------------------------------------------------------
#endif
