//---------------------------------------------------------------------------
//
// ファイルI/O・ディレクトリ・環境変数 (パスは常にstd::wstring)
//
//  2026.10.18  Phase 9.0.0   新設 (wchar_t化。_dirent.*を置き換え)
//
//---------------------------------------------------------------------------
#ifndef WFILE_H
#define WFILE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <cstdio>
#include "misc/charset.h"
//---------------------------------------------------------------------------
// Win: _wfopen, POSIX: fopen(WtoUTF8(path))
FILE* kawari_wfopen(const std::wstring &path, const wchar_t *mode);
//---------------------------------------------------------------------------
// POSIXのnarrow APIに渡す用 (UTF-8)
std::string PathToNative(const std::wstring &path);
//---------------------------------------------------------------------------
// バイナリで全読み込み
bool ReadWholeFile(const std::wstring &path, std::string &out);
// バイナリで全書き込み (append=trueなら追記)
bool WriteWholeFile(const std::wstring &path, const std::string &data, bool append=false);
//---------------------------------------------------------------------------
// 文字コード判定つきテキスト読み込み
// BOMを除去し、DetectCharsetに従ってwstring化する。cs_outに判定結果
bool ReadTextFile(const std::wstring &path, std::wstring &out, TKawariCharset *cs_out=NULL, bool *hasbom=NULL);
// テキスト書き込み。bom=trueならUTF-8のBOMを付ける(cs==KC_UTF8のときのみ)。append=trueなら追記
bool WriteTextFile(const std::wstring &path, const std::wstring &text, TKawariCharset cs=KC_UTF8, bool bom=false, bool append=false);
//---------------------------------------------------------------------------
// ディレクトリ列挙 ("." と ".." は含めない)。開けなければfalse
bool ListDirectory(const std::wstring &dir, std::vector<std::wstring> &names);
// パスの存在・種別確認。存在しなければfalse
bool PathStat(const std::wstring &path, bool *isdir, bool *isfile);
//---------------------------------------------------------------------------
// 環境変数の取得。存在しなければfalse
bool GetEnvironmentW(const std::wstring &name, std::wstring &value);
//---------------------------------------------------------------------------
#endif
