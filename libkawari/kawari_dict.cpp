//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 辞書
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.04.21  Phase 0.50a1  辞書管理のクラス化
//  2001.05.03  Phase 0.50a4  インラインスクリプト
//  2001.05.26  Phase 5.1     インタープリタ・コンパイラ化
//                            API整理
//  2001.06.17  Phase 5.4     複数エントリへの同時追加のバグ修正
//                            逆コンパイラ
//  2001.12.16  Phase 7.2     ClearEntry(TEntryID entry)のバグ修正
//                            (Thanks: しの)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_dict.h"
//---------------------------------------------------------------------------
//#include <algorithm>
//using namespace std;
//---------------------------------------------------------------------------
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 文字列から中間コードツリーを生成する
TKawariCode_base *TNS_KawariDictionary::StringToCode(const string& word) const
{
	TKawariLexer buff(word);

	vector<TKawariCode_base*> codelist;

	if(!TKawariCodeWord::Compile2(codelist,buff)) return(0);

	if(codelist.size()==1) return(codelist[0]);

	TKawariCodeWord *code=new TKawariCodeWord;
	code->Add(codelist);

	return(code);
}
//---------------------------------------------------------------------------
// 単語ID取得
// 戻り値 : 1オリジン、見つからなければ0を返す
TWordID TNS_KawariDictionary::GetWordID(const string& word) const
{
	TKawariCode_base *code=StringToCode(word);
	TWordID id=GetWordID(code);
	delete code;

	return(id);
}
//---------------------------------------------------------------------------
// IDから単語に変換
// 戻り値 : 中間コードツリー
TKawariCode_base *TNS_KawariDictionary::GetWordFromID(TWordID id) const
{
	TKawariCode_base *const*word=WordCollection.Find(id);

	return(word?(*word):NULL);
}
//---------------------------------------------------------------------------
// エントリを生成する
// 既にエントリが存在する場合は、生成せずにIDを返す
// 戻り値 : 生成したエントリのID
TEntryID TNS_KawariDictionary::CreateEntry(const string& entry)
{
	TEntryID id=0;
	EntryCollection.Insert(entry,&id);

	return(id);
}
//---------------------------------------------------------------------------
// 辞書への単語の追加
// 既に単語が存在する場合でも、さらに追加する (現在の仕様)
// 既に単語が存在する場合には、出現確率を変更する (将来の仕様)
// 戻り値 : 追加した単語のID
TWordID TNS_KawariDictionary::Insert(TEntryID entry,TKawariCode_base* word)
{
	if(!word) return(0);

	// エントリ内の単語はコンテキストを持つ
	word=word->CreateContext();

	TWordID id=0;
	if(!WordCollection.Insert(word,&id)) {
		// 既に登録済みの単語だった
		delete word;
		word=GetWordFromID(id);
	}

	TKawariCodeEntryCall *macro=dynamic_cast<TKawariCodeEntryCall *>(word);
	if(macro) {
		// 追加するのは純粋仮想単語
		TEntryID child=CreateEntry(macro->Get());

		PVWToEntry[id]=child;
		EntryToPVW[child]=id;
	}

	Dictionary[entry].push_back(id);
	ReverseDictionary[id].insert(entry);

	return(id);
}
//---------------------------------------------------------------------------
TWordID TNS_KawariDictionary::Insert(TEntryID entry,const string& word)
{
	TKawariCode_base *code=StringToCode(word);
	TWordID id=Insert(entry,code);

	return(id);
}
//---------------------------------------------------------------------------
TWordID TNS_KawariDictionary::Insert(TEntryID entry,TWordID id)
{
	if(id==0) return(0);

	TKawariCode_base* word=GetWordFromID(id);
	if(word==NULL) return(0);

	TKawariCodeEntryCall *macro=dynamic_cast<TKawariCodeEntryCall *>(word);
	if(macro) {
		// 追加するのは純粋仮想単語
		TEntryID child=CreateEntry(macro->Get());

		PVWToEntry[id]=child;
		EntryToPVW[child]=id;
	}

	Dictionary[entry].push_back(id);
	ReverseDictionary[id].insert(entry);

	return(id);
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にする
// メモリに空エントリと単語が残る
// 戻り値 : 成功でtrue
bool TNS_KawariDictionary::ClearEntry(TEntryID entry)
{
	if(Dictionary.count(entry)==0) return(false);

	// 逆引き辞書の消去
	for(unsigned int i=0;i<Dictionary[entry].size();i++) {
		unsigned int id=Dictionary[entry][i];
		ReverseDictionary[id].erase(
		 ReverseDictionary[id].lower_bound(entry));
	}

//	Dictionary[entry].clear();
	Dictionary.erase(entry);

	return(true);
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にしてから単語を追加
// エントリを変数的に利用する時に使用する
// 戻り値 : 追加した単語のID
TWordID TNS_KawariDictionary::InsertAfterClear(TEntryID entry,const string& word)
{
	TKawariCode_base *code=StringToCode(word);
	TWordID id=InsertAfterClear(entry,code);

	return(id);
}
//---------------------------------------------------------------------------
// 指定されたエントリの先頭の単語を返す
// 戻り値 : 単語のID
TWordID TNS_KawariDictionary::FindFirst(TEntryID entry) const
{
	if(Dictionary.count(entry)==0) return(0);

	return(Dictionary.find(entry)->second[0]);
}
//---------------------------------------------------------------------------
// 指定されたエントリの単語を全て列挙
// 戻り値 : 単語の個数
unsigned int TNS_KawariDictionary::FindAll(TEntryID entry,vector<TWordID> &wordcol) const
{
	if(Dictionary.count(entry)==0) return(0);

	map<TEntryID,vector<TWordID> >::const_iterator it=Dictionary.find(entry);
	wordcol.insert(wordcol.end(),it->second.begin(),it->second.end());

	return(it->second.size());
}
//---------------------------------------------------------------------------
// 指定されたエントリ全てに含まれる単語を
// 純粋仮想単語「${エントリ名}」のみ展開して再帰的に列挙する
// 戻り値 : 単語の個数
unsigned int TNS_KawariDictionary::GetWordCollection(TEntryID entry,set<TWordID> &wordcol) const
{
	if(Dictionary.count(entry)==0) return(0);


	// 過去に検索済みのエントリ
	set<TEntryID> donelist;
	// 検索予定のエントリ
	vector<TEntryID> parselist;

	parselist.push_back(entry);

	while(parselist.size()) {
		TEntryID entryid=parselist.back();
		parselist.pop_back();
		if(donelist.count(entryid)) continue;

		donelist.insert(entryid);

		if(Dictionary.count(entryid)==0) continue;

		map<TEntryID,vector<TWordID> >::const_iterator it=Dictionary.find(entryid);
		for(unsigned int i=0;i<it->second.size();i++) {
			TWordID id=it->second[i];
			if(PVWToEntry.count(id)==0) {
				wordcol.insert(id);
			} else {
				// 純粋仮想単語
				TEntryID child=PVWToEntry.find(id)->second;
				if(donelist.count(child)==0) parselist.push_back(child);
			}
		}
	}

	return(wordcol.size());
}
//---------------------------------------------------------------------------
unsigned int TNS_KawariDictionary::GetWordCollection(const vector<TEntryID>& entrytable,set<TWordID> &wordcol) const
{
	if(entrytable.size()==0) return(0);

	if(entrytable.size()==1) return(GetWordCollection(entrytable[0],wordcol));

	set<TWordID> collection[3];

	GetWordCollection(entrytable[0],collection[0]);

	for(unsigned int no=1;no<entrytable.size();no++) {
		unsigned int pre=(no*2-2)%3;
		unsigned int current=(no*2-1)%3;
		unsigned int next=(no*2)%3;
		collection[current].clear();
		GetWordCollection(entrytable[no],collection[current]);

		// 以下、Borland C++ 5.5.1のset_intersectionがバグ持ちなので(iterator周りのバグ)

		set<TWordID>::iterator first1=collection[pre].begin();
		set<TWordID>::iterator first2=collection[current].begin();
		set<TWordID>::iterator last1=collection[pre].end();
		set<TWordID>::iterator last2=collection[current].end();
		set<TWordID> *result;

		if(no<(entrytable.size()-1)) {
			result=&collection[next];
		} else {
			result=&wordcol;
		}

		result->clear();
		while (first1 != last1 && first2 != last2) {
			if (*first1 < *first2) ++first1;
			 else if (*first2 < *first1) ++first2;
			 else {
				result->insert(*first1);
				++first1;
				++first2;
			}
		}
	}

	return(wordcol.size());
}
//---------------------------------------------------------------------------

