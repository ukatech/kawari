//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// コンパイラ依存
//
//  2026.10.18  Phase 9.0.0   UNICODE/WINVER定義を追加 (wchar_t化)
//
//      Programed by NAKAUE.T (Meister)
//
// そのうちautoconfに対応させたりするのだろうか?
//
//---------------------------------------------------------------------------
#ifndef CONFIG_H
#define CONFIG_H

//---------------------------------------------------------------------------
// autoconfigもどき

#ifdef NDEBUG
    #undef _STLP_DEBUG
#else
    #define _STLP_DEBUG 1
#endif

#if defined(__MSDOS__)
#	define KAWARI_MS
#elif defined(WIN32)||defined(_WIN32)||defined(_Windows)||defined(_MSC_VER)
#	define KAWARI_MS
#	define KAWARI_WIN32
#elif defined(__CYGWIN__)
#	define KAWARI_POSIX
#	define KAWARI_WIN32
#	define KAWARI_MS
#elif defined(linux)||defined(_linux)
#	define KAWARI_LINUX
#	define KAWARI_POSIX
#else
#	define KAWARI_POSIX
#endif

//---------------------------------------------------------------------------
#ifdef KAWARI_WIN32
	// Win32 APIはすべてW版を使う。windows.hを読む前に定義しておく
	// (Windows 2000 以降を対象とする)
#	ifndef UNICODE
#		define UNICODE
#	endif
#	ifndef _UNICODE
#		define _UNICODE
#	endif
#	ifndef WINVER
#		define WINVER 0x0500
#	endif
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0500
#	endif
#endif
//---------------------------------------------------------------------------
#ifdef _MSC_VER

	// STLを使うと必ず出る識別名255文字超Warningを抑制
	#pragma warning(disable:4786)

	// forのスコープがANSIに準拠していない問題対策(篠原君考案の手法)
	// 参考文献 http://www.ksky.ne.jp/~seahorse/cpp/loopvar.html
	// VC6(_MSC_VER<1300)だけ。VC7.1以降は標準に準拠しており、
	// 新しいSTLはキーワードのマクロ化を#errorで拒否する
	#if (_MSC_VER < 1300) && !defined(for)
	#define for if(0);else for
	#endif

	// cstdlib等がstd名前空間を使っていないために起こるエラーを抑制
	namespace std{}

	// POSIX opendir/readdir/closedirが無いため、代替品の指定
	#define NOHAVE_READDIR

#endif
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
	// BCCのvectorはデフォルトで最初に1KBメモリーを確保するので
	// メモリー使用効率が悪化する
	// 参照 : BCC5.5.1 memory.stlの111行目付近
	#define _RWSTD_CONTAINER_BUFFER_SIZE 4
	
	#define STRING_APPEND_INATOMIC

#endif
//---------------------------------------------------------------------------
#ifdef __MINGW32__
# if defined(_WIN32)
	// 他のヘッダよりも先に windows.h を読み込む 
	// （でないと生成バイナリが例外で落ちるみたいなので） 
# include <windows.h>
# endif
	#define STRING_APPEND_INATOMIC
#endif
//---------------------------------------------------------------------------
#if defined(__GNUC__)
#	if (__GNUC__ >= 3)
#		define HAVE_SSTREAM_H
#	endif
#endif
//---------------------------------------------------------------------------
// 64bit整数 (VC6には long long が無く、__int64 を使う)
#if defined(_MSC_VER) && (_MSC_VER < 1310)
	typedef __int64 kawari_int64;
	typedef unsigned __int64 kawari_uint64;
#else
	typedef long long kawari_int64;
	typedef unsigned long long kawari_uint64;
#endif
//---------------------------------------------------------------------------
#endif

