//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 字句解析
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.04.24  Phase 0.5     字句解析機分離
//  2001.05.20  Phase 5.1     インタープリタ・コンパイラ化
//	2002.01.11  Phase 7.3.1   I18N Hack (kpcg.I18Nhack)
//  2003.02.16  Phase 7.5.0
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_lexer.h"
//---------------------------------------------------------------------------
#include <cctype>
using namespace std;
//---------------------------------------------------------------------------
#include "misc/misc.h"
#include "misc/i18n.h"
//---------------------------------------------------------------------------
// 区切り文字の定義
#define CONST_START_MACRO		"$"			// マクロの開始文字
#define CONST_START_SEPARATOR	":;,](!&|)"		// その他単独の区切り文字
#define CONST_START_WHITESPACE	" \t"		// 空白
#define CONST_START_QUOTE		"\""		// クォート文字列
//---------------------------------------------------------------------------
const string StartMacro(CONST_START_MACRO);
const string StartSeparator(CONST_START_SEPARATOR);
const string StartWhiteSpace(CONST_START_WHITESPACE);
const string StartQuote(CONST_START_QUOTE);
const string StartAll(
 CONST_START_MACRO
 CONST_START_SEPARATOR
 CONST_START_WHITESPACE
 CONST_START_QUOTE);
//---------------------------------------------------------------------------
// 空白文字列を探す
// 解釈できた最後の文字の次の位置が帰る
#if defined(STRING_APPEND_INATOMIC)
unsigned int TKawariLexer::AcceptSpace(void) const
{
	const string &orgsen=(*this);
	unsigned int cntsen = orgsen.size();
	unsigned int endpos=Pos;

	while(endpos<cntsen) {
		if((orgsen[endpos]!=' ')&&(orgsen[endpos]!='\t')) break;
		endpos++;
	}

	return(endpos);
}
#else
unsigned int TKawariLexer::AcceptSpace(void) const
{
	const string &orgsen=(*this);

	unsigned int endpos=Pos;

	while(endpos<orgsen.size()) {
		if((orgsen[endpos]!=' ')&&(orgsen[endpos]!='\t')) break;
		endpos++;
	}

	return(endpos);
}
#endif
//---------------------------------------------------------------------------
// 'または"で囲まれた文字列を探す
// 解釈できた最後の文字の次の位置が帰る

