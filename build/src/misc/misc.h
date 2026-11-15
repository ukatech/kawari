//---------------------------------------------------------------------------
//
// 雑用
//
//      Programed by NAKAUE.T (Meister)
//
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#ifndef MISC_H
#define MISC_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstdlib>
#include <climits>
#include <cwchar>
using namespace std;
//---------------------------------------------------------------------------
#include "misc/mt19937ar.h"
#include "misc/charset.h"
#include "misc/wfile.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
extern TMTRandomGenerator MTRandomGenerator;
//---------------------------------------------------------------------------
// 0からnum-1までの乱数を発生
inline int Random(int num)
{
	return((int)(MTRandomGenerator.genrand_real2()*num));
}
//---------------------------------------------------------------------------
// 乱数のシードを設定
inline void SRandom(int seed)
{
	MTRandomGenerator.init_genrand((unsigned long)seed);
}
//---------------------------------------------------------------------------
// 文字列の前後の空白を取り除く
inline wstring StringTrim(const wstring& str)
{
	wstring::size_type linetop=str.find_first_not_of(L" \t\r\n");
//	wstring::size_type lineend=str.find_last_not_of(L" \t\r\n");
	wstring::size_type lineend=str.find_last_not_of(L'\0');
	lineend=str.find_last_not_of(L" \t\r\n",lineend);
	if(linetop==wstring::npos) return(wstring(L""));

	return(str.substr(linetop,lineend-linetop+1));
}
//---------------------------------------------------------------------------
// 整数値から文字列を得る
wstring IntToString(int n);
//---------------------------------------------------------------------------
// 文字列から整数値を得る (atoi相当。数値で始まらなければ0)
inline int StringToInt(const std::wstring &str)
{
	// long が 64bit の環境 (LP64) でも int の範囲に飽和させる
	long v=wcstol(str.c_str(), NULL, 10);
	if(v>INT_MAX) v=INT_MAX;
	if(v<INT_MIN) v=INT_MIN;
	return (int)v;
}

//---------------------------------------------------------------------------
// 64bit整数版 (時刻の通算秒数用。式の評価は int のまま)
wstring Int64ToString(kawari_int64 n);
// 文字列から64bit整数値を得る (StringToInt と同じ規則。範囲外は飽和)
kawari_int64 StringToInt64(const std::wstring &str);
//---------------------------------------------------------------------------
// 整数値判定
bool IsInteger (const std::wstring &str);
//---------------------------------------------------------------------------
// 真偽判定
inline bool IsTrue (const std::wstring &str) {
	return (!((str==L"")||(str==L"0")||(str==L"false")));
}
//---------------------------------------------------------------------------
// パス表現の正規化
#ifdef KAWARI_MS
#	define FILE_SEPARATOR		L'\\'
#	define FILE_SEPARATOR_ALT	L'/'
#else
#	define FILE_SEPARATOR		L'/'
#	define FILE_SEPARATOR_ALT	L'\\'
#endif
wstring CanonicalPath(const wstring &path);
wstring CanonicalPath(const wstring &basepath, const wstring &path);
inline bool IsAbsolutePath(const wstring &path){
#ifdef KAWARI_MS
	return (((path.size()>0)&&(path[0]==L'\\'))||((path.size()>1)&&(path[1]==L':')));
#else
	return ((path.size()>0)&&(path[0]==L'/'));
#endif
}
wstring PathToFileName(const wstring &path);
wstring PathToBaseDir(const wstring &path);
//---------------------------------------------------------------------------
template <class T>
int StringCompare(const basic_string<T> &str0, const basic_string<T> &str1, size_t s=0, size_t n=(size_t)-1){	// (size_t)-1 は basic_string<T>::npos。VC6はnposを既定引数に書くと内部エラーになる
#if defined(__GNUC__) && (__GNUC__ < 3)
	return str0.compare(str1, s, n);
#else
	return str0.compare(s, n, str1);
#endif
}
//---------------------------------------------------------------------------
#endif
