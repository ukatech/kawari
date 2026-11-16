//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨7互換モード (Phase 9.3.0)
//
//   華和梨 Phase 7.5.0 の辞書の文法 (kawari.ini と、"エントリ : 単語, 単語" の
//   1 行 1 定義の辞書) を読み、9 の中間コードを作る。
//   9 のコンパイラ (kawari_compiler.cpp) には手を入れず、7.5.0 の字句規則と
//   文法をここで別に実装する。モードはゴースト単位 (TKawariEngine::IsLegacy7)。
//
//   7 と 9 で「同じ書き方なのに意味が違う」もの (3 引数の if、履歴参照の数え方、
//   素の ( ) など) は、7 用のノードが 7 の意味で実行することで解消する。
//
//---------------------------------------------------------------------------
#ifndef KAWARI7_H__
#define KAWARI7_H__
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <iostream>
#include "libkawari/kawari_code.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
class TKawariVM;
//---------------------------------------------------------------------------
// 7.5.0 の字句解析器 (kawari_lexer.cpp の TKawariLexer の移植)
//   区切り文字 : ; , ] ( ! & | )   空白 : 半角スペースとタブ   引用符 : "
//   マクロ : ${ と $(   (7 には $[ も ' も無い)
class TKawari7Lexer {
public:
	struct Token {
		enum Type {
			T_ERROR,		// 解釈できないマクロなど
			T_MACRO_ENTRY,	// ${
			T_MACRO_INLINE,	// $(
			T_SEPARATOR,	// 区切り文字 1 文字
			T_SPACE,		// 空白の連続
			T_SENTENCE		// 文字列 (引用符で囲まれたものは中身)
		};
		Type type;
		std::wstring str;
	};

	std::wstring src;
	size_t pos;

	TKawari7Lexer(const std::wstring &s) : src(s), pos(0) {}

	bool IsEnd(void) const { return pos>=src.size(); }
	wchar_t At(size_t i) const { return (i<src.size())?src[i]:L'\0'; }

	// 空白を読み飛ばした位置
	size_t AcceptSpace(void) const;
	// 引用符で囲まれた文字列の次の位置
	size_t AcceptQuotedString(wchar_t quote=L'"') const;
	// エントリ名の次の位置
	size_t AcceptEntryName(void) const;
	// エントリ名を separator 区切りで読む
	void GetEntryList(std::vector<std::wstring> &list, wchar_t separator=L'&');
	// トークンを 1 つ読む
	Token GetToken(void);
	// マクロを含まない単語を、separator のどれかの区切りが来るまで読む (マッチエントリ用)
	std::wstring GetWordString(const wchar_t *separator);
	// 引用符つき文字列から引用符を外す
	static std::wstring DecodeQuotedString(const std::wstring &s);
};

//---------------------------------------------------------------------------
// 7 の文字列。逆コンパイルが 7 の書き方 ("..." で囲み、中の " は \") になる
class TK7String : public TKVMCodeString {
public:
	virtual std::wstring DisCompile(void) const;
	TK7String(const std::wstring &str) : TKVMCodeString(str) {}
};

//---------------------------------------------------------------------------
// ${エントリ} / ${エントリ&エントリ}
//   7 では単語をそのまま(重複も数えて)一様に選ぶ。結果は履歴に積む。
class TK7EntryCall : public TKVMCode_base {
	std::vector<std::wstring> names;
public:
	const std::vector<std::wstring> &Names(void) const { return names; }
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	virtual bool Less(const TKVMCode_base &r_) const;
	virtual std::wostream &Debug(std::wostream &os, unsigned int level=0) const;
	TK7EntryCall(const std::vector<std::wstring> &n) : names(n) {}
	virtual ~TK7EntryCall() {}
};

//---------------------------------------------------------------------------
// ${0} ${1} ... 履歴参照 (7 では参照しても履歴に積まない)
class TK7HistoryCall : public TKVMCode_base {
	unsigned int no;
public:
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	virtual bool Less(const TKVMCode_base &r_) const;
	virtual std::wostream &Debug(std::wostream &os, unsigned int level=0) const;
	TK7HistoryCall(unsigned int n) : no(n) {}
	virtual ~TK7HistoryCall() {}
};

//---------------------------------------------------------------------------
// インラインスクリプトの 1 文 (list = 引数。list[0] がコマンド名)
class TK7Statement : public TKVMCodeList_base {
public:
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	// コマンド名が固定の文字列ならその中身。そうでなければ空
	std::wstring GetArg0(void) const;
	TK7Statement(const std::vector<TKVMCode_base *> &tmplist) : TKVMCodeList_base(tmplist) {}
	virtual ~TK7Statement() {}
protected:
	virtual const std::wstring GetName(void) const { return L"K7Statement"; }
};

//---------------------------------------------------------------------------
// $( 文 ; 文 ... )  (list = TK7Statement)
//   if / while / until / loop / foreach / ? / silent / NULL は 7 の意味で実行する。
class TK7InlineScript : public TKVMCodeList_base {
public:
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	TK7InlineScript(const std::vector<TKVMCode_base *> &tmplist) : TKVMCodeList_base(tmplist) {}
	virtual ~TK7InlineScript() {}
protected:
	virtual const std::wstring GetName(void) const { return L"K7InlineScript"; }
};

//---------------------------------------------------------------------------
// 7 の文法のコンパイラ (kawari_code.cpp の Compile 群の移植)
class TKawari7Compiler {
public:
	// 1 つの単語を、separator (0 なら無し) が来るまで読む (TKawariCodeWord::Compile2)
	// 戻り値 : 何か読めたら true
	static bool CompileWord(TKawari7Lexer &lex, std::vector<TKVMCode_base *> &codelist, wchar_t separator);

	// 辞書の行の続きから、カンマで区切られた次の単語を 1 つ得る (TKawariEngine::ParseKawariDict)
	// 空なら NULL
	static TKVMCode_base *ParseDictWord(TKawari7Lexer &lex);

	// 文字列全体を 1 つの単語として中間コード化する (TNS_KawariDictionary::StringToCode)
	// 空なら NULL
	static TKVMCode_base *CompileString(const std::wstring &src);

	// 文字列をエントリ名で使える文字列にする (TKawariEngine::EncodeEntryName)
	static std::wstring EncodeEntryName(const std::wstring &orgsen);

private:
	static bool compileText(TKawari7Lexer &lex, std::wstring &out);
	static bool compileWordParts(TKawari7Lexer &lex, std::vector<TKVMCode_base *> &codelist);
	static bool compileStatement(TKawari7Lexer &lex, std::vector<TKVMCode_base *> &codelist);
	static bool compileInlineScript(TKawari7Lexer &lex, std::vector<TKVMCode_base *> &codelist);
};

//---------------------------------------------------------------------------
// マッチエントリ 1 つ (["単語", ...] : 応答)
// 応答の単語は、エントリ entryname に入れてある。
struct TKawari7Match {
	std::vector<std::wstring> keywords;	// 全て system.Sentence に含まれていれば一致
	std::wstring entryname;
};

//---------------------------------------------------------------------------
// 7 モードのエンジンが持つ状態
struct TKawari7State {
	// マッチエントリ
	std::vector<TKawari7Match> matches;
	// kawari.ini の security (0〜3。無指定は 2)
	unsigned int securitylevel;
	// kawari.ini の locale (名前。空なら指定なし) と、その文字コード
	std::wstring locale;
	TKawariCharset localecs;

	TKawari7State() : securitylevel(2), localecs(KC_NULL) {}
};
//---------------------------------------------------------------------------
#endif // KAWARI7_H__
