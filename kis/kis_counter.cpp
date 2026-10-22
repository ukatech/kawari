//---------------------------------------------------------------------------
//
// "‰Ø˜a—œ" for ‚ ‚êˆÈŠO‚Ì‰½‚©ˆÈŠO‚Ì‰½‚©
//  2001.05.30  Phase 5.1     First version
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.25  Phase 5.1     First version
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_counter.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
string KIS_inc::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	int diff=1;
	if(args.size()>=3) {
		diff=atoi(args[2].c_str());
	}

	int counter=atoi(KisEngine->Engine()->FindFirst(args[1]).c_str())+diff;

	if(args.size()>=4) {
		int limit=atoi(args[3].c_str());
		if(counter>limit) counter=limit;
	}

	KisEngine->Engine()->InsertAfterClear(args[1],IntToString(counter));

	return("");
}
//---------------------------------------------------------------------------
string KIS_dec::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	int diff=1;
	if(args.size()>=3) {
		diff=atoi(args[2].c_str());
	}

	int counter=atoi(KisEngine->Engine()->FindFirst(args[1]).c_str())-diff;

	if(args.size()>=4) {
		int limit=atoi(args[3].c_str());
		if(counter<limit) counter=limit;
	}

	KisEngine->Engine()->InsertAfterClear(args[1],IntToString(counter));

	return("");
}
//---------------------------------------------------------------------------

