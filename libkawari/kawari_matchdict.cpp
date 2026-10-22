//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// マッチエントリ辞書
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.06.09  Phase 5.3     マッチエントリ再実装
//  2001.06.17  Phase 6.0     マッチエントリの最後を読まないバグ修正
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
//using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_matchdict.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// マッチエントリを生成する
// 既にマッチエントリが存在する場合は、生成せずにIDを返す
// 戻り値 : 生成したエントリのID
TEntryID TNS_KawariMatchDictionary::CreateMatchEntry(TKawariMatch_base* match)
{
	if(!match) return(0);

	TMatchID matchid=0;
	if(!MatchCollection.Insert(match,&matchid)) {
		// 既に登録済みのマッチエントリだった
		delete match;
		TKawariMatch_base *const*m=MatchCollection.Find(matchid);
		match=(m?*m:NULL);
	}

	TEntryID id=Dictionary->CreateEntry(string("system.match")+IntToString(matchid));

	MatchToEntry[matchid]=id;
	EntryToMatch[id]=matchid;

	return(id);
}
//---------------------------------------------------------------------------
// 発動条件がそろったマッチエントリを全て列挙
// 戻り値 : 発動条件がそろったマッチエントリの個数
unsigned int TNS_KawariMatchDictionary::Evaluation(vector<TEntryID> &entrycol)
{
	entrycol.clear();

	// TWordCollectionの列挙方法は再考の余地あり
	// TMatchIDは1オリジン
	for(unsigned int matchid=1;matchid<=MatchCollection.Size();matchid++) {
		TKawariMatch_base *const*match=MatchCollection.Find(matchid);
		if(!match) continue;

		if((*match)->Evaluation(Engine)) {
			entrycol.push_back(MatchToEntry[matchid]);
		}
	}

	return(entrycol.size());
}
//---------------------------------------------------------------------------

