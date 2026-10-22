//---------------------------------------------------------------------------
//
// KOSUI -- Kawari cOnSole Use Interpritor --
// 幸水 -- コンソール版華和梨インタープリタ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.30  Phase 5.1     コンソールインタープリタ
//  2001.08.06  Phase 6.2     KawariDebuggerに合わせてデフォルトをecho-modeに
//  2001.08.12  Phase 6.2.1   KawariDebuggerと統合
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "libkawari/kawari.h"
#include "libkawari/kawari_engine.h"
#ifdef USEKDB
	#include "tool/kdb.h"
#endif
//---------------------------------------------------------------------------
// 幸水版
class TKawariInterfaceKosui: public TKawariInterface_base {
private:

	TNS_KawariANI *Kawari;
	TKawariEngine *KawariEngine;

public:

	TKawariInterfaceKosui(const string& datapath,const string& inifile)
	{
		Kawari=new TNS_KawariANI;
		KawariEngine=dynamic_cast<TKawariEngine*>(Kawari->Engine);
		if(!KawariEngine) throw("error");

		Kawari->LoadSub(datapath,inifile);
	}

	~TKawariInterfaceKosui()
	{
		Kawari->Unload();

		delete Kawari;
	}

	// バージョン番号を返す
	virtual string GetModuleVersion(void)
	{
		return(Kawari->GetModuleVersion());
	}

	// 与えられたスクリプトを解釈・実行する
	virtual string Parse(const string& script)
	{
		return(KawariEngine->Parse(script));
	}
};
//---------------------------------------------------------------------------
// 接続するゴーストを選ぶ
TKawariInterface_base* AttachTarget(const string& datapath,const string& inifile)
{
	cout << "[" << 0 << "] : " << "Interpreter mode" << endl;

	#ifdef USEKDB
		// 華和梨デバッガを組み込む場合
		map<HWND,string> ghostname;
		GetGhostList(ghostname);

		if(ghostname.size()>0) {
			vector<HWND> ghostlist;
			map<HWND,string>::iterator it;
			for(it=ghostname.begin();it!=ghostname.end();it++) {
				ghostlist.push_back(it->first);
				cout << "[" << ghostlist.size() << "] : " << it->second << endl;
			}

			unsigned int no;
			while(true) {
				cout << "Attach : ";
				cin >> no;
				if(no<=ghostlist.size()) break;
			}
			string buff;
			getline(cin,buff);

			if(no>0) return(new TKawariInterfaceSakuraAPI(ghostlist[no-1]));
		}
	#endif

	return(new TKawariInterfaceKosui(datapath,inifile));
}
//---------------------------------------------------------------------------
int main(int argc,char *argv[])
{
	string datapath="./";
	string inifile="./kawari.ini";

	if(argc>1) {
		inifile=argv[1];
		unsigned int pos=inifile.rfind('/');
		if(pos==string::npos) pos=inifile.rfind('\\');
		// iniファイル名が「表.ini」だったりしない限り破綻しない(反語)
		datapath=inifile.substr(0,pos+1);
	}

	cout << "K O S U I -- Kawari cOnSole Use Interpritor --" << endl;
	cout << "2001 Programed by NAKAUE.T (Meister)" << endl << endl;

	TKawariInterface_base *Kawari=AttachTarget(datapath,inifile);

	cout << endl;
	cout << "Kernel version " << Kawari->GetModuleVersion() << endl << endl;

	cout << "mode change by \".\"" << endl;

	bool commode=false;
	while(true) {
		if(commode) cout << "command-mode > ";
		 else cout << "echo-mode > ";

		string buff;
		getline(cin,buff);
		buff=buff.c_str();

		if(buff.size()==0) continue;

		if(buff=="exit") break;
		if(buff==".") {
			commode=!commode;
			continue;
		}

		if(commode) cout << Kawari->Parse(string("$(")+buff+")") << endl;
		 else cout << Kawari->Parse(buff) << endl;
	}

	delete Kawari;

	return(0);
}
//---------------------------------------------------------------------------

