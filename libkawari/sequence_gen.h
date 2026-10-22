//---------------------------------------------------------------------------
//
//  sequence_gen -- sequence_number_generator --
//
//      Programed by NAKAUE.T
//
//  2001.01.12  revision 1    version 0.1.1
//  2001.08.08  revision 2    ostrstreamバグ修正
//                            ついでにスペルミス修正
//
//---------------------------------------------------------------------------
#ifndef SEQUENCE_GEN_H
#define SEQUENCE_GEN_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <map>
#include <string>
using namespace std;
//---------------------------------------------------------------------------
class TSequenceGenerator {
private:

	map<string,unsigned int> SequenceTable;

public:

    TSequenceGenerator (){
    }
    ~TSequenceGenerator (){
    }
    
	unsigned int Set(const string& key,unsigned int val)
	{
		return(SequenceTable[key]=val);
	}

	unsigned int Reset(const string& key)
	{
		return(Set(key,0));
	}

	unsigned int Get(const string& key)
	{
		if(SequenceTable.count(key)==0) Reset(key);

		unsigned int seq=SequenceTable[key];
		SequenceTable[key]=SequenceTable[key]+1;
		return(seq);
	}

	string GetString(const string& key,int width=0);

	void Delete(const string& key)
	{
		SequenceTable.erase(SequenceTable.find(key));
	}

};
extern TSequenceGenerator SequenceGenerator;
//---------------------------------------------------------------------------
#endif
