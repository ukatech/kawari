//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 辞書操作 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.25  Phase 5.1     First version
//  2001.06.17  Phase 5.4     enumerate復活
//  2001.07.08  Phase 6.0     evalバグフィックス
//  2001.07.08  Phase 6.1     clear追加
//  2001.08.08  Phase 6.2     entry追加
//  2001.08.25  Phase 6.3     entry仕様追加
//                            getこっそり追加
//                            size追加
//  2001.12.18  Phase 7.2     arrayさらにこっそり追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_dict.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_dict.h"
//---------------------------------------------------------------------------
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
string KIS_set::Function(const vector<string>& args)
{
	if(args.size()>=3) KisEngine->Engine()->InsertAfterClear(args[1],args[2]);
	return("");
}
//---------------------------------------------------------------------------
string KIS_adddict::Function(const vector<string>& args)
{
	if(args.size()>=3) KisEngine->Engine()->Insert(args[1],args[2]);
	return("");
}
//---------------------------------------------------------------------------
string KIS_clear::Function(const vector<string>& args)
{
	if(args.size()>=2) KisEngine->Engine()->ClearEntry(args[1]);
	return("");
}
//---------------------------------------------------------------------------
string KIS_enumerate::Function(const vector<string>& args)
{
	string retstr;

	if(args.size()>=2) {
		vector<TWordID> wordcol;

		KisEngine->Dictionary()->FindAll(KisEngine->Dictionary()->GetEntryID(args[1]),wordcol);

		if(wordcol.size()) {
			retstr+=KisEngine->Dictionary()->GetWordFromID(wordcol[0])->DisCompile();

			for(unsigned int i=1;i<wordcol.size();i++) {
				retstr+=" ";
				retstr+=KisEngine->Dictionary()->GetWordFromID(wordcol[i])->DisCompile();
			}
		}
	}

	return(retstr);
}
//---------------------------------------------------------------------------
string KIS_eval::Function(const vector<string>& args)
{
	if(args.size()==1) return("");

	TKawariEngine *kawariengine=dynamic_cast<TKawariEngine*>(KisEngine->Engine());
	if(!kawariengine) return("");

	string word = args[1];
	for(unsigned int i=2;i<args.size();i++) word+=string(" ")+args[i];

	return(kawariengine->Parse(word));
}
//---------------------------------------------------------------------------
string KIS_entry::Function(const vector<string>& args)
{
	if((args.size()!=2)&&(args.size()!=3)) return("");

	string retstr=KisEngine->Engine()->RandomSelect(args[1]);
	if(retstr.size()) return(retstr);
	 else if(args.size()==3) return(args[2]);

	return("");
}
//---------------------------------------------------------------------------
string KIS_get::Function(const vector<string>& args)
{
	if(args.size()!=3) return("");

	vector<TWordID> wordcol;

	KisEngine->Dictionary()->FindAll(KisEngine->Dictionary()->GetEntryID(args[1]),wordcol);
	unsigned int i=(unsigned int)atoi(args[2].c_str());

	string retstr;

	if(wordcol.size()>i) {
		retstr=KisEngine->Dictionary()->GetWordFromID(wordcol[i])->DisCompile();
	}

	return(retstr);
}
//---------------------------------------------------------------------------
string KIS_size::Function(const vector<string>& args)
{
	if(args.size()!=2) return("");

	vector<TWordID> wordcol;

	KisEngine->Dictionary()->FindAll(KisEngine->Dictionary()->GetEntryID(args[1]),wordcol);

	return(IntToString(wordcol.size()));
}
//---------------------------------------------------------------------------
string KIS_array::Function(const vector<string>& args)
{
	if(args.size()!=3) return("");

	vector<TWordID> wordcol;

	KisEngine->Dictionary()->FindAll(KisEngine->Dictionary()->GetEntryID(args[1]),wordcol);
	unsigned int i=(unsigned int)atoi(args[2].c_str());

	string retstr;

	if(wordcol.size()>i) {
		retstr=KisEngine->Dictionary()->GetWordFromID(wordcol[i])->Run(KisEngine);
	}

	return(retstr);
}
//---------------------------------------------------------------------------

