//---------------------------------------------------------------------------
//
// KOSUI - Direct SSTP Adapter
//
//      Programed by Suikyo.
//
//  2002.05.04  Phase 8.0.0   FirstVersion
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "tool/kosui_dsstp.h"
#include "misc/phttp.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstdio>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
#if defined(_MSC_VER) && (_MSC_VER < 1300)
// VC6付属のSDKにはDWORD_PTRが無い (32bit専用なのでDWORDと同じ)
typedef DWORD DWORD_PTR;
#endif
//---------------------------------------------------------------------------
namespace{
	const unsigned int DIRECT_SSTP_PORT=9801;

	// 戻り値が有効かどうか
	bool isResponseValid=false;

	// 戻り値受け渡し場所
	string response;	// 受信した生バイト列

	// ミューテックスオブジェクト
	HANDLE mutex;
}
//---------------------------------------------------------------------------
static LRESULT CALLBACK MainWndProc(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam)
{
	COPYDATASTRUCT* pcds;
	switch(message){
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
		pcds=(COPYDATASTRUCT*)lParam;
		if((pcds!=NULL)&&(pcds->dwData==DIRECT_SSTP_PORT)){
			// (ポインタ, 長さ)で作る。NUL終端は保証されない
			string tmpstr((const char *)pcds->lpData, (size_t)pcds->cbData);
			if(::WaitForSingleObject(mutex, 1000)!=WAIT_TIMEOUT){
				response=tmpstr;
				isResponseValid=true;
				::ReleaseMutex(mutex);
			}else{
				// 強引
				isResponseValid=true;
			}
			return TRUE;
		}else{
			return FALSE;
		}
	default:
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}
	return 0L;
}
//---------------------------------------------------------------------------
// 応答が届くか timeout_ms が過ぎるまでメッセージを処理する
// ダミーの窓はこのスレッドのものなので、送られてきたWM_COPYDATAはこのスレッドでしか処理されない
static void PumpMessagesUntilResponse(DWORD timeout_ms){
	DWORD start=::GetTickCount();
	while(!isResponseValid){
		MSG msg;
		while(::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)){
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
		if(isResponseValid) break;
		DWORD elapsed=::GetTickCount()-start;
		if(elapsed>=timeout_ms) break;
		::MsgWaitForMultipleObjects(0, NULL, FALSE, timeout_ms-elapsed, QS_ALLINPUT|QS_SENDMESSAGE);
	}
}
//---------------------------------------------------------------------------
// コンストラクタ
// hwnd : DAのウィンドウハンドル
TKosuiDSSTPInterface::TKosuiDSSTPInterface (HWND hwnd, const wstring& evt)
	: event(evt), dahwnd(hwnd), dumhwnd(0) {
	WNDCLASSW wndclass;
	memset(&wndclass, 0, sizeof(WNDCLASSW));

	const wchar_t *szClassName=L"KOSUI dummy window";
	const wchar_t *szAppName=L"KOSUI";

	// 現在のインスタンスを得る
	HINSTANCE hInstance=(HINSTANCE)::GetModuleHandle(NULL);

	// ダミーのウィンドウを作る
	wndclass.lpfnWndProc = MainWndProc;
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = szClassName;
	RegisterClassW(&wndclass);
	dumhwnd = CreateWindowW(
			szClassName, szAppName,
			WS_POPUP,
			CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
			NULL,NULL, hInstance, NULL);

	// Mutex Objectを作成
	mutex=::CreateMutexW(NULL, FALSE, NULL);
}
//---------------------------------------------------------------------------
// デストラクタ
TKosuiDSSTPInterface::~TKosuiDSSTPInterface(){
	// ウィンドウの削除
	DestroyWindow(dumhwnd);
	// ミューテックスオブジェクトの削除
	::CloseHandle(mutex);
}
//---------------------------------------------------------------------------
// 情報を得る
wstring TKosuiDSSTPInterface::GetInformation(void){
	return L"Can not get Information.(Direct SSTP mode)";
}
//---------------------------------------------------------------------------
// 与えられたソースを解釈・実行する
wstring TKosuiDSSTPInterface::Parse(const wstring& script){
	// 橋本孔明さんの 簡易SSTPクライアント を参考
	TPHMessage request;

	char szHWnd[100];
	// ウィンドウハンドルは64bit版Windowsでも32bitで表せる(符号拡張された値)
	sprintf(szHWnd, "%u", (unsigned int)(UINT_PTR)dumhwnd);

	// 送信するCharsetに従ってバイト列化する
	const TKawariCharset sendcs=KC_SJIS;
	request.SetStartline(L"NOTIFY SSTP/1.0");
	request[L"Charset"]=CharsetName(sendcs);
	request[L"Sender"]=L"KOSUI";
	request[L"Event"]=event;
	request[L"HWnd"]=AtoW(szHWnd);
	request[L"Reference0"]=script;

	string reqmes=request.Serialize(sendcs);

	COPYDATASTRUCT cds;
	cds.dwData = DIRECT_SSTP_PORT;
	cds.cbData = reqmes.size();
	cds.lpData = (LPVOID)reqmes.c_str();

	DWORD_PTR dwret;	// 64bitではPDWORD_PTR(8バイト)を要求される
	::SendMessageTimeout(
		dahwnd, WM_COPYDATA, (WPARAM)dumhwnd,
		(LPARAM)&cds, SMTO_ABORTIFHUNG, 1000, &dwret);

	// 送信の処理中に返らなかった応答を待つ
	PumpMessagesUntilResponse(1000);

	string respmes;

	if(::WaitForSingleObject(mutex, 1000)!=WAIT_TIMEOUT){
		if(isResponseValid){
			respmes=response;
			isResponseValid=false;
		}
		::ReleaseMutex(mutex);
	}
	// 応答のCharsetに従う (無ければShift_JIS)
	TKawariCharset rcs=TPHMessage::ScanCharset(respmes);
	if ((rcs!=KC_SJIS)&&(rcs!=KC_UTF8)) rcs=KC_SJIS;
	return MBtoW(respmes, rcs);
}
//---------------------------------------------------------------------------
