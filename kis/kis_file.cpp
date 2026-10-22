//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- file attribute --
//
//      Programed by Kouji.U (sky)
//
//  2001.06.03  Created
//  2001.06.17  Phase 6.0     Phase6.0にあわせて修正 (NAKAUE.T)
//  2001.07.14  Phase 6.1     loadで絶対パス指定可能に (NAKAUE.T)
//  2001.07.19  Phase 6.2     Mingw対応 (NAKAUE.T)
//  2001.12.09  Phase 7.1.2   kis_textfileと統合
//                            readdir追加
//  2001.12.18  Phase 7.2     VC向けにFindFirstFileA、FindNextFileAを使った
//                            opendir/readdir/closedir作成(Thanks: えびさわ)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <sys/types.h>
#if defined(_WIN32) && defined(NOHAVE_READDIR)
	#include "misc/_dirent.h"
#else
	#include <dirent.h>
#endif
using namespace std;
//---------------------------------------------------------------------------
#include "kis/kis_file.h"
#include "libkawari/kawari_engine.h"
//---------------------------------------------------------------------------
void KIS_save::Run(const vector<string>& args, bool crypt)
{
	if (args.size()<3) return;

	vector<string> entry;
	entry.insert(entry.end(),args.begin()+2,args.end());

	KisEngine->Engine()->SaveKawariDict(KisEngine->Engine()->GetDataPath()+args[1],entry,crypt);

	return;
}
//---------------------------------------------------------------------------
string KIS_load::Function(const vector<string>& args)
{
	if (args.size()<2) return("");

	string filename;
	if((args[1][0]=='/')
#if defined(_Windows)||defined(_MSC_VER)||defined(_WIN32)||defined(__MSDOS__)
	 ||(args[1][0]=='\\')||((args[1].size()>1)&&(args[1][1]==':'))
#endif
	 ) {
		// 絶対パス
		filename=args[1];
	} else {
		// 相対パス
		filename=KisEngine->Engine()->GetDataPath()+args[1];
	}

	KisEngine->Engine()->LoadKawariDict(filename);

	return("");
}
//---------------------------------------------------------------------------
string KIS_textload::Function(const vector<string>& args)
{
	if(args.size()!=3) return("");

	string filename;
	if((args[1][0]=='/')
#if defined(_Windows)||defined(_MSC_VER)||defined(_WIN32)||defined(__MSDOS__)
	 ||(args[1][0]=='\\')||((args[1].size()>1)&&(args[1][1]==':'))
#endif
	 ) {
		// 絶対パス
		filename=args[1];
	} else {
		// 相対パス
		filename=KisEngine->Engine()->GetDataPath()+args[1];
	}

	KisEngine->Engine()->LoadTextFile(filename,args[2]);

	return("");
}
//---------------------------------------------------------------------------
string KIS_readdir::Function(const vector<string>& args)
{
	if(args.size()!=3) return("");

	string dirname;
	if((args[1][0]=='/')
#if defined(_Windows)||defined(_MSC_VER)||defined(_WIN32)||defined(__MSDOS__)
	 ||(args[1][0]=='\\')||((args[1].size()>1)&&(args[1][1]==':'))
#endif
	 ) {
		// 絶対パス
		dirname=args[1];
	} else {
		// 相対パス
		dirname=KisEngine->Engine()->GetDataPath()+args[1];
	}

	DIR *dirhandle;
	struct dirent *direntry;
	if((dirhandle=opendir(dirname.c_str()))==NULL) return("");

	KisEngine->Engine()->ClearEntry(args[2]);

	while((direntry=readdir(dirhandle))!=NULL) {
		string entry=direntry->d_name;
		KisEngine->Engine()->Insert(args[2],entry);
	}

	closedir(dirhandle);

	return("");
}
//---------------------------------------------------------------------------
