//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 辞書
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.04.21  Phase 0.50a1   辞書管理のクラス化
//  2001.05.03  Phase 0.50a4  インラインスクリプト
//  2001.05.26  Phase 5.1     インタープリタ・コンパイラ化
//                            API整理
//  2001.06.17  Phase 6.0     複数エントリへの同時追加のバグ修正
//
//---------------------------------------------------------------------------
#ifndef KAWARI_DICT_H
#define KAWARI_DICT_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
#include <set>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_code.h"
#include "libkawari/wordcollection.h"
//---------------------------------------------------------------------------
// 単語ID
typedef unsigned int TWordID;
//---------------------------------------------------------------------------
// エントリID
typedef unsigned int TEntryID;
//---------------------------------------------------------------------------
class TNS_KawariDictionary {
private:

	// すべての単語を保持する
	TWordPointerCollection<TKawariCode_base,TKawariCode_baseP_Less> WordCollection;

	// すべてのエントリ名を保持する
	TWordCollection<string,less<string> > EntryCollection;

	// 純粋仮想単語(PVW:PureVirtualWord)とエントリの対応表
	map<TWordID,TEntryID> PVWToEntry;
	map<TEntryID,TWordID> EntryToPVW;

	// 辞書
	map<TEntryID,vector<TWordID> > Dictionary;

	// 逆引き辞書
	map<TWordID,multiset<TEntryID> > ReverseDictionary;


public:

	~TNS_KawariDictionary (){
	}
	// 総単語数を取得
	unsigned int WordCollectionSize(void) const;

	// 有効エントリ数を取得
	unsigned int EntryCollectionSize(void) const;

	// 文字列から中間コードツリーを生成する
	TKawariCode_base *StringToCode(const string& word) const;


	// ID変換系API

	// 単語ID取得
	// 戻り値 : 1オリジン、見つからなければ0を返す
	TWordID GetWordID(TKawariCode_base* word) const;
	TWordID GetWordID(const string& word) const;

	// IDから単語に変換
	// 戻り値 : 中間コードツリー
	TKawariCode_base *GetWordFromID(TWordID id) const;

	// エントリID取得
	// 戻り値 : 1オリジン、見つからなければ0を返す
	TEntryID GetEntryID(const string& entry) const;


	// 辞書追加・削除系API

	// エントリを生成する
	// 既にエントリが存在する場合は、生成せずにIDを返す
	// 戻り値 : 生成したエントリのID
	TEntryID CreateEntry(const string& entry);

	// 指定されたエントリを空にする
	// メモリに空エントリと単語が残る
	// 戻り値 : 成功でtrue
	bool ClearEntry(TEntryID entry);

	// 辞書への単語の追加
	// 既に単語が存在する場合でも、さらに追加する (現在の仕様)
	// 既に単語が存在する場合には、出現確率を変更する (将来の仕様)
	// 注意・既に単語IDを持つ単語であった場合、deleteされる
	// 戻り値 : 追加した単語のID
	TWordID Insert(TEntryID entry,TKawariCode_base* word);
	TWordID Insert(TEntryID entry,const string& word);
	TWordID Insert(TEntryID entry,TWordID id);

	// 指定されたエントリを空にしてから単語を追加
	// 注意・既に単語IDを持つ単語であった場合、deleteされる
	// エントリを変数的に利用する時に使用する
	// 戻り値 : 追加した単語のID
	TWordID InsertAfterClear(TEntryID entry,TKawariCode_base* word);
	TWordID InsertAfterClear(TEntryID entry,const string& word);


	// 検索系API

	// 指定されたエントリの先頭の単語を返す
	// 戻り値 : 単語のID
	TWordID FindFirst(TEntryID entry) const;

	// 指定されたエントリの単語を全て列挙
	// 戻り値 : 単語の個数
	unsigned int FindAll(TEntryID entry,vector<TWordID> &wordcol) const;

	// 指定されたエントリ全てに含まれる単語を
	// 純粋仮想単語「${エントリ名}」のみ展開して再帰的に列挙する
	// 戻り値 : 単語の個数
	unsigned int GetWordCollection(TEntryID entry,set<TWordID> &wordcol) const;
	unsigned int GetWordCollection(const vector<TEntryID>& entrytable,set<TWordID> &wordcol) const;

};
//---------------------------------------------------------------------------
// 総単語数を取得
inline unsigned int TNS_KawariDictionary::WordCollectionSize(void) const
{
	return(ReverseDictionary.size());
}
//---------------------------------------------------------------------------
// 有効エントリ数を取得
inline unsigned int TNS_KawariDictionary::EntryCollectionSize(void) const
{
	return(Dictionary.size());
}
//---------------------------------------------------------------------------
// 単語ID取得
inline TWordID TNS_KawariDictionary::GetWordID(TKawariCode_base* word) const
{
	return(WordCollection.Find(word));
}
//---------------------------------------------------------------------------
// エントリID取得
inline TEntryID TNS_KawariDictionary::GetEntryID(const string& entry) const
{
	return(EntryCollection.Find(entry));
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にしてから単語を追加
inline TWordID TNS_KawariDictionary::InsertAfterClear(TEntryID entry,TKawariCode_base* word)
{
	ClearEntry(entry);
	return(Insert(entry,word));
}
//---------------------------------------------------------------------------
#endif
