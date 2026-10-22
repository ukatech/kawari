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
#include <algorithm>
#include "misc/i18n.h"
//---------------------------------------------------------------------------

#define MAX_TRAIL 5

#define LOWBYTE(x) (((unsigned int)(x)) & 0xFF)

TKawariI18N::TKawariI18N(const string& l) {
	// Initialize to_charset
	// The key should be lowercase
	to_charset["iso-8859-1"] = ISO8859;
	to_charset["iso-8859-2"] = ISO8859;
	to_charset["iso-8859-5"] = ISO8859;
	to_charset["sjis"] = SJIS;
	to_charset["shift_jis"] = SJIS;
	to_charset["eucjp"] = eucJP;
	to_charset["euc-jp"] = eucJP;
	to_charset["euckr"] = eucKR;
	to_charset["euc-kr"] = eucKR;
	to_charset["euctw"] = eucTW;
	to_charset["euc-tw"] = eucTW;
	to_charset["euccn"] = eucCN;
	to_charset["euc-cn"] = eucCN;
	to_charset["gb2312"] = eucCN;
	to_charset["big5"] = BIG5;
	to_charset["utf8"] = UTF8;
	to_charset["utf-8"] = UTF8;

	// initialize locale_alias
	locale_alias["c"] = 	"ISO-8859-1";	// wrong
	locale_alias["posix"] =	"ISO-8859-1";	// wrong
	locale_alias["en"] = 	"ISO-8859-1";
	locale_alias["fr"] = 	"ISO-8859-1";
	locale_alias["de"] = 	"ISO-8859-1";
	locale_alias["es"] = 	"ISO-8859-1";
	locale_alias["ja"] =	"Shift_JIS";
	locale_alias["ja_jp"] =	"Shift_JIS";
	locale_alias["sjis"] =	"Shift_JIS";
	locale_alias["eucjp"] = "EUC-JP";
	locale_alias["ko"] =	"EUC-KR";
	locale_alias["ko_kr"] =	"EUC-KR";
	locale_alias["euckr"] = "EUC-KR";
	locale_alias["zh"] =	"Big5";
	locale_alias["zh_tw"] =	"Big5";
	locale_alias["zh_cn"] =	"GB2312";
	locale_alias["gb2312"] =	"GB2312";

	setLocale(l);
}
string TKawariI18N::formalLocaleName(const string& locale) const {
	string ret = locale;
	int index = locale.find_last_of(".");
	if(index != string::npos) 
		ret=locale.substr(index+1);
	
	// lowercase
	transform(ret.begin(), ret.end(), ret.begin(), tolower);

	// check alias
	map<string,string>::const_iterator it;
	if ((it = locale_alias.find(ret)) != locale_alias.end()) {
		ret = it->second;
	}
	
	return ret;
}

TKawariI18N::charset TKawariI18N::getCharset(const string& locale) const {
	charset ret = NONE;

	// encoding specified
	string enc = locale;
	transform(enc.begin(), enc.end(), enc.begin(), tolower);
	map<string,charset>::const_iterator it;
	if ((it=to_charset.find(enc)) != to_charset.end()) {
		ret = it->second;
	} // else no available charset

	return ret;
}

string TKawariI18N::getShioriCharset() const
{
	// Nanika's supported SSTP Charsets: 
	// ASCII, Shift_JIS (default), ISO-2022-JP, EUC-JP, UTF-8
	if (currentCharset==UTF8)
		return "UTF-8";
	else if (currentCharset==ISO8859)
		return "ASCII";
	else if (currentCharset==eucJP)
		return "EUC-JP";
	else if (currentCharset==eucKR)
		return "EUC-KR";
	else if (currentCharset==eucCN)
		return "GB2312";
	else if (currentCharset==BIG5)
		return "Big5";
	else 
		return "Shift_JIS";
}

string TKawariI18N::getLocale() const {
	return currentLocale;
}

string TKawariI18N::setLocale(const string& l) {
	currentLocale = formalLocaleName(l);
	currentCharset = getCharset(currentLocale);
	return currentLocale;
}

