//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨エンジン
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//                            クラス階層整理
//  2001.05.31  Phase 5.2     保守的piro
//  2001.06.09  Phase 5.3     マッチエントリ
//  2001.06.10  Phase 5.3.1   微調整
//  2001.06.17  Phase 5.4     save
//  2001.08.25  Phase 7.0     セキュリティ対策(WriteProtect)
//  2001.12.08  Phase 7.1.2   テキストファイル読み込み対応
//
//---------------------------------------------------------------------------
#ifndef KAWARI_ENGINE_H
#define KAWARI_ENGINE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <set>
#include <string>
#include <vector>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
// #include "libkawari/kawari_engine_base.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_iscript.h"
#include "libkawari/kawari_matchdict.h"
#include "libkawari/kawari.h"
#include "saori/saori.h"
//---------------------------------------------------------------------------
class TKawariEngine {
private:

	// 辞書ファイルのパス名
	string DataPath;

	// 辞書
	TNS_KawariDictionary *Dictionary;

	// スクリプトエンジン
	TKisEngine *KisEngine;

	// 華和梨フォーマットファイルの単語を解釈して取得
	TKawariCode_base *ParseKawariDict(TKawariLexer &buff,const string& localid);

	// マッチエントリ
	TNS_KawariMatchDictionary *MatchDictionary;

	set<TEntryID> ProtectedEntry;

	// 華和梨インターフェース(ログへのアクセス用)
	class TNS_KawariANI *Kawari;

	// SAORI
	class TSaoriPark *SaoriPark;
public:

	// 情報系API

	// 総単語数を取得
	unsigned int WordCollectionSize(void) const
	{
		return(Dictionary->WordCollectionSize());
	}

	// 有効エントリ数を取得
	unsigned int EntryCollectionSize(void) const
	{
		return(Dictionary->EntryCollectionSize());
	}

	// マッチエントリ数を取得
	unsigned int MatchCollectionSize(void) const
	{
		return(MatchDictionary->Size());
	}


	// 辞書追加・削除系API

	// 文字列をエントリ名で使用可能な文字列にエンコードする
	string EncodeEntryName(const string &orgsen) const;

	// 指定されたエントリを空にする
	// メモリに空エントリと単語が残っても良い
	void ClearEntry(const string& entryname);

	// 辞書への単語の追加
	// 既に単語が存在する場合でも、さらに追加する (現在の仕様)
	// 既に単語が存在する場合には、出現確率を変更する (将来の仕様)
	void Insert(const string& entryname,const string& word);

	// 指定されたエントリを空にしてから単語を追加
	// エントリを変数的に利用する時に使用する
	void InsertAfterClear(const string& entryname,const string& word);

	// 指定されたエントリへの書き込みを禁止する
	virtual void WriteProtect(const string& entryname);


	// 検索系API

	// 指定されたエントリの先頭の単語を返す
	// 戻り値 : 単語の文字列
	string FindFirst(const string& entryname) const;

	// 指定されたエントリの単語をランダムに選択して返す
	// 戻り値 : 単語の文字列
	string RandomSelect(const string& entryname) const;

	// 発動条件のそろったマッチエントリの単語をランダムに選択して返す
	// 戻り値 : 単語の文字列
	string MatchEntryRandomSelect(const string& entryname) const;


	// ファイルAPI

	// 辞書ファイルのパス名を設定する
	void SetDataPath(const string &datapath)
	{
		DataPath=datapath;
	}

	// 辞書ファイルのパス名を取得する
	string GetDataPath(void) const
	{
		return(DataPath);
	}

	// 華和梨フォーマット辞書ファイルを読み込む
	// 戻り値 : 成功でtrue
	bool LoadKawariDict(const string &filename);

	// 華和梨フォーマット辞書ファイルを書き込む
	// 戻り値 : 成功でtrue
	bool SaveKawariDict(const string &filename,const vector<string>& entry,bool crypt=false) const;

	// テキストファイルを読み込む
	// 戻り値 : 成功でtrue
	bool LoadTextFile(const string &filename,const string &baseentry);


	// SAORI関係API

	enum SAORILOADTYPE {
		PRELOAD=TSaoriModule::PRELOAD,
		LOADONCALL,
		NORESIDENT
	};

	// SAORIモジュールの登録
	void RegisterSAORIModule(const std::string &aliasname, const std::string &path, const SAORILOADTYPE type);

	// SAORIモジュール登録の削除
	void EraseSAORIModule(const std::string &aliasname);

	// SAORIリクエストを行う
	bool RequestToSAORIModule(
		const std::string &aliasname,
		const TPHMessage &request, TPHMessage &response);


	// 非共通API

	// 与えられた単語を解釈する
	string Parse(const string& word);

	// ログストリームを得る
	ostream *GetLogStream(void);
public:

	TKawariEngine(TNS_KawariANI *kawari);
//	{
//		Dictionary=new TNS_KawariDictionary;
//		KisEngine=new TKisEngine(Dictionary,this);
//		MatchDictionary=new TNS_KawariMatchDictionary(Dictionary,this);
//		SaoriPark=new TSaoriPark();
//		Kawari=kawari;
//	}

	~TKawariEngine(){
		delete SaoriPark;
		delete Dictionary;
		delete KisEngine;
		delete MatchDictionary;
	}

};
//---------------------------------------------------------------------------
#endif

