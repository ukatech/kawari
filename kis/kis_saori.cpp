//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- SAORI --
//
//  2001.03.28 initial
//  2003.02.16  Phase 7.5.0   CharsetをI18N対応
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_saori.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
//#include "libkawari/kawari_log.h"
#include "misc/misc.h"
#include "misc/phttp.h"
#include "misc/i18n.h"
//using namespace kawari_log;
//---------------------------------------------------------------------------
#include <cstdlib>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
string KIS_saoriregist::Function(const vector<string>& args)
{
	if(args.size()!=3 && args.size()!=4) return ("");

	TKawariEngine::SAORILOADTYPE loadopt=TKawariEngine::LOADONCALL;
	if (args.size()>=4){
		if (args[3]=="preload")
			loadopt=TKawariEngine::PRELOAD;
		else if (args[3]=="noresident")
			loadopt=TKawariEngine::NORESIDENT;
	}
	KisEngine->Engine()->RegisterSAORIModule(args[2], KisEngine->Engine()->GetDataPath()+args[1], loadopt);

	return ("");
}
//---------------------------------------------------------------------------
string KIS_saorierase::Function(const vector<string>& args)
{
	if(args.size()!=2) return ("");
	KisEngine->Engine()->EraseSAORIModule(args[1]);
	return ("");
}
//---------------------------------------------------------------------------
bool KIS_callsaori::CallSaori(
	const string& saoriname, const vector<string>& args, TPHMessage &response)
{
	// リクエスト生成
	TPHMessage request;
	request.SetStartline("EXECUTE SAORI/1.0");
	request["Sender"]="kawari";
	request["Charset"]=lang.getShioriCharset();
	string slevel=KisEngine->Engine()->FindFirst("system.Sender.Path");
	// shiori.dllから動かした場合、Sender.Pathは必ず設定される。
	// そうでないのは幸水。だからLocalで良い。
	request["SecurityLevel"]=(slevel.size())?
		((slevel=="local")? string("Local") : slevel): string("Local");
	for (unsigned int i=0; i<args.size(); i++)
		request["Argument"+IntToString(i)]=args[i];

	// リクエスト
	if ((!KisEngine->Engine()->RequestToSAORIModule(saoriname, request, response))
		||(!response.GetStartline().size())){
		// Failed
		//Logger.GetStream(LOG_ERROR) << "Calling SAORI (" << saoriname << ") failed." << endl;
		return false;
	}

	// ステータスラインの分解
	const string &statusline=response.GetStartline();
	unsigned int pos=statusline.find(' ');
	if (pos==string::npos) return false;
	unsigned int npos=statusline.find(' ', pos+1);
	string statuscode=statusline.substr(pos+1, npos-pos-1);

	if (!(statuscode[0]=='2')) return false;
	return true;
}
//---------------------------------------------------------------------------
string KIS_callsaori::Function(const vector<string>& args)
{
	if(args.size()<2) return ("");

	const string saoriname=args[1];
	vector<string> saori_args;
	for (unsigned int i=2; i<args.size(); i++)
		saori_args.push_back(args[i]);

	TPHMessage response;

	if (CallSaori(saoriname, saori_args, response)){
		if (response.count("Result"))
			return response["Result"];
		else
			return ("");
	}else{
		return ("");
	}
}

//---------------------------------------------------------------------------
string KIS_callsaorix::Function(const vector<string>& args)
{
	if(args.size()<3) return ("");

	const string saoriname=args[1];
	const string stem=args[2];
	vector<string> saori_args;
	for (unsigned int i=3; i<args.size(); i++)
		saori_args.push_back(args[i]);

	TPHMessage response;

	if (CallSaori(saoriname, saori_args, response)){
		if (stem.size()){
			string stemp=stem+'.';
			int value_max=0;
			for (TPHMessage::iterator it=response.begin(); it!=response.end(); it++){
				string key=it->first;
				if (key.find("Value")==0){
					int num=atoi(key.c_str()+5)+1;
					if (num>value_max) value_max=num;
				}
				KisEngine->Engine()->InsertAfterClear(stemp+key, it->second);
			}
			KisEngine->Engine()->InsertAfterClear(stemp+"size", IntToString(value_max));
			KisEngine->Engine()->InsertAfterClear(stem, response.GetStartline());
		}
		if (response.count("Result"))
			return response["Result"];
		else
			return ("");
	}else{
		return ("");
	}
}

//---------------------------------------------------------------------------
