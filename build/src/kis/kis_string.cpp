//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 文字列操作 --
//
//      Programed by 
//
//  2002.03.25  Phase 8        First version
//  2002.07.10  Phase 8.1.0    reverseのバグ修正
//  2004.01.04  Phase 8.2.0    indexing policyを統一. bug fix.
//  2005.06.21  Phase 8.2.3    gcc3.xのwarning対応 (suikyo)
//  2008.01.27  Phase 8.2.6    gsubのバグ修正 (連続したpatternを""で置換漏れ)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <algorithm>
#include <string>
#include <cstdlib>
#include <vector>
//---------------------------------------------------------------------------
#include "kis/kis_string.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// インデックス値を正の値にする。負の場合、末尾から数える。
// 文字列長が0の時、インデックスが0の場合のみ0を返す。
// インデックスが範囲を超える場合は-1を返す。
//
// idx : インデックス
// len : 文字列長
static int normalizeIndex(int idx, int len){
	if(len<0) return -1;
	if(idx<0) idx+=len;
	if((!len)&&(!idx)) return idx;	// len=0, idx=0
	if((idx<0)||(idx>=len)) return -1;
	return idx;
}
//---------------------------------------------------------------------------
// substr target index [length]
// index : 負の場合、末尾から数える。末尾の文字が-1に相当。
//         文字列の範囲を超える場合、失敗("")。
// length : 開始位置から末尾方向に数える。
//          省略された場合、末尾まで
//          最後の文字を越える場合は最後の文字まで
//          負の値は失敗("")
wstring KIS_substr::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");

	const wstring target=(args[1]);
	int st=normalizeIndex(StringToInt(args[2]), static_cast<int>(target.length()));
	int len=(args.size()>=4)?StringToInt(args[3]):static_cast<int>(target.length());
	if((st<0)||(len<0)) return (L"");
	// len+stはintをあふれうる (lengthに巨大な値を渡された場合)
	if(len>static_cast<int>(target.length())-st) len=static_cast<int>(target.length())-st;
	return (target.substr(st, len));
}
//---------------------------------------------------------------------------
// length target
// targetの文字列長を返す。
wstring KIS_length::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 2)) return (L"");

	return IntToString(static_cast<int>((args[1]).length()));
}
//---------------------------------------------------------------------------
// target: target wstring
// pattern: pattern wstring
// idx: start index
// f: true if search forward.
// returns -1 if failed.
static int _match(const wstring &target, const wstring &pattern, int idx, bool f){
	if((!target.length())&&(!pattern.length())) return 0;
	if(!pattern.length()){
		// -(length+1) <= idx <= target.length()
		if(idx<0) idx+=static_cast<int>(target.length())+1;
		// 0 <= idx <= target.length()
		if((0<=idx)&&(idx<=static_cast<int>(target.length()))) return idx;
		else return -1;
	}else{
		idx=normalizeIndex(idx, static_cast<int>(target.length()));
		if(idx<0) return -1;
	}
	wstring::size_type pos=(f)?
		target.find(pattern, idx):target.rfind(pattern, idx);
	if (pos!=wstring::npos){
		return static_cast<int>(pos);
	}else{
		return -1;
	}
}
//---------------------------------------------------------------------------
// match target pattern [index]
// indexから末尾に向かって探す。
// 負のindexは末尾から数える。省略すると先頭の文字。
// indexが範囲を超えた場合、失敗("-1")
wstring KIS_match::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");
	return IntToString(_match((args[1]), (args[2]),
							  (args.size()>3) ? StringToInt(args[3]):0,
							  true));
}
//---------------------------------------------------------------------------
// rmatch target pattern [index]
// indexから先頭に向かって探す。
// 負のindexは末尾から数える。省略すると最後の文字。
// 範囲を超えた場合、失敗("-1")
wstring KIS_rmatch::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");
	return IntToString(_match((args[1]), (args[2]),
							  (args.size()>3) ? StringToInt(args[3]):-1,
							  false));
}
//---------------------------------------------------------------------------
// match_at target pattern [index]
// indexにpatternが存在すれば"1". 存在しなければ"".
// 負のindexは末尾から数える。省略すると先頭の文字。
// 範囲を超えた場合、失敗("")
wstring KIS_match_at::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");

	const wstring target=(args[1]);
	const wstring pattern=(args[2]);
	if(!pattern.length()){
		int idx=(args.size()>3)?StringToInt(args[3]):0;
		if(idx<0) idx+=static_cast<int>(target.length());
		if((0<=idx)&&(idx<=static_cast<int>(target.length()))) return (L"1");
		else return (L"");
	}
	int idx=(args.size()>3)?
		normalizeIndex(StringToInt(args[3]),static_cast<int>(target.length())):0;
	if(idx<0) return (L"");
	if((!target.length())&&(!pattern.length())&&(!idx)) return (L"1");
	if(StringCompare<wchar_t>(target, pattern, idx, pattern.size())==0)
		return (L"1");
	else
		return (L"");
}
//---------------------------------------------------------------------------
// char_at target index
// 文字列のindexの値を返す。範囲を超えた場合、失敗("")
wstring KIS_char_at::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");

	wstring target=(args[1]);
	int idx=normalizeIndex(StringToInt(args[2]), static_cast<int>(target.length()));
	// 空文字列の index 0 は normalizeIndex が通すので、ここで弾く
	if((idx<0)||(idx>=static_cast<int>(target.length()))) return (L"");
	wstring retstr;
	retstr+=target[idx];
	return (retstr);
}
//---------------------------------------------------------------------------
// sub target pattern subst [index]
// index以降のtarget中に見つかった最初のpatternをsubstで置換。
// index省略時は先頭の文字。indexが範囲を超えた場合、置換は起きない。
// patternが""の時、indexが指す文字の左側にsubstを挿入。
// その場合、indexはtarget文字列長まで許される。
wstring KIS_sub::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 4)) return (L"");

	wstring target=(args[1]);
	wstring pattern=(args[2]);
	wstring subst=(args[3]);
	if(!pattern.length()){
		int idx=(args.size()>4)? StringToInt(args[4]):0;
		if(idx<0) idx+=static_cast<int>(target.length());
		if(idx<0) return (args[1]);
		if(idx<static_cast<int>(target.length()))
			return (target.replace(idx, 0, subst));
		else if (idx==static_cast<int>(target.length()))
			return (target+subst);
		else
			return (args[1]);
	}
	int pos=_match(target, pattern,
				   ((args.size()>4) ? StringToInt(args[4]):0), true);
	if(pos<0) return (args[1]);
	return (target.replace(pos, pattern.length(), subst));
}
//---------------------------------------------------------------------------
// rsub target pattern subst [index]
// index以前のtarget中に見つかった最初のpatternをsubstで置換。
// index省略時は末尾の文字。indexが範囲を超えた場合、置換は起きない。
// patternが""の時、indexが指す文字の右側にsubstを挿入
// その場合、indexは-target文字列長-1まで許される。
wstring KIS_rsub::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 4)) return (L"");

	wstring target=(args[1]);
	wstring pattern=(args[2]);
	wstring subst=(args[3]);
	int pos=_match(target, pattern,
				   ((args.size()>4) ? StringToInt(args[4]):-1), false);
	if(pos<0) return (args[1]);
	return (target.replace(pos, pattern.length(), subst));
}
//---------------------------------------------------------------------------
// gsub target pattern subst [index]
// index以降のtarget中に見つかった全てのpatternをsubstで置換。
// index省略時は最初の文字。indexが範囲を超えた場合、置換は起きない。
wstring KIS_gsub::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 4)) return (L"");

	wstring target=(args[1]);
	wstring pattern=(args[2]);
	wstring subst=(args[3]);
	int tmp=(args.size()>4)?
		normalizeIndex(StringToInt(args[4]), static_cast<int>(target.length())):0;
	if(tmp<0) return (args[1]);
	wstring::size_type pos=tmp;
	if(!target.length()) return args[1];
	if((!pattern.length())&&(!subst.length())) return args[1];
	// int stride=max(subst.length(), 1);
	// int stride=max(subst.length(), static_cast<wstring::size_type>(1));
	int stride=static_cast<int>(subst.length());
	if(!pattern.length()) stride++;

	while (pos<target.length()){
		pos=target.find(pattern, pos);
		if(pos==wstring::npos) break;
		target=target.replace(pos, pattern.length(), subst);
		pos+=stride;
	}
	if(!pattern.length())
		target+=subst;
	return (target);
}
//---------------------------------------------------------------------------
// reverse target
wstring KIS_reverse::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 2)) return (L"");

	wstring target=(args[1]);
	reverse(target.begin(), target.end());
	return (target);
}
//---------------------------------------------------------------------------
// tr target chars substchars
wstring KIS_tr::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 4)) return (L"");

	wstring target=(args[1]);
	wstring search=(args[2]);
	wstring replace=(args[3]);

	wstring::size_type pos=0;
	wstring::size_type replen=replace.length();
	while (pos<target.length()){
		pos=target.find_first_of(search, pos);
		if (pos==wstring::npos) return (target);
		wstring::size_type index=search.find(target[pos]);
		if (index<replen) {
			target[pos]=replace[index];
			pos++;
		}else{
			target.erase(pos, 1);
		}
	}
	return (target);
}
//---------------------------------------------------------------------------
// compare string1 string2
wstring KIS_compare::Function(const vector<wstring>& args){
	if(!AssertArgument(args, 3)) return (L"");

	wstring str1=(args[1]);
	wstring str2=(args[2]);

	if (str1>str2) return (L"1");
	if (str1==str2) return (L"0");
	return (L"-1");
}
//---------------------------------------------------------------------------
