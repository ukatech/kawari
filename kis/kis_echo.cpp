//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- エコー --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.25  Phase 5.1     First version
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_echo.h"
//---------------------------------------------------------------------------
//#include <iostream>
//using namespace std;
//---------------------------------------------------------------------------
string KIS_echo::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	string retstr=args[1];

	for(unsigned int i=2;i<args.size();i++) retstr+=string(" ")+args[i];

//	cout << retstr << endl;

	return(retstr);
}
//---------------------------------------------------------------------------

