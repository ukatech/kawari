//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// SAORI インターフェース(汎用)
//
//      Programed by Suikyo.
//
//  2002.04.15  Phase 8.0.0   えびさわさんバージョンを参考に導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (Charset対応)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "saori/saori.h"
#include "saori/saori_module.h"
#include "saori/saori_dsstp.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include "misc/misc.h"
using namespace kawari_log;
using namespace std;
using namespace saori;
//---------------------------------------------------------------------------
TBind::TBind(IModuleFactory &fac, TKawariLogger &lgr, const wstring &path, const LOADTYPE t)
	 : type(t), libpath(path), module(NULL), factory(fac), logger(lgr), sendcs(KC_UTF8)
{
	if (type==PRELOAD)
		Attach();
}
//---------------------------------------------------------------------------
// SAORI/1.0 request
bool TBind::Request(const TPHMessage &request, TPHMessage &response)
{
	Attach();
	if (!module)
		return false;

	bool ret=Query(request, response);

	if (type==NORESIDENT)
		Detach();

	return ret;
}
//---------------------------------------------------------------------------
// respcs : 応答のCharsetヘッダ (無い/不明ならKC_NULL)
bool TBind::Query(const TPHMessage &request, TPHMessage &response, TKawariCharset *respcs){

	// 送信するCharsetを要求ヘッダに反映する
	TPHMessage req(request);
	req.erase(L"Charset");
	req.Add(L"Charset", CharsetName(sendcs));

	if (logger.Check(LOG_INFO)){
		logger.GetStream() << L"[SAORI] Query to ("+libpath+L")" << endl
			<< L"---------------------- REQUEST" << endl;
		req.Dump(logger.GetStream());
	}

	TKawariCharset rcs=KC_NULL;
	response=TPHMessage();
	if (!module->RequestMessage(req, response, rcs)){
		// シリアライズ
		string reqstr=req.Serialize(sendcs);

		// Request
		string resstr=module->Request(reqstr);

		// デシリアライズ
		// 応答のCharsetに従う。無ければ、送ったのがUTF-8なら自動判定、そうでなければ送ったのと同じ
		rcs=TPHMessage::ScanCharset(resstr);
		TKawariCharset dcs=rcs;
		if (dcs==KC_NULL)
			dcs=(sendcs==KC_UTF8)?KC_NULL:sendcs;
		response=TPHMessage();
		response.Deserialize(resstr, dcs);
	}
	// (RequestMessageで処理するモジュール (SAORI over DirectSSTP) は、
	//  DLLのようにシリアライズせずにメッセージのまま受け渡す)

	// 応答に有効なCharsetがあれば、次回以降の送信に使う
	if (rcs!=KC_NULL)
		sendcs=rcs;
	if (respcs) *respcs=rcs;

	if (logger.Check(LOG_INFO)){
		logger.GetStream() << L"----------------------RESPONSE" << endl;
		response.Dump(logger.GetStream());
		logger.GetStream() << L"[SAORI] Query end." << endl;
	}

	return true;
}
//---------------------------------------------------------------------------
void TBind::Attach(void){
	if (module) return;
	module=factory.CreateModule(libpath);
	if (!module){
		logger.Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI] module attach failed";
		return;
	}

	TPHMessage request, response;

	// GET Version チェック
	// 最初はUTF-8で送り、応答のCharsetがあればそれを以後使う。
	// 無ければ以後はShift_JISで送る。
	sendcs=KC_UTF8;
	request.SetStartline(L"GET Version SAORI/1.0");
	request[L"Sender"]=L"kawari";

	TKawariCharset rcs=KC_NULL;
	Query(request, response, &rcs);
	if (rcs==KC_NULL)
		sendcs=KC_SJIS;

	if (response.GetStartline().find(L"SAORI/1.")!=0){
		logger.Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI] SAORI version mismatch.";
		Detach();
		return;
	}else{
		logger.GetStream(LOG_INFO) << L"[SAORI] (" << libpath << L") attached." << endl;
	}
}
//---------------------------------------------------------------------------
void TBind::Detach(void){
	if (module){
		factory.DeleteModule(module);
		module=NULL;
	}
	logger.GetStream(LOG_INFO) << L"[SAORI] (" << libpath << L") detached." << endl;
}
//---------------------------------------------------------------------------
TBind::~TBind(){
	Detach();
}
//---------------------------------------------------------------------------
// モジュールの登録
void TSaoriPark::RegisterModule(const wstring &aliasname, const wstring &path, const saori::LOADTYPE type){
	if (sandbox){
		// 安全モードでは実体を作らない (preload でも DLL を読まない)
		stubs.insert(aliasname);
		logger.GetStream(LOG_INFO) << L"[SAORI] Registered \"" << aliasname << L"\" = (" << path << L") (sandbox: not loaded)" << endl;
		return;
	}
	// 2002/09/02 多重ロード対策 suikyo@yk.rim.or.jp
	if (aliasmap.count(aliasname))
		EraseModule(aliasname);
	TBind *module=new TBind((*factory), logger, path, type);
	aliasmap[aliasname]=module;

	logger.GetStream(LOG_INFO) << L"[SAORI] Registered \"" << aliasname << L"\" = (" << path << L")" << endl;
}
//---------------------------------------------------------------------------
// モジュール登録の削除
void TSaoriPark::EraseModule(const wstring &alias){
	if (stubs.erase(alias)){
		logger.GetStream(LOG_INFO) << L"[SAORI] Unregistered (" << alias << L")" << endl;
	}else if (aliasmap.count(alias)){
		TBind *module=aliasmap[alias];
		delete module;
		aliasmap.erase(alias);
		logger.GetStream(LOG_INFO) << L"[SAORI] Unregistered (" << alias << L")" << endl;
	}else{
		logger.Diag(LOG_WARNING, kawari_log::W_SAORI) << L"[SAORI] Can not unregister (" << alias << L"). not found.";
	}
}
//---------------------------------------------------------------------------
// モジュールを得る
TBind * const TSaoriPark::GetModule(const wstring &alias) {
	if (aliasmap.count(alias)){
		return aliasmap[alias];
	}else if (RegisterInternalModule(alias)){
		return aliasmap[alias];
	}else{
		logger.Diag(LOG_ERROR, kawari_log::E_SAORI) << L"[SAORI] module (" << alias << L") not found.";
		return NULL;
	}
}
//---------------------------------------------------------------------------
// 本体に内蔵モジュールの一覧を問い合わせる (internal_systemのListModules)
void TSaoriPark::FetchInternalModules(void){
	TPHMessage request, response;
	request.SetStartline(L"EXECUTE SAORI/1.0");
	request.Add(L"Argument0", L"ListModules");
	TKawariCharset rcs=KC_NULL;
	CallDirectSSTPModule(*link, logger, L"internal_system", request, response, rcs, 5000);

	const wstring &startline=response.GetStartline();
	wstring::size_type space=startline.find(L' ');
	int code=(space==wstring::npos)?0:StringToInt(startline.substr(space+1));
	wstring description;
	TPHMessage::const_iterator ei=response.find(L"ErrorDescription");
	if (ei!=response.end()) description=ei->second;

	if ((code==200)||(code==204)){
		// Value0, Value1, ... に1つずつ。無ければResultのカンマ区切り
		vector<wstring> names;
		for (int i=0; ; i++){
			TPHMessage::const_iterator vi=response.find(L"Value"+IntToString(i));
			if (vi==response.end()) break;
			names.push_back(vi->second);
		}
		TPHMessage::const_iterator ri=response.find(L"Result");
		if (names.empty()&&(ri!=response.end())){
			const wstring &list=ri->second;
			wstring::size_type st=0;
			while (st<=list.size()){
				wstring::size_type en=list.find(L',', st);
				if (en==wstring::npos) en=list.size();
				names.push_back(list.substr(st, en-st));
				st=en+1;
			}
		}
		for (vector<wstring>::const_iterator it=names.begin(); it!=names.end(); it++){
			wstring name=LowerModuleName(StringTrim(*it));
			if (!name.empty()) internalmodules.insert(name);
		}
		internalstate=IM_LISTED;
	}else if ((code==400)&&(description==L"unknown module")){
		// SAORI over DirectSSTPには対応しているが、internal_systemが無い本体
		internalstate=IM_ANY;
	}else if (description==L"baseware does not support SAORI over DirectSSTP"){
		internalstate=IM_NONE;
	}
	// 応答が無かったときは、次の呼び出しで再び問い合わせる
}
//---------------------------------------------------------------------------
// aliasが本体の内蔵モジュール(internal_*)の名前なら、登録する。
// ＠SAORIに「alias,saori/<小文字の名前>.dll」と書いたのと同じ扱いになる
bool TSaoriPark::RegisterInternalModule(const wstring &alias){
	if ((!link)||(!IsInternalModuleName(alias))) return false;
	if (!link->IsAllowed()) return false;
	// 本体のウィンドウが分からないうちは問い合わせられない。次の呼び出しで再び試す
	if (!link->IsReady()) return false;

	if (internalstate==IM_UNKNOWN)
		FetchInternalModules();

	const wstring name=LowerModuleName(alias);
	switch (internalstate){
	case IM_LISTED:
		if (!internalmodules.count(name)) return false;
		break;
	case IM_ANY:
		break;
	default:
		return false;
	}

	RegisterModule(alias, wstring(DIRECTSSTP_PREFIX)+name, LOADONCALL);
	return aliasmap.count(alias)!=0;
}
//---------------------------------------------------------------------------
// 登録済みモジュール名のリストを得る
int TSaoriPark::ListModule(vector<wstring> &list)
{
	logger.GetStream(LOG_INFO) << L"listmodule" << endl;
	int ret=0;
	for(set<wstring>::const_iterator sit=stubs.begin(); sit!=stubs.end(); sit++){
		list.push_back(*sit);
		ret++;
	}
	map<wstring, TBind *>::const_iterator it=aliasmap.begin();
	for(;it!=aliasmap.end(); it++){
		logger.GetStream(LOG_INFO) << L"[SAORI] found(" << it->first << L")" << endl;
		list.push_back(it->first);
		ret++;
	}
	return ret;
}
//---------------------------------------------------------------------------
// 安全モード中に、aliasが実体を作らずに受け付けるモジュールか
bool TSaoriPark::IsSandboxStub(const wstring &alias) const{
	if (!sandbox) return false;
	return stubs.count(alias)||IsInternalModuleName(alias);
}
//---------------------------------------------------------------------------
TSaoriPark::TSaoriPark(TKawariLogger &lgr, dsstp::TBasewareLink *lnk)
	: logger(lgr), link(lnk), sandbox(false), internalstate(IM_UNKNOWN) {
	factory=new TModuleFactoryMaster(logger, link);
}
//---------------------------------------------------------------------------
// 全モジュールのアンロード
TSaoriPark::~TSaoriPark(){
	for(map<wstring, TBind *>::iterator it=aliasmap.begin(); it!=aliasmap.end(); it++){
		if (it->second){
			delete it->second;
		}
	}
	delete factory;
}
//---------------------------------------------------------------------------
