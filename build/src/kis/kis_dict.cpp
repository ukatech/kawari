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
//  2001.07.14  Phase 6.1     clear追加
//  2001.08.08  Phase 6.2     entry追加
//  2001.08.25  Phase 6.3     entry仕様追加
//                            getこっそり追加
//                            size追加
//  2001.12.18  Phase 7.2     arrayさらにこっそり追加
//  2002.03.10  Phase 7.9.0   辞書の直接アクセス禁止
//                            enumerate,array廃止
//                            get仕様変更,exec,enumexec導入
//  2002.03.13                エントリ名に添え字概念導入
//  2002.03.17                辞書操作関数整理
//                            list操作関数追加
//  2005.06.28  Phase 8.2.3   TEntryRange対応
//  2008.02.16  Phase 8.2.7   entrycountバグフィックス
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_dict.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
using namespace kawari::resource;
using namespace kawari_log;
//---------------------------------------------------------------------------
#include <cstdlib>
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_encode_entryname::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return(L"");

	return(TKawariEngine::EncodeEntryName(args[1]));
}
//---------------------------------------------------------------------------
wstring KIS_eval::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return(L"");

	wstring word = args[1];
	for(unsigned int i=2;i<args.size();i++) word+=wstring(L" ")+args[i];

	return(Engine->Parse(word));
}
//---------------------------------------------------------------------------
wstring KIS_size::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return(L"");

	return(IntToString(Engine->EntrySize(args[1])));
}
//---------------------------------------------------------------------------
wstring KIS_set::Function_(const vector<wstring>& args,bool flag_str)
{
	if(!AssertArgument(args, 3)) return(L"");

	wstring word = args[2];
	for(unsigned int i=3;i<args.size();i++) word+=wstring(L" ")+args[i];

#if 0
	wstring entryname;
	int st,end;

	int range=TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	switch(range) {
	case 0:		// 範囲指定無し
		if (flag){
			TEntry entry=Engine->CreateEntry(entryname);
			TWordID id=Engine->CreateStrWord(word);
			entry.PushAfterClear(id);
		}else{
			Engine->PushAfterClear(entryname,word);
		}
		break;
	case 1:		// 単独要素指定
	case 2:		// 範囲指定
		{
			unsigned int size=Engine->EntrySize(entryname);
			if(st<0) st=size+st;
			if(end<0) end=size+end;
			if((st<0)||(end<0)||(st>end)) {
				// エラー
				GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
				return(L"");
			}

			TEntry entry=Engine->CreateEntry(entryname);

			TWordID id;
			if(flag) id=Engine->CreateStrWord(word);
			 else id=Engine->CreateWord(word);

			TWordID nullstr=Engine->CreateStrWord(L"");

			for(unsigned int i=st;i<=(unsigned int)end;i++) {
				entry.Replace2(i,id,nullstr);
			}
		}
		break;
	}

#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	TWordID wid=(flag_str)?
		Engine->CreateStrWord(word):Engine->CreateWord(word);
	if(!range.Range){
		range.Entry.PushAfterClear(wid);
	}else{
		TWordID nullstr=Engine->CreateStrWord(L"");
		for(unsigned int i=range.Start;i<=range.End;i++) 
			range.Entry.Replace2(i,wid,nullstr);
	}
#endif
	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_adddict::Function_(const vector<wstring>& args,bool flag_str)
{
	if(!AssertArgument(args, 3)) return (L"");

	wstring word = args[2];
	for(unsigned int i=3;i<args.size();i++) word+=wstring(L" ")+args[i];
#if 0
	wstring entryname;
	int st,end;
	TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	TEntry entry=Engine->CreateEntry(entryname);

	TWordID id;
	if(flag) id=Engine->CreateStrWord(word);
	 else id=Engine->CreateWord(word);

	entry.Push(id);
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	TWordID wid=(flag_str)?
		Engine->CreateStrWord(word):Engine->CreateWord(word);
	range.Entry.Push(wid);
#endif
	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_unshift::Function_(const vector<wstring>& args,bool flag_str)
{
	if(!AssertArgument(args, 3)) return (L"");

	wstring word = args[2];
	for(unsigned int i=3;i<args.size();i++) word+=wstring(L" ")+args[i];
#if 0
	wstring entryname;
	int st,end;
	TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	TEntry entry=Engine->CreateEntry(entryname);

	TWordID id;
	if(flag) id=Engine->CreateStrWord(word);
	 else id=Engine->CreateWord(word);

	entry.Insert(0,id);
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	TWordID wid=(flag_str)?
		Engine->CreateStrWord(word):Engine->CreateWord(word);
	range.Entry.Insert(0,wid);
#endif
	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_pop::Function_(const vector<wstring>& args,bool flag,bool flag2)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	unsigned int size=Engine->EntrySize(args[1]);
	if(size==0) return(L"");

	unsigned int pos=flag?0:(size-1);

	wstring retstr;
	if(flag2)
		retstr=Engine->IndexWord(args[1],pos);
	else
		retstr=Engine->IndexParse(args[1],pos);

	Engine->Erase(args[1],pos,pos);

	return(retstr);
}
//---------------------------------------------------------------------------
wstring KIS_getrandom::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 3)) return (L"");

	wstring failstr;
	if(args.size()==3) failstr=args[2];

	TEntry entry=Engine->GetEntry(args[1]);
	if(!entry.IsValid()) return(failstr);

	unsigned int size=entry.Size();
	if(size==0) return(failstr);

	wstring retstr=Engine->IndexParse(entry,Engine->Random(size));
	if(retstr.size()==0) return(failstr);

	return(retstr);
}
//---------------------------------------------------------------------------
wstring KIS_get::Function_(const vector<wstring>& args,bool flag)
{
	if(!AssertArgument(args, 2, 2)) return (L"");
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

	wstring retstr;
	for(unsigned int i=(unsigned int)st;i<=(unsigned int)end;i++) {
		if(flag) retstr+=Engine->IndexWord(entry,i);
		 else retstr+=Engine->IndexParse(entry,i);
	}
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	wstring retstr;
	//MEMO: サイズ外(存在しない単語)の逆コンパイル結果は仕様として保証されるのか？
	if(flag)
		for(unsigned int i=range.Start;i<=range.End;i++)
			retstr+=Engine->IndexWord(range.Entry,i);
	else
		for(unsigned int i=range.Start;i<=range.End;i++)
			retstr+=Engine->IndexParse(range.Entry,i);
#endif
	return(retstr);
}
//---------------------------------------------------------------------------
wstring KIS_clear::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");
#if 0
	wstring entryname;
	int st,end;

	int range=TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	switch(range) {
	case 0:		// 範囲指定無し
		Engine->ClearEntry(entryname);
		break;
	case 1:		// 単独要素指定
	case 2:		// 範囲指定
		{
			unsigned int size=Engine->EntrySize(entryname);
			if(st<0) st=size+st;
			if(end<0) end=size+end;
			if((st<0)||(end<0)||(st>end)) {
				// エラー
				GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
				return(L"");
			}

			Engine->Erase(entryname,st,end);
		}
		break;
	}
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	if(!range.Range)
		range.Entry.Clear();
	else
		range.Entry.Erase(range.Start, range.End);
