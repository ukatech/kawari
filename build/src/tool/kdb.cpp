//---------------------------------------------------------------------------
//
// 華和梨デバッガ
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.24  Phase 0.10    コンソール版
//  2001.08.12  Phase 6.2.1   幸水と統合
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <cstdlib>
#include <sstream>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kdb.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
// SakuraAPI/1.3 イベント送信
void TKawariInterfaceSakuraAPI::SakuraEvent(const wstring& wevent,const wstring& wparam)
{
	// SakuraAPIはShift_JISのバイト列で受け渡す
	string event=WtoSJIS(wevent);
	string param=WtoSJIS(wparam);

	const WPARAM SA_GETPROCESSID=138;
	const WPARAM SA_GETSHAREDMEMORY=139;
	const WPARAM SA_NOTIFYEVENT=256;

	UINT WM_SAKURAAPI=RegisterWindowMessageW(L"Sakura");

	DWORD processid=(DWORD)SendMessage(Target,WM_SAKURAAPI,SA_GETPROCESSID,0);
	HANDLE hProcess=OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_WRITE,0,processid);

	HGLOBAL slot;

	for(int i=-1;i<=7;i++) {
		slot=(HGLOBAL)SendMessage(Target,WM_SAKURAAPI,SA_GETSHAREDMEMORY,(LPARAM)i);
		unsigned char dummy[8192]={0};
		WriteProcessMemory(hProcess,slot,(LPVOID)dummy,8192,NULL);
	}

	slot=(HGLOBAL)SendMessage(Target,WM_SAKURAAPI,SA_GETSHAREDMEMORY,(LPARAM)-1);
	WriteProcessMemory(hProcess,slot,(LPVOID)event.c_str(),event.size(),NULL);

	slot=(HGLOBAL)SendMessage(Target,WM_SAKURAAPI,SA_GETSHAREDMEMORY,(LPARAM)0);
	WriteProcessMemory(hProcess,slot,(LPVOID)param.c_str(),param.size(),NULL);

	CloseHandle(hProcess);

	SendMessage(Target,WM_SAKURAAPI,SA_NOTIFYEVENT,0);

	return;
}
//---------------------------------------------------------------------------
// 認識可能な他ゴーストのHWnd一覧を返す
void GetGhostList(map<HWND,wstring> &ghostlist)
{
	// Win32APIが使用可能で、File Mapping Object から情報を取得する場合

	HANDLE hFmo=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Sakura");
	if(!hFmo) return;	// 時々FileMappingObjectが壊れている

	// 0x80以上のバイトが符号拡張されないよう unsigned char で読む
	const unsigned char *fmobuff=(const unsigned char*)MapViewOfFile(hFmo,FILE_MAP_READ,0,0,0);
	if(!fmobuff) {
		CloseHandle(hFmo);
		return;
	}

	// File Mapping Object のサイズ
	// エンディアン嘘つかない
	unsigned int fmosize=
	 (((unsigned int)fmobuff[3])<<24)
	 +(((unsigned int)fmobuff[2])<<16)
	 +(((unsigned int)fmobuff[1])<<8)
	 +(((unsigned int)fmobuff[0]));
	if(fmosize<4) {
		UnmapViewOfFile(fmobuff);
		CloseHandle(hFmo);
		return;
	}

	istringstream *ifs=new istringstream(string((const char*)&fmobuff[4], fmosize-4));

	map<string,HWND> idmap;
	map<string,wstring> namemap;

	string buff;
	while(ifs->peek()!=0) {
		getline(*ifs,buff,'\x0a');
		if(buff.size()==0) break;	// 終了条件がわからないので

		string::size_type pos1=buff.find('.');
		string::size_type pos2=buff.find('\x1');
		string::size_type pos3=buff.find('\x0d');

		if((pos1==string::npos)||(pos2==string::npos)) break;	// 終了条件がわからないので

		string processid=buff.substr(0,pos1);
		string key=buff.substr(pos1+1,pos2-pos1-1);
		string value=buff.substr(pos2+1,pos3-pos2-1);

		if(key=="hwnd") {
			// HWNDは32bitで表せる値。64bit版では符号拡張してポインタにする
			idmap[processid]=((HWND)(INT_PTR)(LONG)strtoul(value.c_str(),NULL,10));
		} else if(key=="name") {
			namemap[processid]=SJIStoW(value);
		}
	}

	delete ifs;

	UnmapViewOfFile((LPVOID)fmobuff);

	CloseHandle(hFmo);

	map<string,HWND>::iterator it;
	for(it=idmap.begin();it!=idmap.end();it++) {
		ghostlist[it->second]=namemap[it->first];
	}

	return;
}
//---------------------------------------------------------------------------
