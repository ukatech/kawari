//---------------------------------------------------------------------------
//
// 雑用
//
//      Programed by NAKAUE.T (Meister)
//
//---------------------------------------------------------------------------
#ifndef MISC_H
#define MISC_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "misc/mt19937ar.h"
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
inline string StringTrim(const string& str)
{
	string::size_type linetop=str.find_first_not_of(" \t\r\n");
//	string::size_type lineend=str.find_last_not_of(" \t\r\n");
	string::size_type lineend=str.find_last_not_of('\0');
	lineend=str.find_last_not_of(" \t\r\n",lineend);
	if(linetop==string::npos) return(string(""));

	return(str.substr(linetop,lineend-linetop+1));
}
//---------------------------------------------------------------------------
// 2バイト文字の1バイト目か?
inline bool iskanji1st(char c)
{
#if 1
	// SJIS
	// 0x00-0x7f ASCII
	// 0x80-0x9f,0xe0-0xfc いわゆる全角1バイト目
	// 0xa0-0xdf いわゆる半角カナ
	return((unsigned char)((c^0x20)-0xa1)<=0x3b);
	// ちなみに2バイト目は0x40-0xfc
#else
	// EUC
	return(c<0);
#endif
}
//---------------------------------------------------------------------------
// 整数値から文字列を得る
string IntToString(int n);
//---------------------------------------------------------------------------
#endif
