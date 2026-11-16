//---------------------------------------------------------------------------
//
// 通算秒数 (1970/1/1 00:00:00 UTC からの秒数) と年月日時分秒の変換
//
//  2026.11.17  Phase 9.3.3   作成。Win32 では CRT の localtime / mktime を使わない
//
// localtime / mktime や Win32 の時刻変換 API は、扱える年の範囲やタイムゾーンの
// 決め方が環境ごとに違い、範囲外の値では失敗する (msvcrt の localtime は 1970年より前で
// NULL を返し、mktime は 3000年を超えると -1 を返す。32bit の VC6 や mingw-w64 は
// time_t が 32bit)。そこで、通算秒数と年月日時分秒の相互変換は 64bit の暦計算で自前で行い
// (グレゴリオ暦。1970年より前も、3000年より後も同じ規則)、OS には
// 「その UTC 時刻でのローカルタイムの UTC オフセット」だけを尋ねる (yaya-shiori と同じ方式)。
//
// ヘッダだけで完結していて、プロセス全体で共有する状態は持たない。
// Win32 (_WIN32。Cygwin は time_t が 64bit なので対象外) では LocalOffset 以下も使える。
//
//---------------------------------------------------------------------------
#ifndef EPOCHTIME_H
#define EPOCHTIME_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <time.h>
#include <string.h>
#if defined(_WIN32)
#	include <windows.h>	// MinGW は config.h が読むが、MSVC / VC6 は読まない
#endif
//---------------------------------------------------------------------------
namespace epochtime {
//---------------------------------------------------------------------------
// 暦計算 (OS に依存しない)
//---------------------------------------------------------------------------
// 切り捨てではなく負の無限大方向への割り算 (b>0)
inline kawari_int64 FloorDiv(kawari_int64 a, kawari_int64 b)
{
	kawari_int64 q=a/b;
	if((a%b)<0) q--;
	return q;
}
//---------------------------------------------------------------------------
// 1970/1/1 からの通算日数 (グレゴリオ暦の外挿。month は 1-12)
inline kawari_int64 DaysFromCivil(kawari_int64 y, int m, int d)
{
	if(m<=2) y-=1;
	kawari_int64 era=FloorDiv(y, 400);
	kawari_int64 yoe=y-era*400;
	kawari_int64 doy=(153*(m+(m>2 ? -3 : 9))+2)/5+d-1;
	kawari_int64 doe=yoe*365+yoe/4-yoe/100+doy;
	return era*146097+doe-719468;
}
//---------------------------------------------------------------------------
// DaysFromCivil の逆
inline void CivilFromDays(kawari_int64 z, kawari_int64 &y, int &m, int &d)
{
	z+=719468;
	kawari_int64 era=FloorDiv(z, 146097);
	kawari_int64 doe=z-era*146097;
	kawari_int64 yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
	kawari_int64 doy=doe-(365*yoe+yoe/4-yoe/100);
	kawari_int64 mp=(5*doy+2)/153;
	d=static_cast<int>(doy-(153*mp+2)/5+1);
	m=static_cast<int>(mp<10 ? mp+3 : mp-9);
	y=yoe+era*400+(m<=2 ? 1 : 0);
}
//---------------------------------------------------------------------------
// 扱う通算秒数の範囲 (±約 9.5 億年)。tm_year が int に収まる
inline kawari_int64 Limit(void)
{
	return static_cast<kawari_int64>(30000000)*1000000000;
}
//---------------------------------------------------------------------------
// 通算秒数 utc を、UTC からの offset 秒だけずれた時刻の年月日時分秒にする。
// isdst は tm_isdst に入れる値。範囲外 (Limit) なら false
inline bool ToTM(kawari_int64 utc, int offset, int isdst, struct tm &out)
{
	memset(&out, 0, sizeof(out));
	if((utc>Limit())||(utc<-Limit())) return false;

	kawari_int64 lt=utc+offset;
	kawari_int64 days=FloorDiv(lt, 86400);
	int rem=static_cast<int>(lt-days*86400);

	kawari_int64 y;
	int m, d;
	CivilFromDays(days, y, m, d);

	out.tm_sec=rem%60;
	out.tm_min=(rem/60)%60;
	out.tm_hour=rem/3600;
	out.tm_mday=d;
	out.tm_mon=m-1;
	out.tm_year=static_cast<int>(y-1900);
	out.tm_wday=static_cast<int>((days%7+11)%7);	// 1970/1/1 は木曜日 (負の日数でも 0..6)
	out.tm_yday=static_cast<int>(days-DaysFromCivil(y, 1, 1));
	out.tm_isdst=isdst;
	return true;
}
//---------------------------------------------------------------------------
// 年月日時分秒を、その時刻を UTC とみなしたときの通算秒数にする (オフセットは引かない)。
// 月・日・時・分・秒が範囲を超えていても線形に繰り上げる (mktime と同じ)。範囲外 (Limit) なら false
inline bool FromTM(const struct tm &in, kawari_int64 &out)
{
	kawari_int64 y=static_cast<kawari_int64>(in.tm_year)+1900;
	kawari_int64 carry=FloorDiv(in.tm_mon, 12);
	y+=carry;
	int mon=static_cast<int>(in.tm_mon-carry*12);

	kawari_int64 t=(DaysFromCivil(y, mon+1, 1)+in.tm_mday-1)*86400
		+static_cast<kawari_int64>(in.tm_hour)*3600
		+static_cast<kawari_int64>(in.tm_min)*60
		+in.tm_sec;
	if((t>Limit())||(t<-Limit())) return false;
	out=t;
	return true;
}
//---------------------------------------------------------------------------
// ローカルタイム (UTC オフセットを返す関数 raw を渡す。OS に依存しない)
//---------------------------------------------------------------------------
// utc の時点でのローカルタイムの UTC オフセット (秒。東が正) を返す関数
typedef int (*OffsetFunc)(kawari_int64 utc);
//---------------------------------------------------------------------------
// utc の時点でのオフセットを返す。isdst (NULL 可) には夏時間なら 1 を入れる。
// 夏時間かどうかは raw から直接分からないので、その年の 1 月 1 日と 7 月 1 日の
// オフセットの小さいほうを標準時とみなして比べる
inline int OffsetAndDst(kawari_int64 utc, OffsetFunc raw, int *isdst)
{
	int offset=raw(utc);
	if(isdst) {
		kawari_int64 y;
		int m, d;
		CivilFromDays(FloorDiv(utc+offset, 86400), y, m, d);
		int o1=raw(DaysFromCivil(y, 1, 1)*86400);
		int o2=raw(DaysFromCivil(y, 7, 1)*86400);
		int stdoffset=(o1<o2) ? o1 : o2;
		*isdst=(offset>stdoffset) ? 1 : 0;
	}
	return offset;
}
//---------------------------------------------------------------------------
// 通算秒数をローカルタイムの年月日時分秒にする。範囲外なら false
inline bool LocalTMWith(kawari_int64 utc, OffsetFunc raw, struct tm &out)
{
	if((utc>Limit())||(utc<-Limit())) return false;
	int isdst=0;
	int offset=OffsetAndDst(utc, raw, &isdst);
	return ToTM(utc, offset, isdst, out);
}
//---------------------------------------------------------------------------
// ローカルタイムの年月日時分秒を通算秒数にする。範囲外なら false。
// 夏時間の切り替えの前後であいまいなときは、一度求めたオフセットで引き直した時刻のオフセットを使う
inline bool MakeLocalWith(const struct tm &in, OffsetFunc raw, kawari_int64 &out)
{
	kawari_int64 t;
	if(!FromTM(in, t)) return false;
	int o1=raw(t);
	int o2=raw(t-o1);
	out=t-o2;
	return true;
}
//---------------------------------------------------------------------------
#if defined(_WIN32)
//---------------------------------------------------------------------------
// Win32: ローカルタイムの UTC オフセットは OS に尋ねる
//---------------------------------------------------------------------------
// 1601/1/1 から 1970/1/1 までの秒数
inline kawari_int64 FileTimeEpochDiff(void)
{
	return static_cast<kawari_int64>(11644473)*1000+600;	// VC6 は 32bit を超える整数リテラルを嫌う
}
//---------------------------------------------------------------------------
inline kawari_int64 FileTimeToEpoch(const FILETIME &ft)
{
	kawari_uint64 ticks=(static_cast<kawari_uint64>(ft.dwHighDateTime)<<32)|static_cast<kawari_uint64>(ft.dwLowDateTime);
	return static_cast<kawari_int64>(ticks/10000000)-FileTimeEpochDiff();
}
//---------------------------------------------------------------------------
inline FILETIME EpochToFileTime(kawari_int64 t)
{
	kawari_uint64 ticks=static_cast<kawari_uint64>(t+FileTimeEpochDiff())*10000000;
	FILETIME ft;
	ft.dwLowDateTime=static_cast<DWORD>(ticks&0xFFFFFFFF);
	ft.dwHighDateTime=static_cast<DWORD>(ticks>>32);
	return ft;
}
//---------------------------------------------------------------------------
// 現在の通算秒数
inline kawari_int64 Now(void)
{
	FILETIME ft;
	::GetSystemTimeAsFileTime(&ft);
	return FileTimeToEpoch(ft);
}
//---------------------------------------------------------------------------
// UTC の SYSTEMTIME を、現在のタイムゾーンのローカルタイムにする。
// SystemTimeToTzSpecificLocalTimeEx (Vista 以降。年ごとの夏時間の規則が効く) があればそれを、
// 無ければ SystemTimeToTzSpecificLocalTime を使う (どちらも実行時に探す)
inline bool SystemTimeUtcToLocal(const SYSTEMTIME &utc, SYSTEMTIME &local)
{
	typedef BOOL (WINAPI *PFN_Ex)(const void *pDynamicTzInfo, const SYSTEMTIME *pUtc, SYSTEMTIME *pLocal);
	typedef BOOL (WINAPI *PFN_Old)(const TIME_ZONE_INFORMATION *pTzInfo, const SYSTEMTIME *pUtc, SYSTEMTIME *pLocal);

	HMODULE hKernel=::GetModuleHandleA("kernel32");
	if(!hKernel) return false;
	// void * を経由するのは、FARPROC から別の型の関数ポインタへの直接のキャストが警告になるため
	PFN_Ex pEx=(PFN_Ex)(void *)::GetProcAddress(hKernel, "SystemTimeToTzSpecificLocalTimeEx");
	if(pEx&&pEx(NULL, &utc, &local)) return true;
	PFN_Old pOld=(PFN_Old)(void *)::GetProcAddress(hKernel, "SystemTimeToTzSpecificLocalTime");
	if(pOld&&pOld(NULL, &utc, &local)) return true;
	return false;
}
//---------------------------------------------------------------------------
// 変換 API が使えないときの代わり。今のバイアスから求める
inline int LocalOffsetByBias(void)
{
	TIME_ZONE_INFORMATION tz;
	DWORD r=::GetTimeZoneInformation(&tz);
	LONG bias=tz.Bias;
	if(r==TIME_ZONE_ID_DAYLIGHT) bias+=tz.DaylightBias;
	else if(r!=TIME_ZONE_ID_INVALID) bias+=tz.StandardBias;
	return static_cast<int>(-bias*60);
}
//---------------------------------------------------------------------------
// utc の時点でのローカルタイムの UTC オフセット (秒。東が正)
inline int LocalOffsetRaw(kawari_int64 utc)
{
	// FILETIME / SYSTEMTIME で表せる範囲に収める (範囲外はその端の規則を使う)
	const kawari_int64 lo=DaysFromCivil(1602, 1, 1)*86400;
	const kawari_int64 hi=DaysFromCivil(30000, 1, 1)*86400;
	kawari_int64 t=utc;
	if(t<lo) t=lo;
	if(t>hi) t=hi;

	FILETIME ft=EpochToFileTime(t);
	SYSTEMTIME su, sl;
	if(::FileTimeToSystemTime(&ft, &su)&&SystemTimeUtcToLocal(su, sl)) {
		kawari_int64 lt=DaysFromCivil(sl.wYear, sl.wMonth, sl.wDay)*86400
			+sl.wHour*3600+sl.wMinute*60+sl.wSecond;
		return static_cast<int>(lt-t);
	}
	return LocalOffsetByBias();
}
//---------------------------------------------------------------------------
// 通算秒数を現在のタイムゾーンのローカルタイムの年月日時分秒にする。範囲外なら false
inline bool LocalTM(kawari_int64 utc, struct tm &out)
{
	return LocalTMWith(utc, LocalOffsetRaw, out);
}
//---------------------------------------------------------------------------
// 現在のタイムゾーンのローカルタイムの年月日時分秒を通算秒数にする。範囲外なら false
inline bool MakeLocal(const struct tm &in, kawari_int64 &out)
{
	return MakeLocalWith(in, LocalOffsetRaw, out);
}
//---------------------------------------------------------------------------
#endif	// _WIN32
//---------------------------------------------------------------------------
}	// namespace epochtime
//---------------------------------------------------------------------------
#endif
