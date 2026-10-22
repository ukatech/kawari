//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// マッチエントリ辞書
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.06.09  Phase 5.3     マッチエントリ再実装
//
//---------------------------------------------------------------------------
#ifndef KAWARI_MATCHDICT_H
#define KAWARI_MATCHDICT_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine_fwd.h"
#include "libkawari/kawari_matchcode.h"
#include "libkawari/wordcollection.h"
#include "libkawari/kawari_dict.h"
//---------------------------------------------------------------------------
class TNS_KawariMatchDictionary {
private:

	// マッチエントリID
	typedef unsigned int TMatchID;

	// すべてのマッチエントリ名を保持する
//	TWordCollection<TKawariMatch_base*,TKawariMatch_baseP_Less> MatchCollection;
	TWordPointerCollection<TKawariMatch_base,TKawariMatch_baseP_Less> MatchCollection;

	// マッチエントリとエントリの対応表
	map<TMatchID,TEntryID> MatchToEntry;
	map<TEntryID,TMatchID> EntryToMatch;

	// 辞書
	TNS_KawariDictionary *Dictionary;

	// エンジン
	class TKawariEngine *Engine;

public:

	TNS_KawariMatchDictionary(TNS_KawariDictionary *dictionary,class TKawariEngine *engine);
//	TNS_KawariMatchDictionary(TNS_KawariDictionary *dictionary,class TKawariEngine *engine)
//	 : Dictionary(dictionary),Engine(engine)
//	{
//		MatchToEntry[0]=0;
//		EntryToMatch[0]=0;
//	}

	// マッチエントリ数を取得
	unsigned int Size(void) const;

	// マッチエントリのエントリID取得
	// 戻り値 : 1オリジン、見つからなければ0を返す
	TEntryID GetMatchEntryID(TKawariMatch_base* match) const;

	// マッチエントリを生成する
	// 既にマッチエントリが存在する場合は、生成せずにIDを返す
	// 戻り値 : 生成したエントリのID
	TEntryID CreateMatchEntry(TKawariMatch_base* match);

	// 発動条件がそろったマッチエントリを全て列挙
	// 戻り値 : 発動条件がそろったマッチエントリの個数
	unsigned int Evaluation(vector<TEntryID> &entrycol);

};
//---------------------------------------------------------------------------
// コンストラクタ
inline TNS_KawariMatchDictionary::TNS_KawariMatchDictionary(TNS_KawariDictionary *dictionary,class TKawariEngine *engine)
{
	Dictionary=dictionary;
	Engine=engine;
	MatchToEntry[0]=0;
	EntryToMatch[0]=0;
}
//---------------------------------------------------------------------------
// マッチエントリ数を取得
inline unsigned int TNS_KawariMatchDictionary::Size(void) const
{
	return(MatchCollection.Size());
}
//---------------------------------------------------------------------------
// マッチエントリのエントリID取得
inline TEntryID TNS_KawariMatchDictionary::GetMatchEntryID(TKawariMatch_base* match) const
{
	return(MatchToEntry.find(MatchCollection.Find(match))->second);
}
//---------------------------------------------------------------------------
#endif

