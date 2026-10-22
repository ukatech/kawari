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
//                            ～安らかにお眠り下さい、過ちは繰り返しません～
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
//                             Phase0.50b2までのバグフィックス
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
//  2001.10.29  Phase 7.0.2   GhostExヘッダ対応
//                            GET Versionを再度kawari.cppに移管
//                            SHIORI/2.6仮対応(現状Shift_JISに固定)
//                            内部イベント追加(OnEventUnknown)
//  2001.10.31  Phase 7.0.3   SecurityLevelヘッダ対応
//  2001.11.21  Phase 7.1     エントリランダム呼び出しのバグ修正
//                            擬似乱数にMTアルゴリズムを採用
//  2001.11.25  Phase 7.1.1   ランダムの初期値に時刻を使うよう変更
//                            [幸水] helpコマンドのメッセージを(嘘臭い)英語に
//                            SecurityLevelを0～3に変更(4を廃止)
//  2001.12.08  Phase 7.1.2   kawari.iniでテキストファイル読み込み対応
//  2001.12.16  Phase 7.2     TMMapの値が復活するバグを修正
//                            (Thanks: しの、酔狂)
//  2002.01.07  Phase 7.3     著作者表示用ヘッダ追加(Thanks: えびさわ)
//                            伝達経路情報をSecurityLevelのみで判別
//  2002.01.08  Phase 7.3.1   BalloonOffsetヘッダ対応
//  2002.05.07  Phase 7.4.0   修正BSDライセンスに統一
//  2003.02.16  Phase 7.5.0   I18N(Thanks:Sanori,whoami)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari.h"
#include "libkawari/sequence_gen.h"
#include "misc/misc.h"
#include "misc/i18n.h"
#include "misc/mmap.h"
#include "saori/saori.h"
//---------------------------------------------------------------------------
// SHIORI/2.0 API
//---------------------------------------------------------------------------
// DLLロード時に呼ばれる
bool TNS_KawariANI::Load(const string& datapath)
{
	return(LoadSub(datapath,datapath+"kawari.ini"));
}
//---------------------------------------------------------------------------
// DLLアンロード時に呼ばれる
bool TNS_KawariANI::Unload(void)
{
	string aistr=GetSentence("system.OnUnload");
	if(LogFS) (*LogFS) << aistr << endl;

	if(LogFS) (*LogFS) << "Unload:" << endl;

	return(true);
}
//---------------------------------------------------------------------------
// 偽AIリクエスト
string TNS_KawariANI::Request(const string& type,TMMap<string,string>& request)
{
	bool logmode=true;
	// OnSecondChangeやOnMouseMoveで一々ログを残していたら身が保ちません
	if(request.count("Event")) {
//		if((request["Event"]=="OnSecondChange")||(request["Event"]=="OnMouseMove")) {
		if(request["Event"]=="OnMouseMove") {
			logmode=false;
		}
	}

	if(logmode&&LogFS) {
		(*LogFS) << "Begin Request" << endl;
		(*LogFS) << type << endl;
//		(*LogFS) << request["ORIGINAL"] << endl;
		TMMap<string,string>::iterator it;
		for(it=request.begin();it!=request.end();it++) {
			if(it->first!="ORIGINAL") (*LogFS) << it->first << " : " << it->second << endl;
		}
		(*LogFS) << endl;
	}


	map<string,string> response;
	unsigned int statuscode=400;

	string sender;
	if(request.count("Sender")) sender=request["Sender"];
//	 else sender="(external)";
    // 末尾の空白を取り除く
//	size_t ilastchr=sender.find_last_not_of(" ");
//	if((ilastchr!=string::npos)&&(ilastchr<(sender.length()-1)))
//		sender = sender.substr(0, ilastchr+1);

    // Senderを伝達経路情報と分離
    unsigned int sender_path;
    string sender_path_name="unknown";
//	if((sender.rfind("(local)")==(sender.length()-7))||(request["SecurityLevel"]=="local")){
    if(request["SecurityLevel"]=="local"){
        sender_path=SENDER_LOCAL;
        sender_path_name="local";
        sender=sender.substr(0, sender.length()-7);
//    }else if((sender=="embryo")||(sender=="SSP")){
//        sender_path=SENDER_SYSTEM;
//        sender_path_name="system";
//	}else if((sender.rfind("(external)")==(sender.length()-10))||(request["SecurityLevel"]=="external")){
    }else if(request["SecurityLevel"]=="external"){
        sender_path=SENDER_EXTERNAL;
        sender_path_name="external";
        sender=sender.substr(0, sender.length()-10);
    }else{
        sender_path=SENDER_UNKNOWN;
        sender_path_name="unknown";
    }

    // もう一度末尾の空白を取り除く
//	ilastchr=sender.find_last_not_of(" ");
//	if((ilastchr!=string::npos)&&(ilastchr<(sender.length()-1)))
//		sender = sender.substr(0, ilastchr+1);

    Engine->InsertAfterClear("system.Sender",sender);
    request["Sender"] = sender;
    Engine->InsertAfterClear("system.Sender.Path",sender_path_name);

	if(SecurityLevel.Check(sender_path)) {
		switch(type[0]) {
		case 'N':
			if((type=="NOTIFY")||(type=="NOTIFY Ghost")||(type=="NOTIFY OwnerGhostName")) {
//				Engine->InsertAfterClear("system-Ghost",request["Ghost"]);
				Engine->InsertAfterClear("system.Ghost",request["Ghost"]);
				GhostName=request["Ghost"];
				statuscode=200;
				string aistr=GetSentence("system.OnNotifyGhost");
				if(LogFS) (*LogFS) << aistr << endl;
			} else if(type=="NOTIFY OtherGhostName") {
				OtherGhostName.clear();
				OtherGhostExName.clear();
//				Engine->ClearEntry("system-OtherGhost");
				Engine->ClearEntry("system.OtherGhost");
				Engine->ClearEntry("system.OtherGhostEx");

				TMMap<string,string>::iterator it;
				for(it=request.lower_bound("Ghost");it!=request.upper_bound("Ghost");it++) {
					OtherGhostName.insert(it->second);
//					Engine->Insert("system-OtherGhost",it->second);
					Engine->Insert("system.OtherGhost",it->second);
				}

				for(it=request.lower_bound("GhostEx");it!=request.upper_bound("GhostEx");it++) {
					OtherGhostExName.insert(it->second);
					Engine->Insert("system.OtherGhostEx",it->second);
				}

				statuscode=200;
				string aistr=GetSentence("system.OnNotifyOther");
				if(LogFS) (*LogFS) << aistr << endl;
			}
			break;
		case 'G':
			if(type=="GET Version") {
				response["ID"] = KAWARI_NAME;
				response["Craftman"] = KAWARI_AUTHOR;
				response["Version"] = KAWARI_MAJOR "." KAWARI_MINOR "."KAWARI_SUBMINOR;
				statuscode=200;
			} else if(type=="GET Sentence") {
				statuscode=RequestGetSentence(request,response);

				string targetghost=Engine->FindFirst("system.communicate");
				if(targetghost.size()==0) {
					targetghost=request["Sender"];
//					targetghost=Engine->FindFirst("system-communicate");
//					if(targetghost.size()==0) {
//						targetghost=request["Sender"];
//					}
				}
				Engine->ClearEntry("system.communicate");
//				Engine->ClearEntry("system-communicate");

				int age=0;
				if(targetghost==request["Sender"]) {
					if(request.count("Age")) age=atoi(request["Age"].c_str())+1;
				}

				if((statuscode==200)&&(targetghost!="stop")&&(age<20)) {
					if(SearchCommunicateTarget(targetghost,response)) {
						response["Age"]=IntToString(age);
					}
				}
			} else if(type=="GET Word") {
				if(request.count("Type")) {
					// Typeには「\ms」のような文字列が渡される

					string key("compatible.");
					key+=request["Type"].substr(1);

					string aistr=GetSentence(key);
//					if(aistr.size()==0) {
//						key=string("compatible-")+request["Type"].substr(1);
//						aistr=GetSentence(key);
//					}
					if(aistr.size()) {
						statuscode=200;
						response["Word"]=aistr;
					} else {
						statuscode=204;
					}
				} else {
					statuscode=204;
				}
			} else if(type=="GET Status") {
				string aistr=GetSentence("system.OnGetStatus");
				if(aistr.size()>0) {
					response["Status"]=aistr;
				} else {
					response["Status"]=
					 IntToString(Engine->WordCollectionSize())+","
					 +IntToString(Engine->EntryCollectionSize()-Engine->MatchCollectionSize())+","
					 +IntToString(Engine->MatchCollectionSize())+","
					 +KAWARI_MAJOR KAWARI_MINOR KAWARI_SUBMINOR ","	// 華和梨のバージョン番号
					 +"100,"
					 +"100";
				}
				statuscode=200;
			} else if(type=="GET String") {
				// SHIORI/2.5
				if(request.count("ID")) {
					string key("resource.");
					key+=Engine->EncodeEntryName(request["ID"]);

					Engine->InsertAfterClear("system.ID",request["ID"]);
					string aistr=GetSentence(key);

					if(aistr.size()) {
						statuscode=200;
						response["String"]=aistr;
					} else {
						statuscode=204;
					}
				}
			}
			break;
		case 'T':
			if(type=="TEACH") {
				// 偽SHIORI/2.4

				Engine->InsertAfterClear("system.Sentence",request["Word"]);

				statuscode=200;
				response["Sentence"]=GetSentence("system.OnTeach");
			} else if(type=="TRANSLATE Sentence") {
				// SHIORI/2.6
				Engine->InsertAfterClear("system.Sentence",request["Sentence"]);
				statuscode=200;
				string aistr=GetSentence("system.OnTranslate");
				if(aistr.size()) {
					response["Sentence"]=aistr;
				} else {
					statuscode=400;
				}
			}
		}
	} else {
		// 実行許可なし
	}



	string statusheader;
	switch(statuscode) {
	case 200:
		statusheader="200 OK";
		break;
	case 204:
		statusheader="204 No Content";
		break;
	case 311:
		statusheader="311 Not Enough";
		break;
	case 312:
		statusheader="312 Advice";
		break;
	case 400:
		statusheader="400 Bad Request";
		break;
	default:
		statusheader="500 Internal Server Error";
		break;
	}

	string status="SHIORI/"SHIORIVER" "+statusheader+"\x0d\x0a";
	// いついかなる時も、responseにはCharsetを付加
	response["Charset"]=lang.getShioriCharset();

	// BalloonOffsetヘッダはsentence系なら全て起こりうるので、ここで対応
	// 2002/1/8 さとー
	string BalloonOffset=Engine->FindFirst("system.BalloonOffset");
	if(BalloonOffset.size()) {
		response["BalloonOffset"]=BalloonOffset;
		Engine->ClearEntry("system.BalloonOffset");
	}

	map<string,string>::iterator it;
	for(it=response.begin();it!=response.end();it++) {
		if(it->second.size()) status=status+it->first+": "+it->second+"\x0d\x0a";
	}
	status+="\x0d\x0a";

	if(logmode&&LogFS) {
		(*LogFS) << endl << "Response" << endl;
		(*LogFS) << status << endl;
		(*LogFS) << "End Request" << endl << endl;
	}

	return(status);
}
//---------------------------------------------------------------------------
// 以下はAPI以外
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TNS_KawariANI::TNS_KawariANI(void) : LogFS(NULL),SecurityLevel(2)
{
	Engine=new TKawariEngine(this);
}
TNS_KawariANI::~TNS_KawariANI()
{
	if(LogFS) {
		(*LogFS) << "delete object." << endl;
		LogFS->close();
		delete LogFS;
	}
	delete Engine;
}

