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
//  2026.11.17  Phase 9.3.2   Win32 は CRT の localtime / mktime をやめ、自前の暦計算で変換する
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
// Win32 (MinGW / MSVC / VC6、32bit も 64bit も) は、CRT の localtime / mktime を使わず、
// 64bit の暦計算 + OS のタイムゾーン情報で変換する (misc/epochtime.h)。CRT の限界
// (msvcrt は 1970年より前と 3000年より後が変換できず、VC6 と 32bit の mingw-w64 は time_t が 32bit)
// に縛られない。環境変数 TZ は見ない (OS のタイムゾーン設定に従う)。
#if defined(_WIN32)
#include "misc/epochtime.h"
static kawari_int64 NowTime(void)
{
	return epochtime::Now();
}
static bool LocalTime(kawari_int64 t, struct tm &out)
{
	return epochtime::LocalTM(t, out);
}
static bool MakeTime(const struct tm &tt, kawari_int64 &out)
{
	return epochtime::MakeLocal(tt, out);
}
#else
// 64bit の time_t を持つ環境 (LP64 の POSIX など)ではそのまま 64bit。
// time_t が 32bit の環境では、範囲外の値は localtime が NULL を返して空文字になる。
static kawari_int64 NowTime(void)
{
	return (kawari_int64)time(NULL);
}
static bool LocalTime(kawari_int64 t, struct tm &out)
{
	time_t v=(time_t)t;
	if((kawari_int64)v!=t) return false;
	struct tm *p=localtime(&v);
	if(!p) return false;
	out=*p;
	return true;
}
static bool MakeTime(const struct tm &tt, kawari_int64 &out)
{
	struct tm copy=tt;	// mktime は引数を正規化して書き換える
	out=(kawari_int64)mktime(&copy);	// 失敗は -1 のまま返す
	return true;
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
	struct tm now;
	// 変換できない値 (扱える範囲を超えたもの、32bit の time_t の環境の範囲外) は空文字
	if(!LocalTime(nowtt, now)) return(L"");

	wstring format=L"%y/%m/%d %H:%M:%S";
	if(args.size()>=2) format=args[1];

	wostringstream ret;
	format+=L' ';	// ダミー
	for(unsigned int i=0;i<(format.size()-1);i++) {
		if(format[i]==L'%') {
			i++;
			switch(format[i]) {
			case L'd':	// 日 (01..31)
				ret << setw(2) << setfill(L'0') << now.tm_mday;
				break;
			case L'e':	// 日 (1..31)
				ret << now.tm_mday;
				break;
			case L'H':	// 時 (00..23)
				ret << setw(2) << setfill(L'0') << now.tm_hour;
				break;
			case L'j':	// 元日からの通算日 (001..366)
				ret << setw(3) << setfill(L'0') << (now.tm_yday+1);
				break;
			case L'J':	// 元日からの通算日 (1..366)
				ret << (now.tm_yday+1);
				break;
			case L'k':	// 時 (0..23)
				ret << now.tm_hour;
				break;
			case L'm':	// 月 (01..12)
				ret << setw(2) << setfill(L'0') << (now.tm_mon+1);
				break;
			case L'M':	// 分 (00..59)
				ret << setw(2) << setfill(L'0') << now.tm_min;
				break;
			case L'n':	// 月 (1..12)
				ret << (now.tm_mon+1);
				break;
			case L'N':	// 分 (0..59)
				ret << now.tm_min;
				break;
			case L'r':	// 秒 (0..59)
				ret << now.tm_sec;
				break;
			case L'S':	// 秒 (00..59)
				ret << setw(2) << setfill(L'0') << now.tm_sec;
				break;
			case L's':	// 1970/1/1 00:00:00 UTCからの秒数
				ret << Int64ToString(nowtt);	// VC6のwostreamは__int64を受け付けない
				break;
			case L'w':	// 曜日 (0..6) 日曜日が0
				ret << now.tm_wday;
				break;
			case L'y':	// 年 (2000...) 4桁
			case L'Y':	// 年 (2000...) 4桁
				ret << (now.tm_year+1900);
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

	// 変換できない (扱える範囲を超えた) ときは -1
	kawari_int64 result;
	if(!MakeTime(tt, result)) result=-1;
	return(Int64ToString(result));

}
//---------------------------------------------------------------------------
