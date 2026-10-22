//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// マッチエントリ中間コード
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.06.09  Phase 5.3     マッチエントリ再実装
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <cstdlib>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_matchcode.h"
#include "libkawari/kawari_engine.h"
//---------------------------------------------------------------------------
// マッチエントリ中間コードの基底クラス
ostream& TKawariMatch_base::DebugIndent(ostream& os,unsigned int level) const
{
	for(unsigned int i=0;i<level;i++) os << "    ";
	return(os);
}
//---------------------------------------------------------------------------
// 指定エントリ中に指定文字列が全て含まれていれば真
bool TKawariMatchFind::Evaluation(TKawariEngine *engine)
{
	string sentence=engine->FindFirst(EntryName);

	for(unsigned int i=0;i<KeyWord.size();i++) {
		if(sentence.find(KeyWord[i])==string::npos) return(false);
	}

	return(true);
}
//---------------------------------------------------------------------------
void TKawariMatchFind::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "Find:";
	os << EntryName << " , ";
	for(unsigned int i=0;i<KeyWord.size()-1;i++) os << KeyWord[i] << "&";
	os << KeyWord.back() << endl;

	return;
}
//---------------------------------------------------------------------------
bool TKawariMatchFind::Less(const TKawariMatch_base& R_) const
{
	const TKawariMatchFind& R=dynamic_cast<const TKawariMatchFind&>(R_);

	if(EntryName!=R.EntryName) return(EntryName<R.EntryName);

	if(KeyWord.size()!=R.KeyWord.size()) return(KeyWord.size()<R.KeyWord.size());

	for(unsigned int i=0;i<KeyWord.size();i++) {
		if(KeyWord[i]<R.KeyWord[i]) return(true);
		if(KeyWord[i]>R.KeyWord[i]) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
// AND
bool TKawariMatchAnd::Evaluation(TKawariEngine *engine)
{
	for(unsigned int i=0;i<Match.size();i++) {
		if(!Match[i]->Evaluation(engine)) return(false);
	}

	return(true);
}
//---------------------------------------------------------------------------
void TKawariMatchAnd::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "AND:" << endl;
	for(unsigned int i=0;i<Match.size();i++) Match[i]->Debug(os,level+1);

	return;
}
//---------------------------------------------------------------------------
bool TKawariMatchAnd::Less(const TKawariMatch_base& R_) const
{
	const TKawariMatchAnd& R=dynamic_cast<const TKawariMatchAnd&>(R_);

	if(Match.size()!=R.Match.size()) return(Match.size()<R.Match.size());

	for(unsigned int i=0;i<Match.size();i++) {
		TKawariMatch_base *l=Match[i];
		TKawariMatch_base *r=R.Match[i];

		if(TKawariMatch_baseP_Less()(l,r)) return(true);
		if(TKawariMatch_baseP_Less()(r,l)) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
TKawariMatchAnd::~TKawariMatchAnd()
{
	for(unsigned int i=0;i<Match.size();i++) delete Match[i];
}
//---------------------------------------------------------------------------
// OR
bool TKawariMatchOr::Evaluation(TKawariEngine *engine)
{
	for(unsigned int i=0;i<Match.size();i++) {
		if(Match[i]->Evaluation(engine)) return(true);
	}

	return(false);
}
//---------------------------------------------------------------------------
void TKawariMatchOr::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "OR:" << endl;
	for(unsigned int i=0;i<Match.size();i++) Match[i]->Debug(os,level+1);

	return;
}
//---------------------------------------------------------------------------
bool TKawariMatchOr::Less(const TKawariMatch_base& R_) const
{
	const TKawariMatchOr& R=dynamic_cast<const TKawariMatchOr&>(R_);

	if(Match.size()!=R.Match.size()) return(Match.size()<R.Match.size());

	for(unsigned int i=0;i<Match.size();i++) {
		TKawariMatch_base *l=Match[i];
		TKawariMatch_base *r=R.Match[i];

		if(TKawariMatch_baseP_Less()(l,r)) return(true);
		if(TKawariMatch_baseP_Less()(r,l)) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
TKawariMatchOr::~TKawariMatchOr()
{
	for(unsigned int i=0;i<Match.size();i++) delete Match[i];
}
//---------------------------------------------------------------------------
