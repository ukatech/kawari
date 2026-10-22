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
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/sequence_gen.h"
//---------------------------------------------------------------------------
#include <strstream>
using namespace std;
//---------------------------------------------------------------------------
TSequenceGenerator SequenceGenerator;
//---------------------------------------------------------------------------
string TSequenceGenerator::GetString(const string& key,int width)
{
	unsigned int seq=Get(key);
	int w=1;
	for(unsigned int num=seq;num>=10;num=num/10) w++;

	if(width<w) width=w;

	ostrstream buffer;
	buffer << key;
	buffer.fill('0');
	buffer.width(width);
	buffer << seq;
	buffer << ends;

	string retstr(buffer.str());
	buffer.freeze(false);
	return(retstr);
}
//---------------------------------------------------------------------------