#endif
	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_insert::Function_(const vector<wstring>& args,bool flag_str)
{
	if(!AssertArgument(args, 3, 3)) return (L"");
#if 0
	wstring entryname;
	int st,end;

	TKawariEngine::DecodeEntryName(args[1],entryname,st,end);

	unsigned int size=Engine->EntrySize(entryname);
	if(st<0) st=size+st;
	if(st<0){
		// エラー
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return (L"");
	}

	wstring word = args[2];

	TEntry entry=Engine->CreateEntry(entryname);

	TWordID id;
	if(flag)
		id=Engine->CreateStrWord(word);
	else
		id=Engine->CreateWord(word);

	entry.Insert(st,id);
#else
	TEntryRange range=Engine->GetEntryRange(args[1]);
	if(range.Start==TKawariEngine::NPos){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_INVALID_INDEX) << args[0] << RC.S(ERR_KIS_DICT_INVALID_INDEX);
		return(L"");
	}

	//MEMO: pushやunshiftは二つ目以降の引数をcatするのに、
	//insert系は単独の単語しか取らないのはなぜ？  今さら変えられないけど。
	TWordID wid=(flag_str)?
		Engine->CreateStrWord(args[2]): Engine->CreateWord(args[2]);
	range.Entry.Insert(range.Start, wid);
#endif
	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_writeprotect::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");

	Engine->WriteProtect(args[1]);
	return (L"");
}
//---------------------------------------------------------------------------
wstring KIS_wordcount::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 1, 1)) return (L"");

	return IntToString(Engine->WordCollectionSize());
}
//---------------------------------------------------------------------------
wstring KIS_entrycount::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 1, 1)) return (L"");

#if 0
	return IntToString(Engine->EntryCollectionSize());
#else
	int entryno=0;
	TEntry rootentry=Engine->CreateEntry(L".");
	vector<TEntry> entrycol;
	if(rootentry.FindTree(entrycol)){
		// sort | uniq
		sort(entrycol.begin(), entrycol.end());
		vector<TEntry>::iterator end=unique(entrycol.begin(), entrycol.end());
		for(vector<TEntry>::iterator it=entrycol.begin(); it!=end; it++){
			if (it->GetName().size()) entryno++;
		}
	}
	return IntToString(entryno);
