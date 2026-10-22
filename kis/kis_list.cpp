//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- リスト --
//
//      Programed by Kouji.U (Suikyo)
//
//  2001.05.16  Phase 0.01    First version
//
//---------------------------------------------------------------------------
#include "libkawari/kawari_dict.h"
#include "kis/kis_list.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <ctime>
#include <strstream>
#include <iomanip>
using namespace std;
//---------------------------------------------------------------------------
string KIS_shift::Function(const vector<string>& args)
{
  string ret;
  if(args.size()>=2){
	vector<string> wordcol;
	KawariDictionary->FindAll(wordcol, args[1]);
	if (wordcol.size()){
	  vector<string>::iterator it=wordcol.begin();
	  ret = (*it);
	  KawariDictionary->DeleteEntry(args[1]);
	  it++;
	  for(;it!=wordcol.end();it++){
		KawariDictionary->Insert(args[1], (*it));
	  }
	}
  }
  return ret;
}

//---------------------------------------------------------------------------
string KIS_unshift::Function(const vector<string>& args)
{
  if (args.size()>=3){
	vector<string> wordcol;
	KawariDictionary->FindAll(wordcol, args[1]);
	KawariDictionary->InsertAfterDelete(args[1], args[2]);
	for(vector<string>::iterator it=wordcol.begin();it!=wordcol.end();it++){
	  KawariDictionary->Insert(args[1], (*it));
	}
  }
  return ("");
}

//---------------------------------------------------------------------------
string KIS_pop::Function(const vector<string>& args)
{
  string ret;
  if(args.size()>=2){
	vector<string> wordcol;
	KawariDictionary->FindAll(wordcol, args[1]);
	KawariDictionary->DeleteEntry(args[1]);
	int size = wordcol.size();
	if (size){
	  for(vector<string>::iterator it=wordcol.begin();;){
		ret = (*it);
		it++;
		if ( it!=wordcol.end() ){
		  KawariDictionary->Insert(args[1], ret);
		}else{
		  break;
		}
	  }
	}
  }
  return ret;
}