int TKawariI18N::moreBytes(int c) const {
	const int UTF8moreChar[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

	int ret = 0;
	c = (c & 0xff);
	switch(currentCharset) {
		case SJIS:
			// from iskanji1st
			if ((unsigned char)((c^0x20)-0xa1)<=0x3b) {
				ret = 1;
			}
			break;

		case eucJP:
			if (((0xa1 <= c) && (c <= 0xfe)) || (c == 0x8e)) {
				ret = 1;
			} else if (c == 0x8f) {
				ret = 2;
			}
			break;

		case eucKR:
		case eucCN:
		case BIG5:
			if ((0xa1 <= c) && (c <= 0xfe)) { ret = 1; }
			break;

		case eucTW:
			if ((0xa1 <= c) && (c <= 0xfe)) {
				ret = 1;
			} else if (c == 0x8e) {
				ret = 3;
			}
			break;

		case UTF8:
			ret = UTF8moreChar[c];
			break;

		case ISO8859:
		default:
			ret = 0;
			break;
	}

	return ret;
}

wchar_t TKawariI18N::StrToWchar(const string &str, int trail) const
{
	wchar_t wcRet=NULL;

	if ((trail < 0) || (trail > MAX_TRAIL)) trail=moreBytes(str[0]);

	// Fail-safe error handler
	if (str.length() < (trail+1))
		return wcRet;

	switch(currentCharset)
	{
		case SJIS:
		case eucKR:
		case eucCN:
		case BIG5:

			if (trail)
				wcRet=(wchar_t) ((LOWBYTE(str[0]) << 8) | LOWBYTE(str[1]));
			else
				wcRet=(wchar_t) LOWBYTE(str[0]);
			break;

		case eucJP:
			switch(trail) {
			case 0 :
				wcRet=(wchar_t) LOWBYTE(str[0]);
				break;
			case 1 :
				wcRet=(wchar_t) ((LOWBYTE(str[0]) << 8) | LOWBYTE(str[1]));
				break;
			case 2 :	
				// Nise Wide-char :)
				// Map 8FA1A1 ~ 8FFEFE to 01A1 ~ 5EFE (01=A1-A0, 5E=FE-A0)
				wcRet= (wchar_t) (((LOWBYTE(str[1]) - 0x0A0) << 8) | LOWBYTE(str[2]));
			default:; // Error
			}
			break;

		case eucTW:
			// eucTW has total 16 planes (whee~), and it is IMPOSSIBLE to map
			// every planes into 2-bytes-wide-char. Hopefully, the plane 1 is
			// redundant with A1A1 ~ FEFE. This function only supports plane 1 ~ plane 4.
			switch(trail) {
			case 0 :
				wcRet=(wchar_t) LOWBYTE(str[0]);
				break;
			case 1 :
				wcRet=(wchar_t) ((LOWBYTE(str[0]) << 8) | LOWBYTE(str[1]));
				break;
			case 3 :	
				// Nise Wide-char :)
				// plane 1: Map 8EA1A1A1 ~ 8EA1FEFE to A1A1 ~ FEFE (Redundant)
				// plane 2: Map 8EA2A1A1 ~ 8EA2FEFE to 0101 ~ 5E5E (01=A1-A0, 5E=FE-A0)
				// plane 3: Map 8EA3A1A1 ~ 8EA3FEFE to A101 ~ FE5E (01=A1-A0, 5E=FE-A0)
				// plane 4: Map 8EA4A1A1 ~ 8EA4FEFE to 01A1 ~ 5EFE (01=A1-A0, 5E=FE-A0)

				if (LOWBYTE(str[1]) < 0xA5)	// bail out plane 5 ~ 16 (Error)
				{
					unsigned int first, second;
					
					if (str[1] & 0x01)	// plane 1, 3
						first=LOWBYTE(str[2]);
					else	// plane 2, 4
						first=LOWBYTE(str[2]) - 0x0A0;

					if (str[1] & 0x02)	// plane 2, 3
						second=LOWBYTE(str[3]) - 0x0A0;
					else	// plane 2, 4
						second=LOWBYTE(str[3]);

				wcRet=(wchar_t) ((first << 8) | second);
				}
				break;
			default: ; // Error
			}
			break;

		case UTF8:
			// Note: This code is not overlong-code-attack-safe. DO NOT use this code when
			// you are developing such security-important programming.
			// This also, more than 2-byte unicode codes are considered as error.

			switch (trail)
			{
			case 0: // 0000-007F
				wcRet=(wchar_t) LOWBYTE(str[0]);
				break;
			case 1: // 0080-07FF
				wcRet=(wchar_t) ((LOWBYTE(str[0]) & 0x01F) << 6);
				wcRet|=(wchar_t) (LOWBYTE(str[1]) & 0x03F);
				break;
			case 2: // 0800-FFFF
				wcRet=(wchar_t) ((LOWBYTE(str[0]) & 0x0F) << 12);
				wcRet|=(wchar_t) ((LOWBYTE(str[1]) & 0x03F) << 6);
				wcRet|=(wchar_t) (LOWBYTE(str[2]) & 0x03F);
				break;

			default:; // Error
			}
			break;

		case ISO8859:
		default:
			wcRet=(wchar_t) LOWBYTE(str[0]);
			break;
	}
	return wcRet;
}

string TKawariI18N::WcharToStr(const wchar_t widechar) const
{
	string strRet;

	unsigned char first, second;
	first= ((widechar >> 8) & 0xFF);
	second= (widechar & 0xFF);

	switch(currentCharset)
	{
		case SJIS:
		case eucKR:
		case eucCN:
		case BIG5:

			if (first)
			{
				strRet=(char)first;
				strRet+=(char)second;
			}
			else
				strRet=(char)second;
			break;

		case eucJP:
			if (first) {
				if ( (0x01 <= first) && (first <= 0x5E) )
				{
					strRet=(char)0x8F;
					strRet+=(char)(first + 0xA0);
				}
				else
					strRet=(char)first;

				strRet+=(char)second;
			}
			else
				strRet=(char)second;
			break;

		case eucTW:
			if (first) {
				if ( (0xA1 <= first) && (first <= 0xFE) )	// plane 1, 3
				{
					if ( (0xA1 <= second) && (second <= 0xFE) )	// plane 1
					{
						strRet=(char)first;
						strRet+=(char)second;
					}
					else	// plane 3
					{
						strRet=(char)0x8E;
						strRet+=(char)0xA3;
						strRet+=(char)first;
						strRet+=(char)(second+0xA0);
					}
				}
				else	// plane 2, 4
				{
					if ( (0xA1 <= second) && (second <= 0xFE) )	// plane 4
					{
						strRet=(char)0x8E;
						strRet+=(char)0xA4;
						strRet+=(char)(first+0xA0);
						strRet+=(char)second;
					}
					else	// plane 2
					{
						strRet=(char)0x8E;
						strRet+=(char)0xA2;
						strRet+=(char)(first+0xA0);
						strRet+=(char)(second+0xA0);
					}
				}
			}
			else
				strRet=(char)second;
			break;

		case UTF8:
			if (widechar < 0x0080)
				strRet=(char)(second & 0x7F);
			else if (widechar < 0x0800)
			{
				strRet= 0xC0 | ((unsigned char)(widechar >> 6) & 0x1F); // (110x xxxx)
				strRet+= 0x80 | ((unsigned char)(widechar) & 0x3F); // (10xx xxxx)
			}
			else // (widechar <= 0xFFFF)
			{
				strRet= 0xE0 | ((unsigned char)(widechar >> 12) & 0x0F); // (1110 xxxx)
				strRet+= 0x80 | ((unsigned char)(widechar >> 6) & 0x3F); // (10xx xxxx)
				strRet+= 0x80 | ((unsigned char)(widechar) & 0x3F); // (10xx xxxx)
			}
			break;


		case ISO8859:
		default:
			strRet=(char)second;
			break;
	}
	return strRet;
}

#ifdef I18N_HACK
// Global variable (Thread UNSAFE!) -_-;

TKawariI18N lang("ja_JP.SJIS");		// default locale is Japanese PC

#endif // I18N_HACK
//---------------------------------------------------------------------------
