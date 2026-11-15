//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 日時 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.14  Phase 0.50    First version
//  2001.05.19  Phase 5.1     少し強化
//  2001.06.17  Phase 6.0     もう少し強化
//  2001.08.08  Phase 6.2     ostrstreamバグ修正
//  2002.07.10  Phase 8.1.0   %s追加
//  2003.11.17  Phase 8.2.0   epocからの経過秒数をフォーマット出力可能に
//  2005.06.21  Phase 8.2.3   gcc3.xのwarning対応 (suikyo)
//                            mktime追加 (さとー)
//  2026.10.23  Phase 9.2.0   時刻を64bitの通算秒数で扱う (2038年問題の回避)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_date.h"
//---------------------------------------------------------------------------
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
using namespace std;
//---------------------------------------------------------------------------
// 時刻は 64bit の通算秒数で扱う (2038年問題の回避)。
// 32bit の mingw-w64 (msvcrt) は time_t が 32bit なので、64bit 版の CRT 関数を直接呼ぶ。
// localtime は 1970年より前では NULL を返し、mktime は 3000年を超えると -1 を返す。
#if defined(__MINGW32__)
static kawari_int64 NowTime(void)
{
	return (kawari_int64)_time64(NULL);
}
static struct tm *LocalTime(kawari_int64 t)
{
	__time64_t v=(__time64_t)t;
	return _localtime64(&v);
}
static kawari_int64 MakeTime(struct tm *tt)
{
	return (kawari_int64)_mktime64(tt);
}
#else
// 64bit の time_t を持つ環境 (LP64 の POSIX など)ではそのまま 64bit。
// time_t が 32bit の環境では、範囲外の値は localtime が NULL を返して空文字になる。
static kawari_int64 NowTime(void)
{
	return (kawari_int64)time(NULL);
}
static struct tm *LocalTime(kawari_int64 t)
{
	time_t v=(time_t)t;
	if((kawari_int64)v!=t) return NULL;
	return localtime(&v);
}
static kawari_int64 MakeTime(struct tm *tt)
{
	return (kawari_int64)mktime(tt);
}
#endif
//---------------------------------------------------------------------------
wstring KIS_date::Function(const vector<wstring>& args)
{
	kawari_int64 nowtt;
	if(args.size()>=3) {
		nowtt=StringToInt64(args[2]);
	} else {
		nowtt=NowTime();
	}
	struct tm *now=LocalTime(nowtt);
	// 負の値などでは NULL が返る (msvcrt)
	if(!now) return(L"");

	wstring format=L"%y/%m/%d %H:%M:%S";
	if(args.size()>=2) format=args[1];

	wostringstream ret;
	format+=L' ';	// ダミー
	for(unsigned int i=0;i<(format.size()-1);i++) {
		if(format[i]==L'%') {
			i++;
			switch(format[i]) {
			case L'd':	// 日 (01..31)
				ret << setw(2) << setfill(L'0') << now->tm_mday;
				break;
			case L'e':	// 日 (1..31)
				ret << now->tm_mday;
				break;
			case L'H':	// 時 (00..23)
				ret << setw(2) << setfill(L'0') << now->tm_hour;
				break;
			case L'j':	// 元日からの通算日 (001..366)
				ret << setw(3) << setfill(L'0') << (now->tm_yday+1);
				break;
			case L'J':	// 元日からの通算日 (1..366)
				ret << (now->tm_yday+1);
				break;
			case L'k':	// 時 (0..23)
				ret << now->tm_hour;
				break;
			case L'm':	// 月 (01..12)
				ret << setw(2) << setfill(L'0') << (now->tm_mon+1);
				break;
			case L'M':	// 分 (00..59)
				ret << setw(2) << setfill(L'0') << now->tm_min;
				break;
			case L'n':	// 月 (1..12)
				ret << (now->tm_mon+1);
				break;
			case L'N':	// 分 (0..59)
				ret << now->tm_min;
				break;
			case L'r':	// 秒 (0..59)
				ret << now->tm_sec;
				break;
			case L'S':	// 秒 (00..59)
				ret << setw(2) << setfill(L'0') << now->tm_sec;
				break;
			case L's':	// 1970/1/1 00:00:00 UTCからの秒数
				ret << Int64ToString(nowtt);	// VC6のwostreamは__int64を受け付けない
				break;
			case L'w':	// 曜日 (0..6) 日曜日が0
				ret << now->tm_wday;
				break;
			case L'y':	// 年 (2000...) 4桁
			case L'Y':	// 年 (2000...) 4桁
				ret << (now->tm_year+1900);
				break;
			case L'%':
				ret << L"%";	// VC6はwchar_tがunsigned shortなので、1文字を<<で流すと数値になる
				break;
			default:
				ret << L"%";	// VC6はwchar_tがunsigned shortなので、1文字を<<で流すと数値になる
				i--;
			}
		} else {
			ret << wstring(1, format[i]);
		}
	}

	return wstring(ret.str());
}
//---------------------------------------------------------------------------
wstring KIS_mktime::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 7, 7)) return(L"");

	struct tm tt;
	tt.tm_year =StringToInt(args[1])-1900;
	tt.tm_mon  =StringToInt(args[2])-1;
	tt.tm_mday =StringToInt(args[3]);
	tt.tm_hour =StringToInt(args[4]);
	tt.tm_min  =StringToInt(args[5]);
	tt.tm_sec  =StringToInt(args[6]);
	tt.tm_isdst=-1;	// 夏時間かどうかは mktime に判定させる (date の localtime と揃える)

	// 最低限の数値の正当性を保証する
	if(tt.tm_year<0)                    tt.tm_year=0;
	if((tt.tm_mon>11)||(tt.tm_mon<0))   tt.tm_mon=0;
	if((tt.tm_mday>31)||(tt.tm_mday<1)) tt.tm_mday=1;
	if((tt.tm_hour>23)||(tt.tm_hour<0)) tt.tm_hour=0;
	if((tt.tm_min>59)||(tt.tm_min<0))   tt.tm_min=0;
	if((tt.tm_sec>59)||(tt.tm_sec<0))   tt.tm_sec=0;

	return(Int64ToString(MakeTime(&tt)));

}
//---------------------------------------------------------------------------
