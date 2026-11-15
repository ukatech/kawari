//---------------------------------------------------------------------------
//
// 雑用
//
//      Programed by NAKAUE.T (Meister)
//
//  2005.05.28 isdigit() problem fixed.
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstdlib>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
TMTRandomGenerator MTRandomGenerator;
//---------------------------------------------------------------------------
// isdigit() problem wa
static inline bool _isdigit(wchar_t ch){
	return ((L'0'<=ch) && (ch<=L'9'));
}
//---------------------------------------------------------------------------
// 整数値判定
bool IsInteger (const std::wstring &str){
	unsigned int max=static_cast<unsigned int>(str.length());
	if (!max) return false;
	unsigned int i=(str[0]==L'-')? 1:0;
	if (i>=max) return false;	// "-" だけ
	for(; i<max; i++)
		if (!_isdigit(str[i]))
			return false;
	return true;
}
//---------------------------------------------------------------------------
// 整数値から文字列を得る
wstring IntToString(int n)
{
	wchar_t buff[64];	// 多めに
	wchar_t *bp=buff;
	wstring ret;

	// INT_MIN の符号反転があふれないよう unsigned で扱う
	unsigned int u=(unsigned int)n;
	if(n<0) {
		ret+=L'-';
		u=0u-u;
	}

	do {
		*(bp++)=(wchar_t)((u%10)+L'0');
		u/=10;
	} while(u>0);

	while(bp!=buff) {
		bp--;
		ret+=*bp;
	}

	return(ret);
}
//---------------------------------------------------------------------------
// 64bit整数値から文字列を得る
wstring Int64ToString(kawari_int64 n)
{
	wchar_t buff[64];
	wchar_t *bp=buff;
	wstring ret;

	// 最小値の符号反転があふれないよう unsigned で扱う
	kawari_uint64 u=(kawari_uint64)n;
	if(n<0) {
		ret+=L'-';
		u=(kawari_uint64)0-u;
	}

	do {
		*(bp++)=(wchar_t)((u%10)+L'0');
		u/=10;
	} while(u>0);

	while(bp!=buff) {
		bp--;
		ret+=*bp;
	}

	return(ret);
}
//---------------------------------------------------------------------------
// 文字列から64bit整数値を得る (先頭の空白と符号を許し、数字でなくなるまで読む)
kawari_int64 StringToInt64(const std::wstring &str)
{
	std::wstring::size_type i=0, max=str.length();
	while((i<max)&&((str[i]==L' ')||(str[i]==L'\t')||(str[i]==L'\r')||(str[i]==L'\n'))) i++;

	bool neg=false;
	if((i<max)&&((str[i]==L'-')||(str[i]==L'+'))) {
		neg=(str[i]==L'-');
		i++;
	}

	// 絶対値は負の側の限界まで持てるよう unsigned で数え、範囲外は飽和させる
	const kawari_uint64 limit=neg ? (((kawari_uint64)1)<<63) : ((((kawari_uint64)1)<<63)-1);
	kawari_uint64 v=0;
	for(; (i<max)&&_isdigit(str[i]); i++) {
		unsigned int d=(unsigned int)(str[i]-L'0');
		if(v>(limit-d)/10) {
			v=limit;
			// 残りの桁は読み捨てる
			while((i<max)&&_isdigit(str[i])) i++;
			break;
		}
		v=v*10+d;
	}
	if(neg) return (kawari_int64)((kawari_uint64)0-v);
	return (kawari_int64)v;
}
//---------------------------------------------------------------------------
// パス表現の正規化
#if defined(KAWARI_MS)
#	define SEPARATOR L'\\'
#	define BADSEPARATOR L'/'
#else
#	define SEPARATOR L'/'
#	define BADSEPARATOR L'\\'
#endif

wstring CanonicalPath(const wstring &path){
	wstring ret=path;
	unsigned int len=static_cast<unsigned int>(ret.length());
	for (unsigned int i=0; i<len; i++)
		if (ret[i]==BADSEPARATOR) ret[i]=SEPARATOR;
	return ret;
}
//---------------------------------------------------------------------------
wstring CanonicalPath(const wstring &basepath, const wstring &path){
	static const wstring parentlink=wstring(L"..")+SEPARATOR;
	static const wstring currentlink=wstring(L".")+SEPARATOR;
	wstring p=CanonicalPath(path);
	wstring bp=CanonicalPath(basepath);

	if (IsAbsolutePath(p)||(!bp.size()))
		return path;
	if (!p.size())
		return basepath;
	// ルート ("/" や "\") の区切りは残す
	if ((bp.length()>1)&&(bp[bp.length()-1]==SEPARATOR))
		bp=bp.substr(0, bp.length()-1);
	while (bp.size()){
		if (p[0]!=L'.')
			break;
		
		if (StringCompare<wchar_t>(p, parentlink, 0, 3)==0){
			wstring::size_type pos=bp.rfind(SEPARATOR);
			if (pos==wstring::npos){
				bp=L"";
			}else if (pos==0){
				bp=bp.substr(0, 1);	// ルートより上には行かない
			}else{
				bp=bp.substr(0, pos);
			}
			p.erase(0, 3);
		}else if (StringCompare<wchar_t>(p, currentlink, 0, 2)==0){
			p.erase(0, 2);
		}else{
			// '.'で始まるファイル名？
			break;
		}
	}
	if (bp.size()&&(bp[bp.length()-1]!=SEPARATOR)) bp+=SEPARATOR;
	return bp+p;
}
//---------------------------------------------------------------------------
wstring PathToFileName(const wstring &path){
	wstring::size_type pos=path.rfind(SEPARATOR);
	if (pos==wstring::npos)
		return path;
	else
		return path.substr(pos+1);
}
//---------------------------------------------------------------------------
wstring PathToBaseDir(const wstring &path){
	wstring::size_type pos=path.rfind(SEPARATOR);
	if (pos==wstring::npos)
		return L"";
	else
		return path.substr(0, pos);
}
//---------------------------------------------------------------------------
