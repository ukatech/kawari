//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨7互換モード : SHIORI/2.x の振り分け
//
//  2026.11.16  Phase 9.3.0   華和梨 7.5.0 の kawari.cpp (Request / RequestGetSentence /
//                            SearchCommunicateTarget / GetResponse) から移植
//
//  7.5.0 は SHIORI/2.x しか話さない。応答の先頭行は SHIORI/2.6 にする
//  (ベースウェアは先頭行の版で 2.x か 3.0 かを決める)。SHIORI/3.0 の GET / NOTIFY (ID ヘッダ) には、
//  7.5.0 と同じく 400 Bad Request を返し、ベースウェアに 2.x へ切り替えさせる。
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <set>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "shiori/kawari_shiori.h"
#include "libkawari/kawari7.h"
#include "libkawari/kawari_log.h"
#include "misc/misc.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#define KW_WIDE2(x)	L##x
#define KW_WIDE(x)	KW_WIDE2(x)
//---------------------------------------------------------------------------
// ヘッダの値 (無ければ空。operator[] と違い、ヘッダを作らない)
static wstring Hdr(const TPHMessage &m, const wchar_t *key)
{
	TPHMessage::const_iterator it=m.find(key);
	if(it==m.end()) return L"";
	return it->second;
}
//---------------------------------------------------------------------------
// 空でなければヘッダに入れる (7.5.0 は空のヘッダを出さない)
static void Put(TPHMessage &m, const wchar_t *key, const wstring &value)
{
	if(value.size()) m[key]=value;
}
//---------------------------------------------------------------------------
static wstring Lower(const wstring &s)
{
	wstring r(s);
	for(size_t i=0; i<r.size(); i++)
		if((r[i]>=L'A')&&(r[i]<=L'Z')) r[i]=static_cast<wchar_t>(r[i]-L'A'+L'a');
	return r;
}
//---------------------------------------------------------------------------
// 末尾が suffix なら取り除く (7.5.0 は SecurityLevel が local / external のとき、
// Sender の末尾を 7 / 10 文字無条件に削っていた。接尾辞があるときだけ削る)
static wstring StripSuffix(const wstring &s, const wchar_t *suffix)
{
	wstring suf(suffix);
	if((s.size()>=suf.size())&&(s.compare(s.size()-suf.size(), suf.size(), suf)==0))
		return s.substr(0, s.size()-suf.size());
	return s;
}
//---------------------------------------------------------------------------
// system.* などの書き込み先エントリに、リクエストの値を文字列としてそのまま入れる
// (7.5.0 は値を単語としてコンパイルしていたが、${ や $( を含む入力が実行されてしまうので、
//  文字列として入れる。${system.Reference0} などで読み出した結果は同じ)
void TKawariShioriAdapter::SetSystemEntry7(const wstring &name, const wstring &value)
{
	if(value.size()) Engine.PushStrAfterClear(name, value);
	else Engine.ClearEntry(name);
}
//---------------------------------------------------------------------------
// エントリの先頭の単語を実行した結果
wstring TKawariShioriAdapter::FirstWord7(const wstring &name)
{
	return Engine.IndexParse(name);
}
//---------------------------------------------------------------------------
// 入力に返答する単語 (COMMUNICATE)
wstring TKawariShioriAdapter::GetResponse7(void)
{
	wstring aistr=Engine.RandomSelect(L"system.OnResponse");
	if(aistr.size()==0) aistr=Engine.RandomSelect(L"event.Response");
	if(aistr.size()==0) aistr=Engine.MatchRandomSelect();
	if(aistr.size()==0) aistr=Engine.RandomSelect(L"system.OnResponseUnknown");
	if(aistr.size()==0) aistr=Engine.RandomSelect(L"system.response.unknown");
	return aistr;
}
//---------------------------------------------------------------------------
// COMMUNICATE の相手を探す
bool TKawariShioriAdapter::SearchCommunicateTarget7(const wstring &targetghost, TPHMessage &response)
{
	if(targetghost.size()==0) return false;
	if(OtherGhostName.count(targetghost)==0) return false;

	Put(response, L"Sender", GhostName);
	Put(response, L"To", targetghost);

	for(int i=0; i<=7; i++) {
		wstring key=FirstWord7(L"system.Response"+IntToString(i));
		if(key.size()) response[L"Reference"+IntToString(i)]=key;
	}
	return true;
}
//---------------------------------------------------------------------------
// リクエストに応じて返答を作る (GET Sentence)
unsigned int TKawariShioriAdapter::RequestGetSentence7(TPHMessage &request, TPHMessage &response)
{
	wstring aistr;

	if(request.count(L"Event")||request.count(L"Sentence")) {
		// 7 の debug : debugger は、Reference を単語として評価して動く (ShioriEcho / KawariCommand)。
		// ini でデバッガを有効にしたゴーストの、そのイベントだけは 7 のとおりコンパイルする。
		wstring ev=Hdr(request, L"Event");
		bool evaluate=(ev==L"ShioriEcho")||(ev==L"KawariCommand");
		for(int i=0; i<=7; i++) {
			wstring refno=IntToString(i);
			wstring value=Hdr(request, (L"Reference"+refno).c_str());
			if(evaluate) Engine.PushAfterClear(L"system.Reference"+refno, value);
			else SetSystemEntry7(L"system.Reference"+refno, value);
		}
	}

	if(request.count(L"Event")) {
		wstring event=Hdr(request, L"Event");
		SetSystemEntry7(L"system.EventName", event);

		aistr=Engine.RandomSelect(L"event."+TKawari7Compiler::EncodeEntryName(event));
		if(aistr.size()==0) aistr=Engine.RandomSelect(L"system.OnEventUnknown");
		if(aistr.size()==0) return 204;
	} else if(request.count(L"Sentence")) {
		SetSystemEntry7(L"system.Sentence", Hdr(request, L"Sentence"));
		SetSystemEntry7(L"system.Sender", Hdr(request, L"Sender"));
		SetSystemEntry7(L"system.Age", Hdr(request, L"Age"));
		SetSystemEntry7(L"system.Surface", Hdr(request, L"Surface"));
		for(int i=0; i<=7; i++)
			Engine.ClearEntry(L"system.Response"+IntToString(i));
		aistr=GetResponse7();
	} else if(request.count(L"Word")) {
		SetSystemEntry7(L"system.Sentence", Hdr(request, L"Word"));
		aistr=GetResponse7();
	} else {
		aistr=Engine.RandomSelect(L"sentence");
		if(aistr.size()==0) return 204;
	}

	response[L"Sentence"]=aistr;
	return 200;
}
//---------------------------------------------------------------------------
// リクエスト (華和梨7互換モード)
void TKawariShioriAdapter::Request7(TPHMessage &request, TPHMessage &response)
{
	const wstring &reqline=request.GetStartline();

	bool saori=false;
	wstring::size_type pos=reqline.find(L" SHIORI/");
	if(pos==wstring::npos) {
		pos=reqline.find(L" SAORI/");
		saori=true;
	}
	wstring type=reqline.substr(0, pos);

	unsigned int statuscode=400;

	// 伝達経路 (SecurityLevel ヘッダ。local / external 以外は unknown)
	wstring sender=Hdr(request, L"Sender");
	wstring slv=Lower(StringTrim(Hdr(request, L"SecurityLevel")));
	TSecurityLevel::TSenderPath sender_path=TSecurityLevel::tsidUnknown;
	wstring sender_path_name=L"unknown";
	if(slv==L"local") {
		sender_path=TSecurityLevel::tsidLocal;
		sender_path_name=L"local";
		sender=StripSuffix(sender, L"(local)");
	} else if(slv==L"external") {
		sender_path=TSecurityLevel::tsidExternal;
		sender_path_name=L"external";
		sender=StripSuffix(sender, L"(external)");
	}

	// 本体への送信 (getproperty / executesstp / SAORI over DirectSSTP) は local のリクエストからだけ許す。
	// 9 の Request() と同じ。7 モードでも =kis ゾーンや 9 の関数から届くので、止めておく。
	Engine.GetBaseware().SetAllowed((sender_path==TSecurityLevel::tsidLocal)||(sender_path==TSecurityLevel::tsidSystem));

	if(!saori) {
		SetSystemEntry7(L"system.Sender", sender);
		request.erase(L"Sender");
		if(sender.size()) request[L"Sender"]=sender;
		SetSystemEntry7(L"system.Sender.Path", sender_path_name);
		// 9 の callsaori が SAORI に渡す SecurityLevel は System.Sender.Path から読む
		// (平坦なエントリ名では、大文字小文字の違う別のエントリ)
		SetSystemEntry7(L"System.Sender.Path", sender_path_name);

		if(SecurityLevel.Check(sender_path)) {
			if(type==L"NOTIFY"||type==L"NOTIFY Ghost"||type==L"NOTIFY OwnerGhostName") {
				wstring ghost=Hdr(request, L"Ghost");
				SetSystemEntry7(L"system.Ghost", ghost);
				GhostName=ghost;
				statuscode=200;
				Engine.RandomSelect(L"system.OnNotifyGhost");
			} else if(type==L"NOTIFY OtherGhostName") {
				OtherGhostName.clear();
				OtherGhostExName.clear();
				Engine.ClearEntry(L"system.OtherGhost");
				Engine.ClearEntry(L"system.OtherGhostEx");

				TPHMessage::const_iterator it;
				for(it=request.lower_bound(L"Ghost"); it!=request.upper_bound(L"Ghost"); ++it) {
					OtherGhostName.insert(it->second);
					Engine.PushStr(L"system.OtherGhost", it->second);
				}
				for(it=request.lower_bound(L"GhostEx"); it!=request.upper_bound(L"GhostEx"); ++it) {
					OtherGhostExName.insert(it->second);
					Engine.PushStr(L"system.OtherGhostEx", it->second);
				}
				statuscode=200;
				Engine.RandomSelect(L"system.OnNotifyOther");
			} else if(type==L"GET Version") {
				response[L"ID"]=KW_WIDE(KAWARI_NAME);
				response[L"Craftman"]=KW_WIDE(KAWARI_AUTHOR);
				response[L"Version"]=KW_WIDE(KAWARI_MAJOR) L"." KW_WIDE(KAWARI_MINOR) L"." KW_WIDE(KAWARI_SUBMINOR);
				statuscode=200;
			} else if(type==L"GET Sentence") {
				statuscode=RequestGetSentence7(request, response);

				// 話しかける相手 (無ければ、話しかけてきた相手)
				wstring targetghost=FirstWord7(L"system.communicate");
				if(targetghost.size()==0) targetghost=Hdr(request, L"Sender");
				Engine.ClearEntry(L"system.communicate");

				int age=0;
				if(targetghost==Hdr(request, L"Sender")) {
					if(request.count(L"Age")) age=StringToInt(Hdr(request, L"Age"))+1;
				}

				if((statuscode==200)&&(targetghost!=L"stop")&&(age<20)) {
					if(SearchCommunicateTarget7(targetghost, response))
						response[L"Age"]=IntToString(age);
				}
			} else if(type==L"GET Word") {
				if(request.count(L"Type")) {
					// Type には「\ms」のような文字列が渡される
					wstring key=L"compatible.";
					key+=Hdr(request, L"Type").substr(1);
					wstring aistr=Engine.RandomSelect(key);
					if(aistr.size()) {
						statuscode=200;
						response[L"Word"]=aistr;
					} else {
						statuscode=204;
					}
				} else {
					statuscode=204;
				}
			} else if(type==L"GET Status") {
				wstring aistr=Engine.RandomSelect(L"system.OnGetStatus");
				if(aistr.size()>0) {
					response[L"Status"]=aistr;
				} else {
					unsigned int nmatch=static_cast<unsigned int>(Engine.GetLegacy7State()->matches.size());
					response[L"Status"]=
					 IntToString(static_cast<int>(Engine.WordCollectionSize()))+L","
					 +IntToString(static_cast<int>(Engine.EntryCollectionSize()-nmatch))+L","
					 +IntToString(static_cast<int>(nmatch))+L","
					 +KW_WIDE(KAWARI_MAJOR) KW_WIDE(KAWARI_MINOR) KW_WIDE(KAWARI_SUBMINOR) L","
					 +L"100,"
					 +L"100";
				}
				statuscode=200;
			} else if(type==L"GET String") {
				// SHIORI/2.5
				if(request.count(L"ID")) {
					wstring id=Hdr(request, L"ID");
					SetSystemEntry7(L"system.ID", id);
					wstring aistr=Engine.RandomSelect(L"resource."+TKawari7Compiler::EncodeEntryName(id));
					if(aistr.size()) {
						statuscode=200;
						response[L"String"]=aistr;
					} else {
						statuscode=204;
					}
				}
			} else if(type==L"TEACH") {
				// SHIORI/2.4
				SetSystemEntry7(L"system.Sentence", Hdr(request, L"Word"));
				statuscode=200;
				response[L"Sentence"]=Engine.RandomSelect(L"system.OnTeach");
			} else if(type==L"TRANSLATE Sentence") {
				// SHIORI/2.6
				SetSystemEntry7(L"system.Sentence", Hdr(request, L"Sentence"));
				statuscode=200;
				wstring aistr=Engine.RandomSelect(L"system.OnTranslate");
				if(aistr.size()) response[L"Sentence"]=aistr;
				else statuscode=400;
			}
			// SHIORI/3.0 の GET / NOTIFY (ID ヘッダ) は、7.5.0 と同じく 400 Bad Request にする。
			// ベースウェアはその応答で 2.x に切り替える (3.0 で応答すると、COMMUNICATE や TEACH など
			// 2.x だけの機能が使われなくなる)
		}
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

	// バルーンのオフセット (どの GET Sentence でも返しうる)
	wstring balloonoffset=FirstWord7(L"system.BalloonOffset");
	if(balloonoffset.size()) {
		response[L"BalloonOffset"]=balloonoffset;
		Engine.ClearEntry(L"system.BalloonOffset");
	}

	if(saori)
		response.SetStartline(L"SAORI/" KW_WIDE(SAORIVER) L" "+statusheader);
	else
		response.SetStartline(L"SHIORI/2.6 "+statusheader);
}
//---------------------------------------------------------------------------
