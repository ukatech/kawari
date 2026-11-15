//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 字句解析
//
//      Programed by Suikyo
//
//  2002.04.18  Phase 8.0.0   書き直し
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_lexer.h"
#include "misc/misc.h"
#include "misc/charset.h"
#include "libkawari/kawari_crypt.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_rc.h"
using namespace kawari::resource;
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
using namespace std;
//---------------------------------------------------------------------------
// ASCII(0x00-0x7F)の表。0x80以上の文字は全モードで「リテラルに使える文字」として扱う
//
// EntryID ::= ( [0-9A-Za-z._?@] | NAChar ) +
static char id_map[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

// BareLiteral.mode1 ::= ( [#x21-#x7E] - ["$'();] | NAChar | [ \t] ) +
static char literal_map1[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

// BareLiteral.mode2 ::= ( [#x21-#x7E] - ["$'(),] | NAChar ) +
static char literal_map2[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

// BareLiteral.mode3 ::= ( [#x21-#x7E] - ["$'()] | NAChar ) +
static char literal_map3[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

static char *lex_map[4] = { id_map, literal_map1, literal_map2, literal_map3 };

// ベア文字列に使える文字か (0x80以上は常に使える)
static inline bool IsLexChar(int m, wchar_t ch){
	unsigned long c=(unsigned long)ch;
	return (c>=0x80) ? true : (lex_map[m][c]!=0);
}

//---------------------------------------------------------------------------
// コンストラクタ
TKawariLexer::TKawariLexer (
	wistream &input, TKawariLogger &lgr, wstring filename, bool preprocess, int lineno,
	TKawariCharset cryptcharset)
	 : pp(new TKawariPreProcessor(input, lineno, preprocess, cryptcharset)), fn(filename), logger(lgr) { }

//---------------------------------------------------------------------------
// デストラクタ
TKawariLexer::~TKawariLexer(void){
	if (pp) delete pp;
}

//---------------------------------------------------------------------------
// 次のベアリテラルトークンを得る
// 次がベアリテラルでなかったら、""が返る。予め分かっている場合にだけ使うこと。
wstring TKawariLexer::getLiteral(Mode m){
	if (!hasNext()) return (L"");

	wchar_t ch=0;
	wstring ret;
	ret.reserve(20);	// こんくらいですか
	while(pp->getch(ch)){
		if (IsLexChar((int)m, ch)){
			ret+=ch;
		} else {
			pp->unget();
			break;
		}
	}
	if(((m==LITERAL_MODE)&&(ch==L','))||
	   ((m==LITERAL_MODE3)&&(ch==L')'))){
		// デリミタ前の空白は削除
		wstring::size_type litend=ret.find_last_not_of(L" \t\x0d\x0a");
		ret=ret.substr(0, litend+1);
	}
//	cout << "getLiteral(" << m << ") = " << ret << endl;
	return ret;
}

//---------------------------------------------------------------------------
// 次のクォートリテラルトークン(非デコード)を返す
// 次がクォートリテラルでなかったら""が返る。
wstring TKawariLexer::getQuotedLiteral(void){
	static const wstring QUOTE(L"\"'");			// クォート文字列
	if (!hasNext()) return (L"");

	wchar_t ch=0, quote;
	pp->peek(ch);
	if (QUOTE.find(ch)==wstring::npos){
		return L"";
	}else{
		quote = ch;
		pp->getch(ch);
	}
	wstring result;
	result.reserve(20);	// こんなもん？
	result+=quote;

	bool closed = false;
	while(pp->getch(ch)) {
		if(ch==quote){
			// 終了
			result+=ch;
			closed=true;
			break;
		}else if(ch==L'\\'){
			// エスケープ
			result+=ch;
			pp->getch(ch);
			if ((ch==quote)||(ch==L'\\'))
				result+=ch;
			else
				pp->unget();
		}else if(0x0a==ch){
			// 改行
			error(ERR_LEXER_EOL_IN_QUOTED_STRING);
			closed=true;
			break;
		}else{
			result+=ch;
		}
	}
	if (!closed)
		// 解析中にEOFに達してしまった
		error(ERR_LEXER_EOF_IN_QUOTED_STRING);
	return result;
}
//---------------------------------------------------------------------------
// 'または"で囲まれた文字列からクォート文字をはずす
wstring TKawariLexer::DecodeQuotedString(const wstring& orgsen)
{
	if(orgsen.size()==0) return(L"");

	wchar_t quote=orgsen[0];

	unsigned int pos=1;
	wstring retstr;
	retstr.reserve(orgsen.size());

	unsigned int max=static_cast<unsigned int>(orgsen.size());
	while(pos<max) {
		if(orgsen[pos]==quote) break;

		if(orgsen[pos]==L'\\') {
			if(((pos+1)<orgsen.size())&&((orgsen[pos+1]==quote)||(orgsen[pos+1]==L'\\')))
				pos++;
		}

		retstr+=orgsen[pos++];
	}

	return(retstr);
}
//---------------------------------------------------------------------------
// !!! check type, before use this !!!
// [0-9]+
// 整数文字列
wstring TKawariLexer::getDecimalLiteral(void){
	wchar_t ch;
	wstring result;
	while(pp->getch(ch)){
		if ((ch>=L'0')&&(ch<=L'9')){
			result+=ch;
		}else{
			pp->unget();
			break;
		}
	}
	return result;
}

//---------------------------------------------------------------------------
// 1文字スキップ
int TKawariLexer::skip(void){
	wchar_t ch;
	if (pp->getch(ch))
		return (int)ch;
	else
		return -1;
}

//---------------------------------------------------------------------------
// 文字からトークンタイプを得る
inline Token::Type TKawariLexer::checkType(Mode m, wchar_t ch) const{
	static const wstring CRLF(L"\x0d\x0a");		// CRLF
	static const wstring WHITESPACE(L" \t");	// 空白文字
	static const wstring QUOTE(L"\"'");			// クォート文字列

	if (pp->IsWaitingModeSwitch()){
		return Token::T_MODESWITCH;
	}else if (IsLexChar((int)m, ch)){
		// ベア文字列
		return Token::T_LITERAL;
	}else if (QUOTE.find(ch)!=wstring::npos){
		// クォート文字列
		return Token::T_QLITERAL;
	}else if (WHITESPACE.find(ch)!=wstring::npos){
		// 空白文字
		return Token::T_SPACE;
	}else if (CRLF.find(ch)!=wstring::npos){
		// 改行文字
		return Token::T_EOL;
	}else{
		// 特殊文字
		return Token::Type(int (ch) & 0xFF);
	}
}

//---------------------------------------------------------------------------
Token::Type TKawariLexer::peek(Mode m){
	if (!hasNext())
		return (Token::T_EOF);

	wchar_t ch;
	if (!pp->peek(ch))
		return (Token::T_EOF);
	return checkType(m, ch);
}

//---------------------------------------------------------------------------
Token TKawariLexer::next(Mode m){
	static const wchar_t CR = 0x0d;				// CR
	static const wchar_t LF = 0x0a;				// LF
	static const wstring WHITESPACE(L" \t");	// 空白文字
	static const wstring QUOTE(L"\"'");			// クォート文字列
	Token result;
	if (!hasNext()) return (result.set(Token::T_EOF));

	wchar_t ch;
	// 最終行の読み込みでEOFになると、hasNext()が真でも読めないことがある
	if (!pp->getch(ch)) return (result.set(Token::T_EOF));
	if (IsLexChar((int)m, ch)){
		// ベア文字列
		pp->unget();
		wstring str=getLiteral(m);
		result.set(Token::T_LITERAL, str);
	}else if (QUOTE.find(ch)!=wstring::npos){
		// クォート文字列
		pp->unget();
		result.set(Token::T_QLITERAL, getQuotedLiteral());
	}else if (WHITESPACE.find(ch)!=wstring::npos){
		// 空白文字
		wstring ws;
		ws.reserve(5); // このぐらい？
		ws+=ch;
		while (pp->getch(ch)){
			if (WHITESPACE.find(ch)==wstring::npos){
				pp->unget();
				break;
			}
			ws += ch;
		}
		result.set(Token::T_SPACE, ws);
	}else if (CR==ch){
		// 改行文字
		if (!pp->eof()){
			pp->getch(ch);
			if (LF==ch){
				result.set(Token::T_EOL, L"\x0d\x0a");
			}else{
				pp->unget();
				result.set(Token::T_EOL, ch);
			}
		}else{
			result.set(Token::T_EOL, ch);
		}
	}else if (LF==ch){
		// 改行文字
		result.set(Token::T_EOL, ch);
	}else{
		// 特殊文字
		// 2文字の演算子チェック '==' '!=' '<=' '>=' '=~' '!~' '**' '||' '&&'
		if (m==ID_MODE){
			result.set(Token::Type(int(ch) & 0xFF), ch);
			if (ch==L'='){
				pp->getch(ch);
				if (ch==L'=') result.set(L"==");
				else if (ch==L'~') result.set(L"=~");
				else pp->unget();
			}else if(ch==L'!'){
				pp->getch(ch);
				if (ch==L'=') result.set(L"!=");
				else if (ch==L'~') result.set(L"!~");
				else pp->unget();
			}else if(ch==L'<'){
				pp->getch(ch);
				if (ch==L'=') result.set(L"<=");
				else pp->unget();
			}else if(ch==L'>'){
				pp->getch(ch);
				if (ch==L'=') result.set(L">=");
				else pp->unget();
			}else if(ch==L'*'){
				pp->getch(ch);
				if (ch==L'*') result.set(L"**");
				else pp->unget();
			}else if(ch==L'|'){
				pp->getch(ch);
				if (ch==L'|') result.set(L"||");
				else pp->unget();
			}else if(ch==L'&'){
				pp->getch(ch);
				if (ch==L'&') result.set(L"&&");
				else pp->unget();
			}
		}else{
			result.set(Token::Type(int(ch) & 0xFF), ch);
		}
	}
	// 正常終了
	//	lt = result;
	return(result);
}
//---------------------------------------------------------------------------
// ファイル名を返す
const wstring &TKawariLexer::getFileName(void) const {
	return fn;
}
//---------------------------------------------------------------------------
// 現在の行番号を返す
int TKawariLexer::getLineNo(void) const{
	// 多分ungetバッファに改行があるときは-1すべき。
	return pp->getLineNo();
}
//---------------------------------------------------------------------------
// 現在行の残りを全て返す
wstring TKawariLexer::getRestOfLine(void){
	return pp->getline();
}
//---------------------------------------------------------------------------
// サービスメソッド
// 空白文字と改行文字のトークンを読み飛ばす。
void TKawariLexer::skipWS(){
	static const wstring WS(L" \t\x0d\x0a");
	wchar_t ch;
	while (pp->peek(ch)){
		if (WS.find(ch)==wstring::npos)
			return;
		else
			pp->getch(ch);
	}
}
//---------------------------------------------------------------------------
// サービスメソッド
// 空白文字と改行文字のトークンを読み飛ばし、
// その次のトークンのタイプを返す
Token::Type TKawariLexer::skipWS(Mode m){
	static const wstring WS(L" \t\x0d\x0a");
	wchar_t ch;
	while (pp->peek(ch)){
		if (WS.find(ch)==wstring::npos)
			return checkType(m, ch);
		else
			pp->getch(ch);
	}
	return Token::T_EOF;
}
//---------------------------------------------------------------------------
// サービスメソッド
// 空白文字のトークンを読み飛ばし、その次のトークンのタイプを返す
// 改行文字は飛ばさない。
Token::Type TKawariLexer::skipS(Mode m){
	static const wstring SPACE(L" \t");
	wchar_t ch;
	while (pp->peek(ch)){
		if (SPACE.find(ch)==wstring::npos)
			return checkType(m, ch);
		else
			pp->getch(ch);
	}
	return Token::T_EOF;
}
//---------------------------------------------------------------------------
// サービスメソッド
// 文字列をエントリ名で使用可能な文字列にエンコードする
wstring TKawariLexer::EncodeEntryName(const wstring &orgsen){
	wstring entsen(orgsen);
	unsigned cntsen = static_cast<unsigned int>(entsen.size());

	for(unsigned int pos=0;pos<cntsen;pos++) {
		// 非ASCII文字(漢字など)は受容
		if(!IsLexChar((int)ID_MODE, entsen[pos])) {
			// 使えない文字は全て'_'に変換
			entsen[pos] = L'_';
		}
	}
	
	return(entsen);
}
//---------------------------------------------------------------------------
// 指定長さだけリードポインタを戻す
bool TKawariLexer::UngetChars(unsigned int length){
	for (unsigned int i=0; i<length; i++)
		if (!pp->unget())
			return false;
	return true;
}
//---------------------------------------------------------------------------
// ある文字の手前までスキップ (文法エラーからの復旧用)
// noret : 改行を許さない場合true
bool TKawariLexer::simpleSkipTo(wchar_t ch, bool noret){
	static const wstring CRLF(L"\x0d\x0a");
	while (hasNext()){
		wchar_t c;
		if (!pp->peek(c))
			break;
		if (c==ch)
			return true;
		else if (noret && (CRLF.find(c)!=wstring::npos))
			return true;
		pp->getch(c);
	}
	return false;
}

//--------------------------------------------------------------------------
// TKawariPreProcessor
//--------------------------------------------------------------------------
bool TKawariPreProcessor::processNextLine(void){
	if (is.eof()) return false;

	// 行末処理
	std::getline(is, linebuf, L'\x0a');
	if((linebuf.size()>0)&&(linebuf[linebuf.size()-1]==L'\x0d')) linebuf.erase(linebuf.size()-1);
	ln++;
	pos=0;
	if (pp){
		// プリプロセス

		// 復号処理
		// 暗号化行はASCII(Base64)なので、バイト列にして復号し、指定の文字コードでwide化する
		if (CheckCrypt(linebuf))
			linebuf=MBtoW(DecryptString(WtoA(linebuf)), cryptcs);

		if (mc && linebuf.find(L":endrem")==0){
			linebuf=L"";
			mc=false;
		}else if (mc){
			// コメント領域中
			linebuf=L"";
		}else if (linebuf[0]==L':'){
			// pragma
			if (linebuf.find(L":rem")==0){
				mc=true;
			}else{
				// unknown
			}
			linebuf=L"";
		}else if (linebuf[0]==L'='){
			// モード切替
			isMS=true;
		}else{
			// 1行コメントの処理
			unsigned int max=static_cast<unsigned int>(linebuf.size());
			for(unsigned int i=0; i<max; i++){
				if ((linebuf[i]==L' ')||(linebuf[i]==L'\t')){
					continue;
				}else if (linebuf[i]==L'#'){
					// comment
					if (lintnotes) lintnotes->ReadComment(lintfile, ln, linebuf.substr(i+1));
					linebuf=L"";
				}
				break;
			}
		}
	}
	linebuf=StringTrim(linebuf);
	linebuf+=L'\x0a';
	return true;
}
wstring TKawariPreProcessor::getline(void){
	wstring retstr=linebuf.substr(pos, linebuf.size()-pos);
	pos=static_cast<unsigned int>(linebuf.size());
	return retstr;
}
// バッファの一部を切り出す
wstring TKawariPreProcessor::substring(int index, int length){
	if ((index>=0)&&(length>0)&&((index+length)<(int)linebuf.size()))
		return linebuf.substr(index, length);
	else
		return (L"");
}
//---------------------------------------------------------------------------
