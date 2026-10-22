//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(汎用)
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//  2003.02.16  Phase 7.5.0   Charset対応
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "saori/saori.h"
#include "misc/i18n.h"
#ifdef __KAWARI__
#	include "libkawari/kawari_log.h"
using namespace kawari_log;
#endif
//---------------------------------------------------------------------------
#include <strstream>
using namespace std;
//---------------------------------------------------------------------------
TSaoriModule::TSaoriModule(const string &path, const LOADTYPE t) : libpath(path), type(t), bind(NULL)
{
	if (type==TSaoriModule::PRELOAD)
		Attatch();
}
//---------------------------------------------------------------------------
// SAORI/1.0 request
bool TSaoriModule::Request(const TPHMessage &request, TPHMessage &response)
{
	if ((type!=PRELOAD)&&(!bind))
		Attatch();
	if (!bind)
		return false;

#ifdef __KAWARI__
	if (Logger.Check(LOG_INFO)){
		Logger.GetStream() << "[SAORI] Query to ("+libpath+")" << endl
			<< "---------------------- REQUEST" << endl;
		request.Dump(Logger.GetStream());
	}
#endif

	// シリアライズ
	string reqstr=request.Serialize();

	// Request
	string resstr=bind->Request(reqstr);

	// デシリアライズ
	response.Deserialize(resstr);

#ifdef __KAWARI__
	if (Logger.Check(LOG_INFO)){
		Logger.GetStream() << "----------------------RESPONSE" << endl;
		response.Dump(Logger.GetStream());
		Logger.GetStream() << "[SAORI] Query end." << endl;
	}
#endif

	if (type==NORESIDENT)
		Detatch();

	return true;
}
//---------------------------------------------------------------------------
void TSaoriModule::Attatch(void){
	if (bind) return;
	bind=TSaoriBinding::Bind(libpath);

	if (bind&&(bind->GetLoadCount()==1)){
		// はじめてのぷらぐいん
		TPHMessage request, response;

		request.SetStartline("GET Version SAORI/1.0");
		request["Charset"]=lang.getShioriCharset();
		request["Sender"]="kawari";

		if (!Request(request, response)
			|| !(response.GetStartline().find("SAORI/1.")==0))
			Detatch();
	}
}
//---------------------------------------------------------------------------
void TSaoriModule::Detatch(void){
	if (bind){
		delete bind;
		bind=NULL;
	}
}
//---------------------------------------------------------------------------
TSaoriModule::~TSaoriModule(){
	Detatch();
}
//---------------------------------------------------------------------------
// モジュールの登録
void TSaoriPark::RegisterModule(const string &aliasname, const string &path, const TSaoriModule::LOADTYPE type){
	TSaoriModule *module=new TSaoriModule(path, type);
	if (aliasmap.count(aliasname))
		EraseModule(aliasname);
	aliasmap[aliasname]=module;
#ifdef __KAWARI__
	Logger.GetStream(LOG_INFO) << "[SAORI] Registered \"" << aliasname << "\" = (" << path << ")" << endl;
#endif
}
//---------------------------------------------------------------------------
// モジュール登録の削除
void TSaoriPark::EraseModule(const string &aliasname){
	if (aliasmap.count(aliasname)){
		TSaoriModule *module=aliasmap[aliasname];
		delete module;
		aliasmap.erase(aliasname);
#ifdef __KAWARI__
		Logger.GetStream(LOG_INFO) << "[SAORI] Unregistered (" << aliasname << ")" << endl;
	}else{
		Logger.GetStream(LOG_WARNING) << "[SAORI] Can not unregister (" << aliasname << "). not found." << endl;
#endif
	}
}
//---------------------------------------------------------------------------
// モジュールを得る
TSaoriModule * const TSaoriPark::GetModule(const string &aliasname) {
	if (aliasmap.count(aliasname)){
		return aliasmap[aliasname];
	}else{
#ifdef __KAWARI__
		Logger.GetStream(LOG_ERROR) << "[SAORI] module (" << aliasname << ") not found." << endl;
#endif
		return NULL;
	}
}
//---------------------------------------------------------------------------
// 全モジュールのアンロード
TSaoriPark::~TSaoriPark(){
	for(map<string, TSaoriModule *>::iterator it=aliasmap.begin(); it!=aliasmap.end(); it++){
		if (it->second){
			it->second->Detatch();
			delete it->second;
		}
	}
}
//---------------------------------------------------------------------------
