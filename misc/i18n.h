//---------------------------------------------------------------------------
//
// "kawari" for are igai no nanika igaino nanika
// nise I18N (Internationalizaiton) module
//
//      I18N Module Programmed by Joo-won Jung (Sanori)
//      Kawari Programed by NAKAUE.T (Meister)
//
//  2001.09.08  Phase 7.0     Created
//  2001.10.05  Phase 7.0     WcharToStr, StrToWchar added
//	2002.01.11  Phase 7.3.1   Applied to KAWARI.kpcg Phase 7.3.1 (kpcg.I18Nhack)
//  2003.02.16  Phase 7.5.0   Support "GB2312"
//
//---------------------------------------------------------------------------
#ifndef I18N_H
#define I18N_H
//---------------------------------------------------------------------------
#include <string>
#include <map>

// Includes from kis_expr.cpp (for using wchar_t)
// some includes might not need, but I do not know platform/version differences...

#ifdef __GNUC__
	#if (__GNUC__ < 3)
		#include <cctype>
	#else
		#include <cctype>
		#include <cwchar>
	#endif
#else
	#include <cwctype>
	#include <cwchar>
#endif

using namespace std;
class TKawariI18N {
public:
	enum charset {
		ISO8859, SJIS, eucJP, eucKR, eucCN, BIG5, eucTW, UTF8,
		NONE };
private:
	map <string, string> locale_alias;
	map <string, charset> to_charset;
	string currentLocale;
	charset currentCharset;
	string formalLocaleName(const string& locale) const;
public:
	TKawariI18N(const string& locale = "C");

	string setLocale(const string& locale = "C");
	string getLocale() const;
	string getShioriCharset() const;
	charset getCharset(const string& locale) const;

	int moreBytes(int c) const;

	// StrToWchar: return the wide character of the 
	//             1st encoded character of string
	// If any error occurred, returned char will be NULL
	wchar_t StrToWchar(const string &str, int trail = -1) const;
	// WcharToStr: return encoded string of an wide character
	string WcharToStr(const wchar_t widechar) const;
	

};

#ifdef I18N_HACK

extern TKawariI18N lang;

#endif // I18N_HACK
//---------------------------------------------------------------------------
#endif
