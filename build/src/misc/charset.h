//---------------------------------------------------------------------------
//
// 文字コード変換 (std::string <-> std::wstring)
//
//  2026.10.18  Phase 9.0.0   新設 (wchar_t化)
//  2026.10.22  Phase 9.1.1   EUC-JP/ISO-2022-JP/EUC-KR/GB2312追加
//
//---------------------------------------------------------------------------
#ifndef CHARSET_H
#define CHARSET_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
enum TKawariCharset {
	KC_NULL,	// 未指定 (自動判定)
	KC_SJIS,	// Shift_JIS (CP932)
	KC_UTF8,	// UTF-8
	KC_ACP,		// OSの既定コードページ (Win: CP_ACP, POSIX: UTF-8)
	KC_EUCJP,	// EUC-JP (Win: CP20932, POSIX: iconv)
	KC_JIS,		// ISO-2022-JP (Win: CP50220, POSIX: iconv)
	KC_EUCKR,	// EUC-KR (Win: CP949, POSIX: iconv)
	KC_GB2312,	// GB2312 (Win: CP936, POSIX: iconv)。実際にはGBKとして扱う
	KC_BIG5,	// Big5 (Win: CP950, POSIX: iconv)
	KC_LATIN1	// ISO-8859-1 (バイトと U+0000〜U+00FF を 1 対 1 に対応させる。自前で変換する)
};
//---------------------------------------------------------------------------
std::wstring SJIStoW(const std::string &s);
std::string WtoSJIS(const std::wstring &ws);
std::wstring UTF8toW(const std::string &s);
std::string WtoUTF8(const std::wstring &ws);
std::wstring ACPtoW(const std::string &s);
std::string WtoACP(const std::wstring &ws);
//---------------------------------------------------------------------------
// KC_NULLならDetectCharsetで判定
std::wstring MBtoW(const std::string &s, TKawariCharset cs);
std::string WtoMB(const std::wstring &ws, TKawariCharset cs);
//---------------------------------------------------------------------------
// 文字コード名は、大文字小文字と、'-' '_' '.' 空白を無視して照合する
// ("Shift_JIS" "shift-jis" "ShiftJIS" は同じ。YAYA と同じ規則)
// "Shift_JIS" "x-sjis" "SJIS" "windows-31j" "cp932" -> KC_SJIS
// "UTF-8" -> KC_UTF8。不明はKC_NULL
// (rccharset の「日本語かどうか」の判定用。Shift_JIS と UTF-8 以外は KC_NULL)
TKawariCharset CharsetFromName(const std::wstring &name);
// CharsetFromNameの名前に加えて、EUC-JP / ISO-2022-JP / EUC-KR / GB2312 / Big5 /
// ISO-8859-1 の名前と別名も受け付ける
// (KIS の *_charset、7互換モードの locale、SHIORI/SAORI/Direct SSTP の Charset ヘッダ)
TKawariCharset CharsetFromNameExt(const std::wstring &name);
// KC_SJIS -> L"Shift_JIS", KC_UTF8 -> L"UTF-8" (KC_ACP,KC_NULLは空文字列)
const wchar_t* CharsetName(TKawariCharset cs);
//---------------------------------------------------------------------------
bool IsValidUTF8(const std::string &s);
bool HasUTF8BOM(const std::string &s);
// BOMか、UTF-8として正当ならKC_UTF8、でなければKC_SJIS
TKawariCharset DetectCharset(const std::string &s);
//---------------------------------------------------------------------------
// ASCII限定の簡易変換 (0x80以上は'?')
std::wstring AtoW(const std::string &s);
std::string WtoA(const std::wstring &ws);
//---------------------------------------------------------------------------
#endif
