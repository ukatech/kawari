//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// データ保持クラス
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.04.24  Phase 0.5     導入
//  2001.05.26  Phase 5.1     インタープリタ・コンパイラ導入に伴いtemplate化
//
//---------------------------------------------------------------------------
#ifndef WORDCOLLECTION_H
#define WORDCOLLECTION_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
// データを保持しIDを割り振るクラス
template<class T,class Compare>
class TWordCollection {
protected:

	// 全てのデータ
	// NULLは抹消されたデータ
	vector<T> WordList;

	// データとIDの対応表
	// 0は抹消されたデータ
	typedef map<T,unsigned int,Compare> TWordIDMap;
	TWordIDMap WordIDMap;

public:

	// 総単語数を取得
	unsigned int Size(void) const
	{
		return(WordList.size());
	}

	// データの追加
	// 成功でtrue,IDは1オリジン
	bool Insert(const T& word,unsigned int *id_=NULL);

	// データの抹消
//	bool Delete(unsigned int id);


	// ID->データ
	const T* Find(unsigned int id) const;

	// データ->ID
	unsigned int Find(const T& word) const;

};

//---------------------------------------------------------------------------
// ポインタを保持しIDを割り振るクラス
template<class T,class Compare>
class TWordPointerCollection : public TWordCollection<T*, Compare>{
public:
    ~TWordPointerCollection (){
        for (vector<T*>::iterator it=WordList.begin(); it < WordList.end(); it++){
            delete (*it);
        }
    }
};
//---------------------------------------------------------------------------
// データの追加
// 戻り値はID (1オリジン)
template<class T,class Compare>
bool TWordCollection<T,Compare>::Insert(const T& word,unsigned int *id_)
{
	unsigned int id=Find(word);
	if(id_) *id_=id;

	if(id!=0) return(false);

	WordList.push_back(word);
	id=WordList.size();

	WordIDMap[word]=id;

	if(id_) *id_=id;

	return(true);
}
//---------------------------------------------------------------------------
// ID->データ
template<class T,class Compare>
 const T* TWordCollection<T,Compare>::Find(unsigned int id) const
{
	if((id==0)||(WordList.size()<=(id-1))) return(NULL);

	return(&(WordList[id-1]));
}
//---------------------------------------------------------------------------
// データ->ID
template<class T,class Compare>
 unsigned int TWordCollection<T,Compare>::Find(const T& word) const
{
	TWordIDMap::const_iterator it=WordIDMap.find(word);

	return((it!=WordIDMap.end())?it->second:0);
}
//---------------------------------------------------------------------------
class TStringP_Less {
public:
	bool operator ()(const string *l,const string *r) const
	{
		return((*l)<(*r));
	}
};
//---------------------------------------------------------------------------
#endif
