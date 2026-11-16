//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- コミュニケート --
//
//      Programed by NAKAUE.T (Meister)
//
//  2002.03.18  Phase 7.9.0   新コミュニケート機構
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_communicate.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
wstring KIS_matchall::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	for(unsigned int i=2;i<args.size();i++) {
		if((args[1]).find((args[i]))==wstring::npos) return(L"");
	}

	return(L"true");
}
//---------------------------------------------------------------------------
wstring KIS_communicate::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");
#if 0
	wstring entryname;
	int st,end;

	TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	TEntry entry=Engine->GetEntry(entryname);
	unsigned int size=entry.Size();
	if(st<0) st=size+st;
	if(end<0) end=size+end;
	if((st<0)||(end<0)||(st>end)) {
		// エラー
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	vector<wstring> matched;
	for(unsigned int i=(unsigned int)st;i<=(unsigned int)end;i++) {
		wstring result=Engine->IndexParse(entry,i);
		if(result.size()) matched.push_back(result);
	}
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		// エラー
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}
	
	vector<wstring> matched;
	for(unsigned int i=range.Start;i<=range.End;i++) {
		wstring result=Engine->IndexParse(range.Entry,i);
		if(result.size()) matched.push_back(result);
	}
#endif

	wstring retstr;
	if(matched.size()) {
		unsigned int index=Engine->Random(static_cast<int>(matched.size()));
		TEntry entry2=Engine->GetEntry(matched[index]);
		if(!entry2.IsValid()) return(L"");

		unsigned int size=entry2.Size();
		retstr=Engine->IndexParse(entry2,Engine->Random(size));
	} else if(args.size()>=3) {
		retstr=args[2];
	}

	return(retstr);
}
//---------------------------------------------------------------------------