// リクエストに応じて返答文を生成する
unsigned int TNS_KawariANI::RequestGetSentence(TMMap<string,string>& request,map<string,string> &response)
{
	string aistr;

	if(request.count("Event")||request.count("Sentence")) {
		for(int i=0;i<=7;i++){
			string refno=IntToString(i);
//			Engine->InsertAfterClear("system-Reference"+refno,request["Reference"+refno]);
			Engine->InsertAfterClear("system.Reference"+refno,request["Reference"+refno]);
		}
	}

	if(request.count("Event")) {
//		Engine->InsertAfterClear("system-EventName",request["Event"]);
		Engine->InsertAfterClear("system.EventName",request["Event"]);

		string entry="event."+Engine->EncodeEntryName(request["Event"]);
		aistr=GetSentence(entry);

		if(aistr.size()==0) {
			aistr=GetSentence("system.OnEventUnknown");
		}

//		if(aistr.size()==0) {
//			entry="event-"+Engine->EncodeEntryName(request["Event"]);
//			aistr=GetSentence(entry);
//		}

		if(aistr.size()==0) return(204);
	} else if(request.count("Sentence")) {
//		Engine->InsertAfterClear("system-Sentence",request["Sentence"]);
//		Engine->InsertAfterClear("system-Sender",request["Sender"]);
//		Engine->InsertAfterClear("system-Age",request["Age"]);
		Engine->InsertAfterClear("system.Sentence",request["Sentence"]);
		Engine->InsertAfterClear("system.Sender",request["Sender"]);
		Engine->InsertAfterClear("system.Age",request["Age"]);
		Engine->InsertAfterClear("system.Surface",request["Surface"]);
		// SHIORI/2.3b用リファレンスエントリの消去 (2001/09/15 さとー)
		for(int i=0;i<=7;i++){
			Engine->ClearEntry("system.Response"+IntToString(i));
		}
		aistr=GetResponse(request["Sentence"]);
	} else if(request.count("Word")) {
//		Engine->InsertAfterClear("system-Sentence",request["Word"]);
		Engine->InsertAfterClear("system.Sentence",request["Word"]);
		aistr=GetResponse(request["Word"]);
	} else {
		aistr=GetSentence("sentence");
		if(aistr.size()==0) return(204);
	}

	response["Sentence"]=aistr;

	return(200);
}
//---------------------------------------------------------------------------
// Communicateの相手を探す
bool TNS_KawariANI::SearchCommunicateTarget(const string &targetghost,map<string,string> &response)
{
	if(targetghost.size()==0) return(false);
	if(OtherGhostName.count(targetghost)==0) return(false);

	response["Sender"]=GhostName;
	response["To"]=targetghost;

	// Referenceヘッダの追加 (2001/09/15 さとー)
	for(unsigned int i=0;i<=7;i++) {
		string key=Engine->FindFirst("system.Response"+IntToString(i));
		if(key.size()) {
			response["Reference"+IntToString(i)]=key;
		}
	}

	return(true);
}
//---------------------------------------------------------------------------
// エントリに一致するランダム単語を返す
string TNS_KawariANI::GetSentence(const string& key)
{
	return(Engine->RandomSelect(key));
}
//---------------------------------------------------------------------------
// 文章に返答するランダム単語を返す
string TNS_KawariANI::GetResponse(string sentence)
{
	string aistr=GetSentence("system.OnResponse");
	if(aistr.size()==0) aistr=GetSentence("event.Response");
//	if(aistr.size()==0) aistr=GetSentence("event-Response");

	if(aistr.size()==0) aistr=Engine->MatchEntryRandomSelect("match.Response");

	if(aistr.size()==0) aistr=GetSentence("system.OnResponseUnknown");
	if(aistr.size()==0) aistr=GetSentence("system.response.unknown");
//	if(aistr.size()==0) aistr=GetSentence("system-response-unknown");

	return(aistr);
}
//---------------------------------------------------------------------------
// データファイル読み込み
bool TNS_KawariANI::LoadFile(const string &filename,const string &type)
{
	ifstream ifs;
	ifs.open(filename.c_str());

	if(!ifs.is_open()) return(false);

	string buff;
	getline(ifs,buff);
	buff=StringTrim(buff);

	ifs.close();

	if(buff!="[SAKURA]") {
		// 華和梨フォーマット
		return(Engine->LoadKawariDict(filename));
	} else {
		// SAKURAフォーマット
		return(LoadSakuraFile(filename,type));
	}
}
//---------------------------------------------------------------------------
// SAKURAフォーマットデータファイル読み込み
bool TNS_KawariANI::LoadSakuraFile(const string &filename,const string &type)
{
	if(type!="dict") return(false);

	ifstream ifs;
	ifs.open(filename.c_str());
	if(!ifs.is_open()) return(false);

	string buff;
	string key="";
	bool countflag=false;
	string countstring="";
	while(getline(ifs,buff)) {
		buff=StringTrim(buff);
		if(buff.size()>0) {
			if(buff[0]=='#') {
				if(!countflag) {
					key=buff;
				}else {
					if(buff.substr(0,buff.size()-1)==countstring) {
						key=countstring;
					} else {
						key=buff;
						countflag=false;
					}
				}
			} else if(key.size()>0) {
				if(buff.substr(0,5)=="count") {
					countflag=true;
					countstring=key;
				} else {
					string entry="piro."+Engine->EncodeEntryName(key);
					Engine->Insert(entry,buff);
					key="";
				}
			}
		}
	}

	return(true);
}
//---------------------------------------------------------------------------
// iniファイル読み込み
bool TNS_KawariANI::LoadINI(const string &filename,const string &datapath,vector<string> &dictfiles)
{
	ifstream ifs;
	ifs.open(filename.c_str());

	if(!ifs.is_open()) return(false);

	string buff;
	while(getline(ifs,buff)) {
		buff=StringTrim(buff);
		if((buff.size()>0)&&(buff[0]!='#')) {
			string key,value;
			string::size_type pos=buff.find(':');
			if(pos==string::npos) {
				key=buff;
				value="";
			} else {
				key=StringTrim(buff.substr(0,pos));
				value=StringTrim(buff.substr(pos+1));
			}

			if(key=="security") {
				SecurityLevel.Set((unsigned int)atoi(value.c_str()));
				if(LogFS) (*LogFS) << "SecurityLevel " << SecurityLevel.Get() << endl;
			} if(key=="randomseed") {
				SRandom(atoi(value.c_str()));
			} else if(key=="dict") {
				dictfiles.push_back(value);
			} else if(key=="text") {
				string::size_type pos=value.find_first_of(" \t");
				if(pos!=string::npos) {
					string entry=StringTrim(value.substr(0,pos));
					value=StringTrim(value.substr(pos+1));
					Engine->LoadTextFile(datapath+value,entry);
				}
			} else if(key=="adddict") {
				string::size_type pos=value.find_first_of(" \t");
				if(pos!=string::npos) {
					string entry=StringTrim(value.substr(0,pos));
					value=StringTrim(value.substr(pos+1));
					Engine->Insert(entry,value);
				}
			} else if(key=="set") {
				string::size_type pos=value.find_first_of(" \t");
				if(pos!=string::npos) {
					string entry=StringTrim(value.substr(0,pos));
					value=StringTrim(value.substr(pos+1));
					Engine->InsertAfterClear(entry,value);
				}
			} else if(key=="include") {
				LoadINI(datapath+value,datapath,dictfiles);
#ifdef I18N_HACK
			} else if(key=="locale") {
				string s = lang.setLocale(value);
				if(LogFS) {
					(*LogFS) << "Setting Locale \"" << value << "\": ";
					if (s =="") {
						(*LogFS) << "Failed";
					} else {
						(*LogFS) << "Succeeded (" << s << ")";
					}
					(*LogFS) << endl;
				}
#endif
			} else if(key=="debug") {
				if(value=="log") {
					if(!LogFS) LogFS=new ofstream((datapath+"kawari.log").c_str());
				} else if(value=="debugger") {
					if(LogFS) (*LogFS) << "Debugger Enable" << endl;

					Engine->Insert("event.ShioriEcho","${system.Reference0}\\e");
					Engine->Insert("event.KawariCommand","$(echo \"$(\"${system.Reference0}\")\" )\\e");
				}
			} else if(key=="saori") {
				vector<string> args;
				string::size_type curp = 0;
				string::size_type nextp;
				while(1) {
					string param;
					nextp = value.find(',', curp);
					if (nextp == string::npos) {
						param = value.substr(curp);
					} else {
						param = value.substr(curp, nextp - curp);
					}
					args.push_back(StringTrim(param));
					if (nextp == string::npos) break;
					curp = nextp + 1;
				}
				if(args.size()==2||args.size()==3) {
					TKawariEngine::SAORILOADTYPE loadopt=TKawariEngine::LOADONCALL;
					if (args.size()>=3){
						if (args[2]=="preload")
							loadopt=TKawariEngine::PRELOAD;
						else if (args[2]=="noresident")
							loadopt=TKawariEngine::NORESIDENT;
					}
					Engine->RegisterSAORIModule(args[1], Engine->GetDataPath()+args[0], loadopt);
					if (LogFS) {
						(*LogFS) << "SAORI:";
						(*LogFS) << " regist " << args[0];
						(*LogFS) << " as " << args[1];
						(*LogFS) << endl;
					}
				}
			}
		}
	}
	ifs.close();

	return(true);
}
//---------------------------------------------------------------------------
// 初期化とファイル読み込み
bool TNS_KawariANI::LoadSub(const string& datapath,const string& inifile)
{
	Engine->SetDataPath(datapath);

	// このタイミングで乱数初期化します
	SRandom((int)time(NULL));

	// 読み込むべき辞書ファイル一覧
	vector<string> dictfiles;

	// iniファイルの読み込み
	LoadINI(inifile,datapath,dictfiles);

	if(dictfiles.size()) {
		for(unsigned int i=0;i<dictfiles.size();i++) {
			LoadFile(datapath+dictfiles[i],"dict");
		}
	}

	string aistr=GetSentence("system.OnLoad");
	if(LogFS) (*LogFS) << aistr << endl;

	// セキュリティレベルが低いときは、homeurlの変更を禁止する
	if(SecurityLevel.Get()<3) Engine->WriteProtect("resource.homeurl");

	return(true);
}
//---------------------------------------------------------------------------
// 以下のエントリは特別扱いされる
//
// 以下は本体からの通知情報
// system.OtherGhost        : NOTIFYで通知されたゴースト名
// system.OtherGhostEx      : NOTIFYで通知されたゴースト名(サーフェス番号付)
// system.Reference0        : イベント引数Reference0～7
//        |
// system.Reference7
// system.Sender            : 話し掛けてきたゴースト名、又はシステム名
// system.Sender.Path       : Senderの経路情報(SYSTEM/LOCAL/UNKNOWN/EXTERNAL)
// system.Age               : COMMUNICATEの回数
// system.Sentence          : 渡された文章
// system.EventName         : 発生したイベント名
// system.ID                : 要求されたリソースID
// system.BalloonOffset     : バルーンのオフセット
//
// 以下はスクリプトから華和梨への通知
// system.communicate       : 話し掛けたいゴースト名
//                            "stop"でCOMMUNICATE打ち切り
// system.Response0         : COMMUNICATEで渡すReference0～7
//        |
// system.Response7
//
// 以下はイベント処理
// event.イベント名         : 外部イベント処理
// resource.リソース名      : SHIORI/2.5リソース取得
// system.OnLoad            : 読み込みイベント(内部イベント)
// system.OnUnload          : 切り離しイベント(内部イベント)
// system.OnTeach           : SHIORI/2.4Teachイベント(内部イベント)
// system.OnResponse        : 応答イベント(内部イベント)
// system.OnResponseUnknown : 応答不明イベント(内部イベント)
// system.OnNotifyGhost     : ゴースト名通知イベント(内部イベント)
// system.OnNotifyOther     : 他ゴースト名通知イベント(内部イベント)
// system.OnGetStatus       : ステータス取得イベント(内部イベント)
// system.OnEventUnknown    : SHIORI/2.2未定義イベント(内部イベント)
// system.OnTranslate       : SHIORI/2.6
//
// 以下はgetword,getdms用
// compatible.ms           : 名詞-人
// compatible.mz           : 名詞-無機物
// compatible.mc           : 名詞-社名
// compatible.mh           : 名詞-店名
// compatible.mt           : 名詞-技
// compatible.me           : 名詞-食物
// compatible.mp           : 名詞-地名みたいなもの
// compatible.m            : 名詞-非限定
// compatible.dms          : 「～に～する～」的な、品詞が複数連結された長めの名詞
//
//---------------------------------------------------------------------------