#endif
}
//---------------------------------------------------------------------------
wstring KIS_find::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 4)) return (L"");

	if(!Engine->EntrySize(args[1])) return (L"-1");
	unsigned int min=(args.size()==4)?StringToInt(args[3]):0;
	unsigned int index=Engine->Find(args[1], args[2], min);
	return (index==Engine->NPos)? (wstring(L"-1")):IntToString((int)index);
}
//---------------------------------------------------------------------------
wstring KIS_rfind::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3, 4)) return (L"");

	if(!Engine->EntrySize(args[1])) return (L"-1");
	unsigned int max=(args.size()==4)?StringToInt(args[3]):Engine->NPos;
	unsigned int index=Engine->RFind(args[1], args[2], max);
	return (index==Engine->NPos)? (wstring(L"-1")):IntToString((int)index);
}
//---------------------------------------------------------------------------
wstring KIS_cleartree::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2, 2)) return (L"");
	if(!args[1].size()) return (L"");

	Engine->ClearTree(args[1]);

	return (L"");
}
//---------------------------------------------------------------------------
void KIS_listsub::_Function(const vector<wstring>& args, bool subflag){
	// エントリ操作周りはエラーチェックを慎重にやります。
	if(!AssertArgument(args, 3, 3)) return;
	if((!args[1].size())||(!args[2].size())) return;

	// "."でも上手くいく
	TEntry parententry=Engine->CreateEntry(args[2]);
	TEntry tempentry=Engine->CreateEntry(args[1]);

	vector<TEntry> entrycol;
	if((subflag)?parententry.FindAllSubEntry(entrycol):
	   parententry.FindTree(entrycol)){
		// sort | uniq
		sort(entrycol.begin(), entrycol.end());
		vector<TEntry>::iterator end=unique(entrycol.begin(), entrycol.end());

		for(vector<TEntry>::iterator it=entrycol.begin(); it!=end; it++){
			wstring entryname=it->GetName();
			if (!entryname.size()) continue;
			tempentry.Push(Engine->CreateStrWord(entryname));
		}
	}
	return;
}
//---------------------------------------------------------------------------
void KIS_copy::_Function(const vector<wstring>& args, bool rmflag){
	if(!AssertArgument(args, 3, 3)) return;
	if((!args[1].size())||(!args[2].size())) return;

	// 引数の順番どうするか……
	TEntry srcentry=Engine->GetEntry(args[1]);
	TEntry dstentry=Engine->CreateEntry(args[2]);
	if(!srcentry.IsValid()) return;

	vector<TWordID> wordcol;
	srcentry.FindAll(wordcol);

	for(vector<TWordID>::iterator it=wordcol.begin(); it!=wordcol.end(); it++)
		dstentry.Push(*it);

	if (rmflag)
		srcentry.Clear();
}
//---------------------------------------------------------------------------
void KIS_copytree::_Function(const vector<wstring>& args, bool rmflag){
	if(!AssertArgument(args, 3, 3)) return;
	if((!args[1].size())||(!args[2].size())) return;
	if((args[1].size()<=args[2].size())&&(args[2].substr(0,args[1].size())==args[1])){
		GetLogger().Diag(LOG_ERROR, kawari_log::E_KIS_ILLEGAL_COPY_OPERATION) << args[0] << RC.S(ERR_KIS_DICT_ILLEGAL_COPY_OPERATION);
		return;
	}

	const wstring &srcparent=args[1];
	const wstring &dstparent=(args[2]==L".")?wstring(L""):args[2];
	const int splen=(srcparent==L".")?0:static_cast<int>(srcparent.size());
	TEntry srcparententry=Engine->CreateEntry(srcparent);

	vector<TEntry> srccol;
	srcparententry.FindTree(srccol);

	// sort | uniq
	sort(srccol.begin(), srccol.end());
	vector<TEntry>::iterator end=unique(srccol.begin(), srccol.end());

	for(vector<TEntry>::iterator it=srccol.begin(); it!=end; it++){
		TEntry srcentry=(*it);
		const wstring src=srcentry.GetName();
		const wstring dst=dstparent+src.substr(splen,src.size()-splen);
		TEntry dstentry=Engine->CreateEntry(dst);

		vector<TWordID> wordcol;
		srcentry.FindAll(wordcol);
		for(vector<TWordID>::iterator wit=wordcol.begin(); wit!=wordcol.end(); wit++)
			dstentry.Push(*wit);

		if (rmflag)
			srcentry.Clear();
	}
}
//---------------------------------------------------------------------------
