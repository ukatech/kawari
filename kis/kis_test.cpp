//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 条件式 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.30  Phase 5.1     First version
//  2002.01.07  Phase 7.3     &&,||,! 演算子、単項の評価追加 (さとー)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_test.h"
//---------------------------------------------------------------------------
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
static bool IsTrue(const string &str)
{
	return(!((str=="")||(str=="0")||(str=="false")));
}

static string TestFunction(const vector<string>& args)
{
	bool status=false;

	switch(args.size()) {
		case 2:
			status=IsTrue(args[1]);
			break;
		case 3:
			if(args[1]=="!") {
				status=(!IsTrue(args[2]));
			}
			break;
		case 4:
			if((args[2]=="==")||(args[2]=="=")) {
				status=(args[1]==args[3]);
			} else if(args[2]=="!=") {
				status=(args[1]!=args[3]);
			} else if(args[2]=="<") {
				status=(args[1]<args[3]);
			} else if(args[2]==">") {
				status=(args[1]>args[3]);
			} else if(args[2]=="<=") {
				status=(args[1]<=args[3]);
			} else if(args[2]==">=") {
				status=(args[1]>=args[3]);
			} else if(args[2]=="-eq") {
				status=(atoi(args[1].c_str())==atoi(args[3].c_str()));
			} else if(args[2]=="-ne") {
				status=(atoi(args[1].c_str())!=atoi(args[3].c_str()));
			} else if(args[2]=="-lt") {
				status=(atoi(args[1].c_str())<atoi(args[3].c_str()));
			} else if(args[2]=="-gt") {
				status=(atoi(args[1].c_str())>atoi(args[3].c_str()));
			} else if(args[2]=="-le") {
				status=(atoi(args[1].c_str())<=atoi(args[3].c_str()));
			} else if(args[2]=="-ge") {
				status=(atoi(args[1].c_str())>=atoi(args[3].c_str()));
			} else if(args[2]=="&&") {
				status=(IsTrue(args[1])&&IsTrue(args[3]));
			} else if(args[2]=="||") {
				status=(IsTrue(args[1])||IsTrue(args[3]));
			}
			break;
		default:
			break;
	}

	return(status?"true":"false");
}
//---------------------------------------------------------------------------
string KIS_test::Function(const vector<string>& args)
{
	return(TestFunction(args));
}
//---------------------------------------------------------------------------
string KIS_test2::Function(const vector<string>& args)
{
	vector<string> args2;
	args2.insert(args2.end(),args.begin(),args.end());
	if(args2.back()=="]") args2.pop_back();

	return(TestFunction(args2));
}
//---------------------------------------------------------------------------

