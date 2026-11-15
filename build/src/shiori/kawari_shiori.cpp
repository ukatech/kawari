//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 暫定shiori.dll
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.02.03  Phase 0.3     ノーコメント
//  2001.02.11  Phase 0.31    再帰定義実装
//  2001.02.23  Phase 0.4     卒論戦争終戦祈念
//                            召薫造蕕�にお眠り下さい、過ちは繰り返しませあ�
//                                                                バージョン
//                            kawari.ini導入
//                            複数辞書ファイル
//                            sentence.txt廃止
//  2001.02.27  Phase 0.41    週末全くやれなかったバグ取り(getword関連)
//  2001.03.15  Phase 0.42    unload予約
//                            ロギング機能
//                            暗号化ファイル対応
//                            漢字エントリ対応
//                            ローカル変数対応
//  2001.04.25  Phase 0.50a1  レスポンス対応
//  2001.04.27  Phase 0.50a2  SHIORI/2.1対応
//  2001.04.28  Phase 0.50a3  COMMUNICATE戦争
//       |
//  2001.05.02
//  2001.05.03  Phase 0.50a4  インラインスクリプト
//  2001.05.12  Phase 0.50b2  Piroject-X 完結編
//                            SHIORI/2.2対応
//                            インラインスクリプトを$()に変更
//  2001.05.30  Phase 5.1     インタープリタ・コンパイラ化
//                            Phase0.50b2までのバグフィックス
//  2001.05.31  Phase 5.2     保守的piro
//  2001.06.10  Phase 5.3.1   GET Version対策
//                            偽SHIORI/2.4実装 (^_^;
//  2001.06.18  Phase 5.4     インラインスクリプト大幅強化
//  2001.07.10  Phase 6.0     getmoduleversion追加
//                            バグフィックス
//  2001.07.21  Phase 6.2     SHIORI/2.5実装
//                            内部イベント追加(OnLoad,OnUnload,SHIORI/2.4)
//                            ハードコーディングのエラーメッセージ廃止
//                            kawari.iniにadddict、include追加
//  2001.08.06  Phase 6.2     Age二重カウントアップのバグ修正
//  2001.08.07  Phase 6.2     内部イベントのプレフィックスをsystem.に統一
//                            内部イベント追加(OnNotifyGhost,OnNotifyOther,OnGetStatus)
//  2001.08.08  Phase 6.2     幸水用にLoadSubを新設
//  2001.08.25  Phase 6.3     セキュリティーホール対策
//  2001.08.25  Phase 7.0     セキュリティ対策(WriteProtect)
//  2001.09.23  Phase 7.0.1   セキュリティ対策をredo34以降の方式に対応
//                            Sender経路情報追加(System.Sender.Path)
//                            3エントリ以上でのAND検索バグを修正
//                            SHIORI/2.3b対応
//                            インラインスクリプト強化
//                            起動速度の高速化
//                            ハイフンを使用する旧エントリの対応停止
//  2002.01.12  Phase 8.0     なかなか出来ないPhase8
//                            NIDからフィードバック
//                            kpcg Phase7.3.1からフィードバック
//  2002.03                   Phase 8.0 (Rhapsody in the Blue)は破棄
//  2002.03.10  Phase 7.9.0   Philedelphia実験
//  2002.03.15                なんか、もう、あらゆるものが消えていく・・・
//                            kawari.ini廃止,代わりにkawarirc.kis導入
//  2002.03.20                SAORI統合
//  2002.04.12  Phase 8.0.0   インターフェース切り直し
//  2002.04.19                SHIORI/3.0対応
//  2002.12.30  Phase 8.1.0   Winter Comicket Version
//                            複数インスタンスを許可(内部的に)
//  2026.10.18  Phase 9.0.0   wchar_t化 (Charsetヘッダ対応)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
//#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cwchar>
#include <ctime>
#include <cctype>
using namespace std;
//---------------------------------------------------------------------------
#include "shiori/kawari_shiori.h"
#include "libkawari/kawari_log.h"
#include "misc/misc.h"
#include "misc/charset.h"
using namespace kawari_log;
//---------------------------------------------------------------------------
// 狭い文字列リテラルのマクロ(KAWARI_NAMEなど)をワイドにする
#define KW_WIDE2(x)	L##x
#define KW_WIDE(x)	KW_WIDE2(x)
//---------------------------------------------------------------------------
// レスポンスのCharsetヘッダを確定する
// 辞書側がSystem.Response.Charsetで有効な値を指定していればそれを優先、
// 無い/不明ならリクエストの文字コード(cs)を使う。
// 戻り値 : レスポンスのエンコードに使う文字コード
static TKawariCharset FixResponseCharset(TPHMessage &response, TKawariCharset cs)
{
	if (response.count(L"Charset")){
		TKawariCharset rcs=CharsetFromName(response[L"Charset"]);
		if ((rcs==KC_SJIS)||(rcs==KC_UTF8)) return rcs;
		response.erase(L"Charset");
	}
	response.Add(L"Charset", CharsetName(cs));
	return cs;
}
//---------------------------------------------------------------------------
// SHIORI/2.x API
//---------------------------------------------------------------------------
// DLLロード時に呼ばれる
bool TKawariShioriAdapter::Load(const wstring& datapath)
{
	// このタイミングで乱数初期化
	SRandom((unsigned int)time(NULL));

	Engine.SetDataPath(datapath);

	Engine.CreateEntry(L"System.DataPath").Push(Engine.CreateStrWord(datapath));
	Engine.WriteProtect(L"System.DataPath");

	// 初期実行スクリプト
	Engine.LoadKawariDict(datapath+L"kawarirc.kis");

	// セキュリティレベル設定
	wstring slv=Engine.IndexParse(L"System.SecurityLevel");
	if (slv.size()&&IsInteger(slv)){
		unsigned int lv=(unsigned int)StringToInt(slv);
		SecurityLevel.Set(lv);
	}else{
		Engine.CreateEntry(L"System.SecurityLevel").Push(Engine.CreateStrWord(IntToString(SecurityLevel.Get())));
	}
	Engine.WriteProtect(L"System.SecurityLevel");

	// 初期化終了
	initialized=true;

	Engine.GetLogger().GetStream(LOG_INFO) << L"[SHIORI/SAORI Adapter] Load finished." << endl;
	return(true);
}
//---------------------------------------------------------------------------
// DLLアンロード時に呼ばれる
bool TKawariShioriAdapter::Unload(void)
{
	wstring aistr=EnumExec(L"System.Callback.OnUnload");

	Engine.GetLogger().GetStream(LOG_INFO) << L"[SHIORI/SAORI Adapter] Unload." << endl;

	return(true);
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
void TKawariShioriAdapter::Request(TPHMessage &request, TPHMessage &response, TKawariCharset cs)
{
	const wstring &reqline=request.GetStartline();
	TKawariLogger &Logger=Engine.GetLogger();

	// リクエストコマンドの抽出
	bool saori=false;
	wstring::size_type pos=reqline.find(L" SHIORI/");
	if(pos==wstring::npos) {
		pos=reqline.find(L" SAORI/");
		saori=true;
	}
	wstring type=reqline.substr(0,pos);

	if (!initialized){
		// 未初期化エラー
		if(!saori) {
			response.SetStartline(L"SHIORI/" KW_WIDE(SHIORIVER) L" 500 Internal Server Error");
		} else {
			response.SetStartline(L"SAORI/" KW_WIDE(SAORIVER) L" 500 Internal Server Error");
		}
		return;
		// KIU_TODO: エラーログを吐きましょう
	}

	// ログ取りのチェック
	unsigned int evtchk;
	if(type==L"GET"){
		// GET SHIORI/3.0
		if(request[L"ID"].find(L"On")==0){
			// イベント
			if((request[L"ID"]==L"OnSecondChange")||(request[L"ID"]==L"OnMinuteChange"))
				// 時刻イベント
				evtchk=LOG_TIMEEVENTS;
			else if(request[L"ID"].find(L"OnMouse")==0)
				// マウスイベント
				evtchk=LOG_MOUSEEVENTS;
			else
				// その他
				evtchk=LOG_BASEEVENTS;
		}else{
			// リソース
			evtchk=LOG_RSCEVENTS;
		}
	}else if(type==L"NOTIFY"){
		// NOTIFY SHIORI/3.0
		evtchk=LOG_BASEEVENTS;
	}else if((type==L"GET String")&&(request.count(L"ID"))){
		// GET String SHIORI/2.0
		// リソース
		evtchk=LOG_RSCEVENTS;
	}else if(request.count(L"Event")) {
		// イベント
		if((request[L"Event"]==L"OnSecondChange")||(request[L"Event"]==L"OnMinuteChange")){
			// 時刻イベント
			evtchk=LOG_TIMEEVENTS;
		}else if (request[L"Event"]==L"OnMouseMove"){
			// マウスイベント
			evtchk=LOG_MOUSEEVENTS;
		}else{
			evtchk=LOG_BASEEVENTS;
		}
	}else{
		evtchk=LOG_BASEEVENTS;
	}
	bool logmode=Logger.Check(evtchk);

	if(logmode) {
		Logger.GetStream() << L"[SHIORI/SAORI Adapter] Query sequence begin." << endl;
		Logger.GetStream() << L"---------------------- REQUEST" << endl;
		request.Dump(Logger.GetStream());
	}

	unsigned int statuscode=400;

	// 伝達経路を確認
	wstring sender_path_name;
	TSecurityLevel::TSenderPath sender_path;
	GetSenderPath(request[L"SecurityLevel"],sender_path,sender_path_name);

	// 本体へ送る機能 (getproperty, 内蔵モジュールなど) は、localなリクエストからだけ使える
	Engine.GetBaseware().SetAllowed(
		(sender_path==TSecurityLevel::tsidLocal)||(sender_path==TSecurityLevel::tsidSystem));

	if(SecurityLevel.Check(sender_path)) {

		// リクエストヘッダを辞書に格納
		Engine.ClearTree(L"System.Request");
		Engine.CreateEntry(L"System.Request").Push(Engine.CreateStrWord(type));

		// 伝達経路 (local / external / unknown)。callsaoriがSAORIに渡すSecurityLevelに使う
		Engine.PushStrAfterClear(L"System.Sender.Path", sender_path_name);
		for(TMMap<wstring,wstring>::const_iterator it=request.begin();it!=request.end();it++) {
			Engine.CreateEntry(L"System.Request."+Engine.EncodeEntryName(it->first)).Push(Engine.CreateStrWord(it->second));
		}

		// 辞書内のレスポンスエントリを消去
		Engine.ClearTree(L"System.Response");

		// SHIORI/3.0
		if(type==L"GET"){
			const wstring &reqid=request[L"ID"];
			wstring aistr;

			// デフォルトKOSUIインターフェースはオーバーライド不可能
			if (reqid==L"ShioriEcho"){
				if (Engine.IndexParse(L"System.Debugger")==L"on"){
					Engine.ClearEntry(L"System.Request.Reference0");

					aistr=Engine.Parse(request[L"Reference0"])+L"\\e";
				}
			}else{
				aistr=EnumExec(L"System.Callback.OnGET");

				GetResponse(response);
				response.erase(L"Value");

				// version/craftman/craftmanw/shioriid/nameは、
				// ユーザがオーバーライド可能
				if (!aistr.size()){
					if (reqid==L"version")
						aistr=KW_WIDE(KAWARI_MAJOR) L"." KW_WIDE(KAWARI_MINOR) L"." KW_WIDE(KAWARI_SUBMINOR);
					else if ((reqid==L"craftman")||(reqid==L"craftmanw"))
						aistr=KW_WIDE(KAWARI_AUTHOR);
					else if ((reqid==L"shioriid")||(reqid==L"name"))
						aistr=KW_WIDE(KAWARI_NAME);
				}
			}

			if (aistr.size()){
				response[L"Value"]=aistr;
				statuscode=200;
			}else{
				statuscode=204;
			}
			response[L"Sender"]=GhostName;

		}else if(type==L"NOTIFY"){
			const wstring &reqid=request[L"ID"];
			if (reqid==L"ownerghostname"){
				GhostName=request[L"Reference0"];
			}else if (reqid==L"hwnd"){
				// 本体のウィンドウ。getproperty, 内蔵モジュールなどの送り先になる
				Engine.GetBaseware().SetHandleFromReference(request[L"Reference0"]);
			}
			wstring aistr=EnumExec(L"System.Callback.OnNOTIFY");

			GetResponse(response);
			response.erase(L"Value");
			response[L"Sender"]=GhostName;

			statuscode=204;
		}else
			// 以降 SHIORI/2.6
			if(type==L"GET Sentence") {
			// 単純発話とかイベントとか色々

			wstring aistr;
			if(request.count(L"Sentence")) {
				// コミュニケートの応答
				aistr=EnumExec(L"System.Callback.OnGetSentence");
			} else {
				// KOSUIインターフェース
				if (request[L"Event"]==L"ShioriEcho"){
					if (Engine.IndexParse(L"System.Debugger")==L"on"){
						Engine.ClearEntry(L"System.Request.Reference0");
						aistr=Engine.Parse(request[L"Reference0"])+L"\\e";
					}
				}else{
					// イベント処理/単純発話
					aistr=EnumExec(L"System.Callback.OnEvent");
				}
			}

			//
			GetResponse(response);
			response.erase(L"Sentence");
			response.erase(L"Age");
			response[L"Sender"]=GhostName;

			if(aistr.size()){
				response[L"Sentence"]=aistr;
				statuscode=200;
			}else{
				statuscode=204;
			}

			// ゴースト間コミュニケートの処理
			// 無言電話もできます
			wstring targetghost=response[L"To"];
			response.erase(L"To");
			wstring reqsender=request[L"Sender"];
			if((!targetghost.size())&&
			   ((!request.count(L"Age"))||(!reqsender.size())||
				(reqsender==L"User")||(reqsender==L"Nobody"))){
				// コミュニケート終了
			}else if(targetghost==L"stop"){
				// コミュニケート終了
			}else {
				int age=0;
				if(targetghost.size()&&(targetghost!=reqsender)){
					// 新しい相手にコミュニケート
				}else{
					// Ageのインクリメント
					targetghost=reqsender;
					if(request.count(L"Age"))
						age=StringToInt(request[L"Age"])+1;
				}
				response[L"To"]=targetghost;
				response[L"Age"]=IntToString(age);
			}
		} else if(type==L"GET Status") {
			wstring aistr=Engine.IndexParse(L"System.Callback.OnGetStatus");
			if(aistr.size()>0) {
				response[L"Status"]=aistr;
			} else {
				response[L"Status"]=
				 IntToString(Engine.WordCollectionSize())+L","
				 +IntToString(Engine.EntryCollectionSize())+L","
				 +L"100,"
				 +KW_WIDE(KAWARI_MAJOR) KW_WIDE(KAWARI_MINOR) KW_WIDE(KAWARI_SUBMINOR) L","	// 華和梨のバージョン番号
				 +L"100,"
				 +L"100";
			}
			statuscode=200;
		} else if((type==L"GET String")&&(request.count(L"ID"))) {
			// SHIORI/2.5
			wstring aistr=Engine.IndexParse(L"System.Callback.OnResource");
			response.erase(L"String");
			if (aistr.size()){
				statuscode=200;
				response[L"String"]=aistr;
			}else{
				statuscode=204;
			}
		} else if(type==L"NOTIFY OwnerGhostName") {
			// ゴースト名の通知
			GhostName=request[L"Ghost"];
			statuscode=200;
			EnumExec(L"System.Callback.OnRequest");
		} else if(type==L"GET Version") {
			// 初期化前のGET Versionだけはshiori_interface.cppで既に処理している
			if(!saori) {
				response[L"ID"]=KW_WIDE(KAWARI_NAME);
				response[L"Craftman"]=KW_WIDE(KAWARI_AUTHOR);
				response[L"Version"]=KW_WIDE(KAWARI_MAJOR) L"." KW_WIDE(KAWARI_MINOR) L"." KW_WIDE(KAWARI_SUBMINOR);
			} else {
				// 仕様書通りだと"200 OK"も付けられない事になるが、はて？
//				return("SAORI/1.0");
			}
			statuscode=200;
		} else if(saori&&(type==L"EXECUTE")) {
			// SAORI実行
			EnumExec(L"System.Callback.OnSaoriExecute");
			statuscode=GetResponse(response);
			if(statuscode==0) statuscode=400;
		} else {
			// その他全てのリクエスト
			EnumExec(L"System.Callback.OnRequest");
			statuscode=GetResponse(response);
			if(statuscode==0) statuscode=400;
		}
	} else {
		// 実行許可なし
	}

	wstring statusheader;
	switch(statuscode) {
	case 200:
		statusheader=L"200 OK";
		break;
	case 204:
		statusheader=L"204 No Content";
		break;
	case 311:
		statusheader=L"311 Not Enough";
		break;
	case 312:
		statusheader=L"312 Advice";
		break;
	case 400:
		statusheader=L"400 Bad Request";
		break;
	default:
		statusheader=L"500 Internal Server Error";
		break;
	}

	if(!saori)
		response.SetStartline(L"SHIORI/" KW_WIDE(SHIORIVER) L" "+statusheader);
	else
		response.SetStartline(L"SAORI/" KW_WIDE(SAORIVER) L" "+statusheader);

	// Charsetヘッダの確定 (辞書側の指定を優先)
	FixResponseCharset(response, cs);

	if (logmode){
		Logger.GetStream() << L"--------------------- RESPONSE" << endl;
		response.Dump(Logger.GetStream());
		Logger.GetStream() << L"[SHIORI/SAORI Adapter] Query sequence end." << endl;
	}
}
//---------------------------------------------------------------------------
// 以下はAPI以外
//---------------------------------------------------------------------------
// Senderを伝達経路情報と分離
void TKawariShioriAdapter::GetSenderPath(const wstring &senderstr,
 TKawariShioriAdapter::TSecurityLevel::TSenderPath &sender_path,wstring &sender_path_name)
{
	wstring sender=StringTrim(senderstr);

	if((sender==L"local")||(sender==L"Local")) {
		sender_path=TSecurityLevel::tsidLocal;
		sender_path_name=L"local";
	} else if((sender==L"external")||(sender==L"External")) {
		sender_path=TSecurityLevel::tsidExternal;
		sender_path_name=L"external";
	} else if(sender==L"") {
		// 往々にしてSecurityLevelは渡ってこない
		sender_path=TSecurityLevel::tsidLocal;
		sender_path_name=L"local";
	}else{
		sender_path=TSecurityLevel::tsidUnknown;
		sender_path_name=L"unknown";
	}

	return;
}
//---------------------------------------------------------------------------
// 指定したエントリの単語を全て呼び出す
wstring TKawariShioriAdapter::EnumExec(const wstring& key)
{
	TEntry entry=Engine.GetEntry(key);
	unsigned int size=entry.Size();

	wstring aistr;
	for(unsigned int i=0;i<size;i++) aistr+=Engine.IndexParse(entry,i);

	return(aistr);
}
//---------------------------------------------------------------------------
// 辞書内のレスポンスを全て読み出す
int TKawariShioriAdapter::GetResponse(TPHMessage &response)
{
	const wchar_t *resentryname=L"System.Response";
	TEntry resentry=Engine.GetEntry(resentryname);

	if (!resentry.IsValid()) return 0;

	vector<TEntry> entrycol;
	resentry.FindTree(entrycol);

	for(unsigned int i=0;i<entrycol.size();i++) {
		if(entrycol[i]!=resentry) {
			wstring key=entrycol[i].GetName();
			key=key.substr(wcslen(resentryname)+1);
			wstring str=Engine.IndexParse(entrycol[i]);
			if(str.size()) response[key]=str;
		}
	}

	return StringToInt(Engine.IndexParse(resentry));
}
//---------------------------------------------------------------------------
TKawariShioriFactory *TKawariShioriFactory::instance=NULL;
//---------------------------------------------------------------------------
// デストラクタ
TKawariShioriFactory::~TKawariShioriFactory(){
	typedef vector<TKawariShioriAdapter *> TKawariList;
	// 全てのインスタンスを破棄
	for(TKawariList::iterator it=list.begin(); it!=list.end(); it++)
		if (*it)
			delete (*it);
	list.clear();
}
//---------------------------------------------------------------------------
// 新規インスタンスの作成
unsigned int TKawariShioriFactory::CreateInstance(const wstring &datapath){
	TKawariShioriAdapter *instance=new TKawariShioriAdapter();
	if (!instance->Load(datapath)){
		delete instance;
		return 0;
	}

	// serching NULL entry
	int index=-1;
	int list_max=static_cast<int>(list.size());
	for(int i=0; i<list_max; i++)
		if (!list[i])
			index=i;
	if (index==-1){
		list.push_back(instance);
		return static_cast<unsigned int>(list.size());
	}else{
		list[index]=instance;
		return index+1;
	}
}
//---------------------------------------------------------------------------
// インスタンスの削除
bool TKawariShioriFactory::DisposeInstance(unsigned int h){
	if ((h==0)||(h>list.size())) return false;
	TKawariShioriAdapter *instance=list[(int)h-1];
	if (!instance) return false;
	instance->Unload();
	delete instance;
	list[h-1]=NULL;
	return true;
}
//---------------------------------------------------------------------------
// リクエスト
// リクエストのCharsetヘッダ(無い/不明ならShift_JIS)でデコードし、
// 同じ文字コード(辞書側がSystem.Response.Charsetを指定していればそれ)でエンコードして返す
string TKawariShioriFactory::RequestInstance(unsigned int h, const string &reqstr){
	if ((h==0)||(h>list.size())) return ("");
	TKawariShioriAdapter *instance=list[(int)h-1];
	if (!instance) return ("");

	TKawariCharset cs=TPHMessage::ScanCharset(reqstr);
	if ((cs!=KC_SJIS)&&(cs!=KC_UTF8)) cs=KC_SJIS;

	TPHMessage mreq, mres;
	mreq.Deserialize(reqstr, cs);
	instance->Request(mreq, mres, cs);
	TKawariCharset rcs=FixResponseCharset(mres, cs);
	return mres.Serialize(rcs);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// 以下のエントリは特別扱いされる(Phase7.9.0にて全面的に変更)
//
// 本体からの通知情報
// System.Request.*               : SHIORI/2.0 リクエストヘッダ
//
// 本体への応答
// System.Response.*              : SHIORI/2.0 レスポンス
// System.Response.To             : 話し掛けたいゴースト名
//                                  "stop"でCOMMUNICATE打ち切り
// System.Response                : SHIORI/2.0 ステータスコード
//
// イベント処理
// System.Callback.OnUnload       : 切り離しイベント
// System.Callback.OnEvent        : 外部イベント処理
// System.Callback.OnResource     : リソース取得イベント
// System.Callback.OnGetSentence  : 応答イベント
// System.Callback.OnGetStatus    : ステータス取得イベント
// System.Callback.OnSaoriExecute : SAORI実行
// System.Callback.OnRequest      : その他全てのリクエスト
//
// その他
// System.DataPath                : shiori.dllの存在するディレクトリ
//
//---------------------------------------------------------------------------
