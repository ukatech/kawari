//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 字句解析
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.04.24  Phase 0.5     字句解析機分離
//  2001.05.20  Phase 5.1     インタープリタ・コンパイラ化
//
//---------------------------------------------------------------------------
#ifndef KAWARI_LEXER_H
#define KAWARI_LEXER_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
// 字句解析関係
//---------------------------------------------------------------------------
extern const string StartMacro;
extern const string StartSeparator;
extern const string StartWhiteSpace;
extern const string StartQuote;
extern const string StartAll;
//---------------------------------------------------------------------------
// 字句解析機
class TKawariLexer : public string {
public:

	// トークン
	struct TToken {
		// トークンの種類
		enum TTokenType {
			ttokError,			// エラー
			ttokSentence,		// 文字列
			ttokMacroEntry,		// ${
			ttokMacroInline,	// $(
			ttokSeparator,		// 区切り文字 :;,](!&|)
			ttokSpace,			// 空白文字列 " \t"
			ttokDummy
		} Type;

		// トークンの文字列
		string Str;
	};


	// 次に解析する文字の位置
	unsigned int Pos;

	// ユーザのメモ書き用
	unsigned int Pos2;


	TKawariLexer(void) : Pos(0) , Pos2(0) , string() {}

	TKawariLexer(const string& str) : Pos(0) , Pos2(0) , string(str) {}

	TKawariLexer(const char *str) : Pos(0) , Pos2(0) , string(str) {}

	string operator=(const TKawariLexer& r)
	{
		return(*((string*)&r));
	}

	TKawariLexer& operator=(const string& r)
	{
		*((string*)this)=r;
		Pos=0;
		Pos2=0;
		return(*this);
	}

	bool IsEnd(void)
	{
		return(Pos>=size());
	}


	// 解釈関数 Accept系 (Posを書き換えない)

	// 空白文字列を探す
	// 解釈できた最後の文字の次の位置が帰る
	unsigned int AcceptSpace(void) const;

	// 'または"で囲まれた文字列を探す
	// 解釈できた最後の文字の次の位置が帰る
	unsigned int AcceptQuotedString(char quote='"') const;

	// エントリ名を探す
	// 解釈できた最後の文字の次の位置が帰る
	unsigned int AcceptEntryName(void) const;


	// 解釈関数 Get系 (Posを書き換える)

	// エントリ名指定子列を一つ切り出す
	// Posは解釈した分だけ進められる
	void GetEntryList(vector<string>& entrytable,char separator='&');

	// トークンを一つ切り出す
	// Posは解釈した分だけ進められる
	TToken GetToken(void);

	// 単語文字列を一つ切り出す
	// Posは解釈した分だけ進められる
	string GetWordString(const char *separator_=NULL);


	// サービス関数

	// 'または"で囲まれた文字列からクォート文字をはずす
	static string DecodeQuotedString(const string& orgsen);

};
//---------------------------------------------------------------------------
#endif