#if defined(STRING_APPEND_INATOMIC)
unsigned int TKawariLexer::AcceptQuotedString(char quote) const
{
	const string &orgsen=(*this);
	unsigned int cntsen = orgsen.size();

	if((Pos>=cntsen)||(orgsen[Pos]!=quote)) return(Pos);

	unsigned int endpos=Pos+1;

	while(endpos<cntsen) {
		if(orgsen[endpos]==quote) {
			endpos++;
			break;
		}

		if(orgsen[endpos]=='\\') {
			if((endpos+1)<cntsen) endpos++;
		}

#ifdef I18N_HACK
		if(int trail = lang.moreBytes(orgsen[endpos])) {
			// jump over one character in MBCS environment
			if((endpos+trail)<cntsen) endpos+=trail;
		}
#else
		if(iskanji1st(orgsen[endpos])) {
			// 漢字
			if((endpos+1)<cntsen) endpos++;
		}
#endif
		endpos++;
	}

	return(endpos);
}
#else
unsigned int TKawariLexer::AcceptQuotedString(char quote) const
{
	const string &orgsen=(*this);

	if((Pos>=orgsen.size())||(orgsen[Pos]!=quote)) return(Pos);

	unsigned int endpos=Pos+1;

	while(endpos<orgsen.size()) {
		if(orgsen[endpos]==quote) {
			endpos++;
			break;
		}

		if(orgsen[endpos]=='\\') {
			if((endpos+1)<orgsen.size()) endpos++;
		}

#ifdef I18N_HACK
		if(int trail = lang.moreBytes(orgsen[endpos])) {
			// jump over one character in MBCS environment
			if((endpos+trail)<orgsen.size()) endpos+=trail;
		}
#else
		if(iskanji1st(orgsen[endpos])) {
			// 漢字
			if((endpos+1)<orgsen.size()) endpos++;
		}
#endif
		endpos++;
	}

	return(endpos);
}
#endif
//---------------------------------------------------------------------------
// エントリ名を探す
// 解釈できた最後の文字の次の位置が帰る
#if defined(STRING_APPEND_INATOMIC)
unsigned int TKawariLexer::AcceptEntryName(void) const
{
	const string &orgsen=(*this);
	unsigned int cntsen = orgsen.size();

	unsigned int endpos=Pos;

	while(endpos<cntsen) {
#ifdef I18N_HACK
		if(int trail=lang.moreBytes(orgsen[endpos])) {
			if((endpos+trail)<cntsen) {
				endpos += (trail+1);
			} else {
				break;
			}
#else
		if(iskanji1st(orgsen[endpos])) {
			// 漢字
			if((endpos+1)<cntsen) {
				endpos += 2;
			} else {
				break;
			}
#endif
		} else if(
		 isalnum(orgsen[endpos])
		 ||(orgsen[endpos]=='_')
		 ||(orgsen[endpos]=='.')
		 ||((orgsen[endpos]=='-')&&(endpos-Pos>0)) ) {
			// 英数字,'_'と'-'
			// '-'は二文字目以降でしか使えない
			endpos++;
		} else if(orgsen[endpos]=='@') {
			// ローカル変数をあらわす'@'
			// 内部的には、二文字目以降で使用することもある
			endpos++;
		} else {
			break;
		}
	}

	return(endpos);
}
#else
unsigned int TKawariLexer::AcceptEntryName(void) const
{
	const string &orgsen=(*this);

	unsigned int endpos=Pos;

	while(endpos<orgsen.size()) {
#ifdef I18N_HACK
		if(int trail=lang.moreBytes(orgsen[endpos])) {
			if((endpos+trail)<orgsen.size()) {
				endpos += (trail+1);
			} else {
				break;
			}
#else
		if(iskanji1st(orgsen[endpos])) {
			// 漢字
			if((endpos+1)<orgsen.size()) {
				endpos++;
				endpos++;
			} else {
				break;
			}
#endif
		} else if(
		 isalnum(orgsen[endpos])
		 ||(orgsen[endpos]=='_')
		 ||(orgsen[endpos]=='.')
		 ||((orgsen[endpos]=='-')&&(endpos-Pos>0)) ) {
			// 英数字,'_'と'-'
			// '-'は二文字目以降でしか使えない
			endpos++;
		} else if(orgsen[endpos]=='@') {
			// ローカル変数をあらわす'@'
			// 内部的には、二文字目以降で使用することもある
			endpos++;
		} else {
			break;
		}
	}

	return(endpos);
}
#endif

//---------------------------------------------------------------------------
// 文字列の指定位置posからエントリ名指定子列を一つ切り出す
// posは解釈した分だけ進められる

#if defined(STRING_APPEND_INATOMIC)
void TKawariLexer::GetEntryList(vector<string>& entrytable,char separator)
{
	const string &orgsen=(*this);
	unsigned int cntsen = orgsen.size();

	while(Pos<cntsen) {
		Pos=AcceptSpace();
		unsigned int startpos=Pos;
		Pos=AcceptEntryName();
		if(startpos!=Pos) {
			entrytable.push_back(orgsen.substr(startpos,Pos-startpos));
		} else if(orgsen[Pos]==separator) {
			// マルチエントリの区切り文字
			Pos++;
		} else {
			break;
		}
	}

	Pos=AcceptSpace();	// サービス

	return;
}
#else
void TKawariLexer::GetEntryList(vector<string>& entrytable,char separator)
{
	const string &orgsen=(*this);

	while(Pos<orgsen.size()) {
		Pos=AcceptSpace();
		unsigned int startpos=Pos;
		Pos=AcceptEntryName();
		if(startpos!=Pos) {
			entrytable.push_back(orgsen.substr(startpos,Pos-startpos));
		} else if(orgsen[Pos]==separator) {
			// マルチエントリの区切り文字
			Pos++;
		} else {
			break;
		}
	}

	Pos=AcceptSpace();	// サービス

	return;
}
#endif
//---------------------------------------------------------------------------
// 文字列の指定位置posからトークンを一つ切り出す
// posは解釈した分だけ進められる
#if defined(STRING_APPEND_INATOMIC)
TKawariLexer::TToken TKawariLexer::GetToken(void)
{
	const string &orgsen=(*this);
	unsigned int cntsen = orgsen.size();

	TToken result;
	result.Type=TToken::ttokError;
	result.Str="";

	if(Pos>=cntsen) return(result);

	unsigned int startpos=Pos;

	try {
		if(orgsen[Pos]==CONST_START_MACRO[0]) {
			// マクロ
			if((++Pos)>=orgsen.size()) throw("error");

			if(orgsen[Pos]=='{') {
				// ${エントリ名}
				result.Type=TToken::ttokMacroEntry;

				if((++Pos)>=orgsen.size()) throw("error");

				result.Str+="${";
			} else if(orgsen[Pos]=='(') {
				// $(インラインスクリプト)
				result.Type=TToken::ttokMacroInline;

				if((++Pos)>=orgsen.size()) throw("error");

				result.Str+="$(";
			} else {
				// 未知のマクロ
				Pos++;
				throw("error");
			}
		} else if(StartSeparator.find(orgsen[Pos])!=string::npos) {
			// 区切り文字
			result.Type=TToken::ttokSeparator;

			result.Str+=orgsen[Pos++];
		} else if(StartWhiteSpace.find(orgsen[Pos])!=string::npos) {
			// 空白文字列
			result.Type=TToken::ttokSpace;

			unsigned int startpos=Pos;
			Pos=AcceptSpace();
			result.Str+=orgsen.substr(startpos,Pos-startpos);
		} else if(StartQuote.find(orgsen[Pos])!=string::npos) {
			// クォート文字列
			result.Type=TToken::ttokSentence;

			unsigned int startpos=Pos;
			Pos=AcceptQuotedString();
			result.Str+=DecodeQuotedString(orgsen.substr(startpos,Pos-startpos));
		} else {
			// 文字列
			result.Type=TToken::ttokSentence;
			
			unsigned int tmppos = Pos;
			while((Pos<cntsen)
			 &&(StartAll.find(orgsen[Pos])==string::npos)) {
#ifdef I18N_HACK
				if(int trail=lang.moreBytes(orgsen[Pos])) {
					if((Pos+trail)<cntsen) {
							Pos+=trail+1;
					} else Pos++;
				} else {
					Pos++;
				}
#else
				if(iskanji1st(orgsen[Pos])) {
					if((Pos+1)<cntsen) {
						Pos += 2;
					} else Pos++;
				} else {
					Pos++;
				}
#endif
			}
			if (Pos > tmppos)
				result.Str += orgsen.substr(tmppos, Pos - tmppos);
		}
	} catch(...) {
		// エラー処理
		result.Type=TToken::ttokError;
		result.Str=orgsen.substr(startpos,Pos-startpos);
		return(result);
	}

	// 正常終了
	return(result);
}
#else
TKawariLexer::TToken TKawariLexer::GetToken(void)
{
	const string &orgsen=(*this);

	TToken result;
	result.Type=TToken::ttokError;
	result.Str="";

	if(Pos>=orgsen.size()) return(result);

	unsigned int startpos=Pos;

	try {
		if(orgsen[Pos]==CONST_START_MACRO[0]) {
			// マクロ
			if((++Pos)>=orgsen.size()) throw("error");

			if(orgsen[Pos]=='{') {
				// ${エントリ名}
				result.Type=TToken::ttokMacroEntry;

				if((++Pos)>=orgsen.size()) throw("error");

				result.Str+="${";
			} else if(orgsen[Pos]=='(') {
				// $(インラインスクリプト)
				result.Type=TToken::ttokMacroInline;

				if((++Pos)>=orgsen.size()) throw("error");

				result.Str+="$(";
			} else {
				// 未知のマクロ
				Pos++;
				throw("error");
			}
		} else if(StartSeparator.find(orgsen[Pos])!=string::npos) {
			// 区切り文字
			result.Type=TToken::ttokSeparator;

			result.Str+=orgsen[Pos++];
		} else if(StartWhiteSpace.find(orgsen[Pos])!=string::npos) {
			// 空白文字列
			result.Type=TToken::ttokSpace;

			unsigned int startpos=Pos;
			Pos=AcceptSpace();
			result.Str+=orgsen.substr(startpos,Pos-startpos);
		} else if(StartQuote.find(orgsen[Pos])!=string::npos) {
			// クォート文字列
			result.Type=TToken::ttokSentence;

			unsigned int startpos=Pos;
			Pos=AcceptQuotedString();
			result.Str+=DecodeQuotedString(orgsen.substr(startpos,Pos-startpos));
		} else {
			// 文字列
			result.Type=TToken::ttokSentence;
			while((Pos<orgsen.size())
			 &&(StartAll.find(orgsen[Pos])==string::npos)) {
#ifdef I18N_HACK
				if(int trail=lang.moreBytes(orgsen[Pos])) {
					if((Pos+trail)<orgsen.size()) {
						for(int i=0; i <= trail; i++)
							result.Str+=orgsen[Pos++];
					} else Pos++;
				} else {
					result.Str+=orgsen[Pos++];
				}
#else
				if(iskanji1st(orgsen[Pos])) {
					if((Pos+1)<orgsen.size()) {
						result.Str+=orgsen[Pos++];
						result.Str+=orgsen[Pos++];
					} else Pos++;
				} else {
					result.Str+=orgsen[Pos++];
				}
#endif
			}
		}
	} catch(...) {
		// エラー処理
		result.Type=TToken::ttokError;
		result.Str=orgsen.substr(startpos,Pos-startpos);
		return(result);
	}

	// 正常終了
	return(result);
}
#endif
//---------------------------------------------------------------------------
// 文字列の指定位置posからマクロを含まない単語文字列を一つ切り出す
// posは解釈した分だけ進められる
string TKawariLexer::GetWordString(const char *separator_)
{
	string separator=separator_?string(separator_):string(",");

	Pos=AcceptSpace();

	string word;
	string ws;

	while(!IsEnd()) {
		unsigned int oldpos=Pos;
		TKawariLexer::TToken token=GetToken();

		// 単語の区切り記号だった
		if((token.Type==TKawariLexer::TToken::ttokSeparator)
		 &&(separator.find(token.Str)!=string::npos)) {
			// バックトラック
			Pos=oldpos;
			break;
		}

		// エントリ呼び出し
		// $(インラインスクリプト)
		// 本来エラー処理
		if((token.Type==TKawariLexer::TToken::ttokMacroEntry)
		 ||(token.Type==TKawariLexer::TToken::ttokMacroInline)) {
			// バックトラック
			Pos=oldpos;
			break;
		}

		// それ以外
		switch(token.Type) {
		case TKawariLexer::TToken::ttokSpace :
			// 空白文字列
			ws=token.Str;
			break;
		default:
			// 文字列・・・かもしれない
			if(ws.size()) {
				word+=ws;
				ws="";
			}
			word+=token.Str;
		}
	}

	return(word);
}
//---------------------------------------------------------------------------
// 'または"で囲まれた文字列からクォート文字をはずす
string TKawariLexer::DecodeQuotedString(const string& orgsen)
{
	if(orgsen.size()==0) return("");

	char quote=orgsen[0];

	unsigned int pos=1;
	string retstr;

	while(pos<orgsen.size()) {
		if(orgsen[pos]==quote) break;

		if(orgsen[pos]=='\\') {
			if(((pos+1)<orgsen.size())&&(orgsen[pos+1]==quote)) pos++;
		}

#ifdef I18N_HACK
		if(int trail = lang.moreBytes(orgsen[pos])) {
			// MBCS
			if((pos+trail)<orgsen.size()) {
				for(int i=0; i < trail; i++)
					retstr+=orgsen[pos++];
			}
		}
#else
		if(iskanji1st(orgsen[pos])) {
			// 漢字
			if((pos+1)<orgsen.size()) retstr+=orgsen[pos++];
		}
#endif
		retstr+=orgsen[pos++];
	}

	return(retstr);
}
//---------------------------------------------------------------------------

