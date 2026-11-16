//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨7互換モード : 7.5.0 の字句解析・コンパイラ・実行
//
//  2026.11.16  Phase 9.3.0   華和梨 7.5.0 の kawari_lexer.cpp / kawari_code.cpp /
//                            kawari_iscript.cpp を、9 の中間コードに合わせて移植
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <cstdlib>
#include <set>
#include <vector>
#include <string>
//---------------------------------------------------------------------------
#include "libkawari/kawari7.h"
#include "libkawari/kawari_compiler.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// 区切り文字 (7.5.0 の CONST_START_SEPARATOR)
static const wchar_t *const SEPARATORS=L":;,](!&|)";
//---------------------------------------------------------------------------
static inline bool IsSeparatorChar(wchar_t c)
{
	for(const wchar_t *p=SEPARATORS; *p; p++)
		if(*p==c) return true;
	return false;
}
//---------------------------------------------------------------------------
static inline bool IsSpaceChar(wchar_t c)
{
	return (c==L' ')||(c==L'\t');
}
//---------------------------------------------------------------------------
// 文字列の最初の位置からの、行頭のゆるい整数 (atoi 相当)
static unsigned int ParseUInt(const wstring &s)
{
	return static_cast<unsigned int>(wcstoul(s.c_str(), NULL, 10));
}
//---------------------------------------------------------------------------
// ASCII の英数字
static inline bool IsAsciiAlnum(wchar_t c)
{
	return ((c>=L'0')&&(c<=L'9'))||((c>=L'a')&&(c<=L'z'))||((c>=L'A')&&(c<=L'Z'));
}
//---------------------------------------------------------------------------
// 7 のエントリ名に使える ASCII 以外の文字
// (Shift_JIS の 2 バイト文字は使える。半角カナ (1 バイト) は使えない)
static inline bool IsWideNameChar(wchar_t c)
{
	if(static_cast<unsigned int>(c)<0x80) return false;
	if((c>=0xFF61)&&(c<=0xFF9F)) return false;
	return true;
}
//===========================================================================
// 字句解析器
//===========================================================================
// 空白を読み飛ばした位置
size_t TKawari7Lexer::AcceptSpace(void) const
{
	size_t endpos=pos;
	while(endpos<src.size()) {
		if(!IsSpaceChar(src[endpos])) break;
		endpos++;
	}
	return endpos;
}
//---------------------------------------------------------------------------
// 引用符で囲まれた文字列を読んだ次の位置
size_t TKawari7Lexer::AcceptQuotedString(wchar_t quote) const
{
	if((pos>=src.size())||(src[pos]!=quote)) return pos;

	size_t endpos=pos+1;
	while(endpos<src.size()) {
		if(src[endpos]==quote) {
			endpos++;
			break;
		}
		if(src[endpos]==L'\\') {
			if((endpos+1)<src.size()) endpos++;
		}
		endpos++;
	}
	return endpos;
}
//---------------------------------------------------------------------------
// エントリ名を読んだ次の位置
size_t TKawari7Lexer::AcceptEntryName(void) const
{
	size_t endpos=pos;
	while(endpos<src.size()) {
		wchar_t c=src[endpos];
		if(IsWideNameChar(c)) {
			endpos++;
		} else if(IsAsciiAlnum(c)||(c==L'_')||(c==L'.')||((c==L'-')&&(endpos-pos>0))) {
			endpos++;
		} else if(c==L'@') {
			endpos++;
		} else {
			break;
		}
	}
	return endpos;
}
//---------------------------------------------------------------------------
// エントリ名を separator 区切りで読む
void TKawari7Lexer::GetEntryList(vector<wstring> &list, wchar_t separator)
{
	while(pos<src.size()) {
		pos=AcceptSpace();
		size_t startpos=pos;
		pos=AcceptEntryName();
		if(startpos!=pos) {
			list.push_back(src.substr(startpos, pos-startpos));
		} else if(At(pos)==separator) {
			pos++;
		} else {
			break;
		}
	}
	pos=AcceptSpace();
}
//---------------------------------------------------------------------------
// トークンを 1 つ読む
TKawari7Lexer::Token TKawari7Lexer::GetToken(void)
{
	Token result;
	result.type=Token::T_ERROR;

	if(pos>=src.size()) return result;

	size_t startpos=pos;
	wchar_t c=src[pos];

	if(c==L'$') {
		// マクロ
		pos++;
		bool ok=false;
		if(pos<src.size()) {
			if(src[pos]==L'{') {
				pos++;
				if(pos<src.size()) {
					result.type=Token::T_MACRO_ENTRY;
					result.str=L"${";
					ok=true;
				}
			} else if(src[pos]==L'(') {
				pos++;
				if(pos<src.size()) {
					result.type=Token::T_MACRO_INLINE;
					result.str=L"$(";
					ok=true;
				}
			} else {
				// 知らないマクロ
				pos++;
			}
		}
		if(!ok) {
			result.type=Token::T_ERROR;
			result.str=src.substr(startpos, pos-startpos);
		}
		return result;
	}

	if(IsSeparatorChar(c)) {
		result.type=Token::T_SEPARATOR;
		result.str=wstring(1, c);
		pos++;
	} else if(IsSpaceChar(c)) {
		result.type=Token::T_SPACE;
		size_t st=pos;
		pos=AcceptSpace();
		result.str=src.substr(st, pos-st);
	} else if(c==L'"') {
		// 引用符つき文字列
		result.type=Token::T_SENTENCE;
		size_t st=pos;
		pos=AcceptQuotedString(L'"');
		result.str=DecodeQuotedString(src.substr(st, pos-st));
	} else {
		// 文字列
		result.type=Token::T_SENTENCE;
		size_t st=pos;
		while(pos<src.size()) {
			wchar_t d=src[pos];
			if((d==L'$')||IsSeparatorChar(d)||IsSpaceChar(d)||(d==L'"')) break;
			pos++;
		}
		result.str=src.substr(st, pos-st);
	}
	return result;
}
//---------------------------------------------------------------------------
// マクロを含まない単語を、separator のどれかの区切りが来るまで読む
wstring TKawari7Lexer::GetWordString(const wchar_t *separator)
{
	wstring seps=separator?wstring(separator):wstring(L",");

	pos=AcceptSpace();

	wstring word;
	wstring ws;

	while(!IsEnd()) {
		size_t oldpos=pos;
		Token token=GetToken();

		if((token.type==Token::T_SEPARATOR)&&(seps.find(token.str)!=wstring::npos)) {
			pos=oldpos;
			break;
		}
		if((token.type==Token::T_MACRO_ENTRY)||(token.type==Token::T_MACRO_INLINE)) {
			pos=oldpos;
			break;
		}
		if(token.type==Token::T_SPACE) {
			ws=token.str;
		} else {
			if(ws.size()) {
				word+=ws;
				ws=L"";
			}
			word+=token.str;
		}
	}
	return word;
}
//---------------------------------------------------------------------------
// 引用符つき文字列から引用符を外す
wstring TKawari7Lexer::DecodeQuotedString(const wstring &s)
{
	if(s.size()==0) return L"";

	wchar_t quote=s[0];
	size_t p=1;
	wstring retstr;

	while(p<s.size()) {
		if(s[p]==quote) break;
		if(s[p]==L'\\') {
			if(((p+1)<s.size())&&(s[p+1]==quote)) p++;
		}
		retstr+=s[p++];
	}
	return retstr;
}
//===========================================================================
// ノード
//===========================================================================
// 7 の文字列の逆コンパイル
wstring TK7String::DisCompile(void) const
{
	wstring retstr=L"\"";
	for(size_t i=0; i<s.size(); i++) {
		if(s[i]!=L'"') retstr+=s[i];
		else retstr+=L"\\\"";
	}
	retstr+=L"\"";
	return retstr;
}
//---------------------------------------------------------------------------
// 7 の ${エントリ&エントリ} が見る単語の集まり
// 「${別のエントリ} だけの単語」は、その別のエントリの単語に展開して数える
static void CollectWords7(TKawariVM &vm, const TEntry &start, set<TWordID> &wordcol)
{
	TNS_KawariDictionary &dict=vm.Dictionary();
	set<TEntry> donelist;
	vector<TEntry> parselist;
	parselist.push_back(start);

	while(parselist.size()) {
		TEntry entry=parselist.back();
		parselist.pop_back();
		if(donelist.count(entry)) continue;
		donelist.insert(entry);

		vector<TWordID> tmpcol;
		entry.FindAll(tmpcol);
		for(size_t i=0; i<tmpcol.size(); i++) {
			TWordID id=tmpcol[i];
			TKVMCode_base *code=dict.GetWordFromID(id);
			TK7EntryCall *call=code?dynamic_cast<TK7EntryCall *>(code):NULL;
			if(call&&(call->Names().size()==1)) {
				TEntry child=dict.GetEntry(call->Names()[0]);
				if(child.IsValid()&&(donelist.count(child)==0))
					parselist.push_back(child);
			} else {
				wordcol.insert(id);
			}
		}
	}
}
//---------------------------------------------------------------------------
// ${エントリ} / ${エントリ&エントリ}
wstring TK7EntryCall::Run(TKawariVM &vm)
{
	TNS_KawariDictionary &dict=vm.Dictionary();
	TWordID id=0;
	TEntry from(NULL, 0);

	if(names.size()==1) {
		TEntry entry=dict.GetEntry(names[0]);
		if(!entry.IsValid()) return L"";
		vector<TWordID> wordlist;
		entry.FindAll(wordlist);
		if(wordlist.size()==0) return L"";
		id=wordlist[Random(static_cast<int>(wordlist.size()))];
		from=entry;
	} else {
		// 存在するエントリだけの積集合
		set<TWordID> result;
		bool first=true;
		for(size_t i=0; i<names.size(); i++) {
			TEntry entry=dict.GetEntry(names[i]);
			if(!entry.IsValid()) continue;
			set<TWordID> col;
			CollectWords7(vm, entry, col);
			if(first) {
				result=col;
				first=false;
			} else {
				set<TWordID> inter;
				for(set<TWordID>::const_iterator it=result.begin(); it!=result.end(); ++it)
					if(col.count(*it)) inter.insert(*it);
				result=inter;
			}
		}
		if(result.size()==0) return L"";
		size_t index=static_cast<size_t>(Random(static_cast<int>(result.size())));
		set<TWordID>::const_iterator it=result.begin();
		for(size_t i=0; i<index; i++) ++it;
		id=*it;
	}

	TKVMCode_base *code=dict.GetWordFromID(id);
	if(!code) return L"";

	wstring retstr;
	{
		TKawariVM::TWordFrame frame(vm, from, id);
		retstr=vm.RunWithNewContext(code);
	}
	// 7 は、エントリ呼び出しの結果だけを履歴に積む
	dict.PushToHistory(retstr);
	return retstr;
}
//---------------------------------------------------------------------------
wstring TK7EntryCall::DisCompile(void) const
{
	wstring retstr=L"${";
	for(size_t i=0; i<names.size(); i++) {
		if(i) retstr+=L"&";
		retstr+=names[i];
	}
	retstr+=L"}";
	return retstr;
}
//---------------------------------------------------------------------------
bool TK7EntryCall::Less(const TKVMCode_base &r_) const
{
	const TK7EntryCall &r=static_cast<const TK7EntryCall &>(r_);
	if(names.size()!=r.names.size()) return names.size()<r.names.size();
	for(size_t i=0; i<names.size(); i++) {
		if(names[i]<r.names[i]) return true;
		if(names[i]>r.names[i]) return false;
	}
	return false;
}
//---------------------------------------------------------------------------
wostream &TK7EntryCall::Debug(wostream &os, unsigned int level) const
{
	return DebugIndent(os, level) << L"K7EntryCall(" << DisCompile() << L")" << endl;
}
//---------------------------------------------------------------------------
// ${0}
wstring TK7HistoryCall::Run(TKawariVM &vm)
{
	return vm.Dictionary().GetHistory(static_cast<int>(no));
}
//---------------------------------------------------------------------------
wstring TK7HistoryCall::DisCompile(void) const
{
	return L"${"+IntToString(static_cast<int>(no))+L"}";
}
//---------------------------------------------------------------------------
bool TK7HistoryCall::Less(const TKVMCode_base &r_) const
{
	const TK7HistoryCall &r=static_cast<const TK7HistoryCall &>(r_);
	return no<r.no;
}
//---------------------------------------------------------------------------
wostream &TK7HistoryCall::Debug(wostream &os, unsigned int level) const
{
	return DebugIndent(os, level) << L"K7HistoryCall(" << DisCompile() << L")" << endl;
}
//---------------------------------------------------------------------------
// 文
wstring TK7Statement::Run(TKawariVM &vm)
{
	vector<wstring> args;
	for(size_t i=0; i<list.size(); i++)
		args.push_back(list[i]->Run(vm));
	return vm.FunctionCall7(args);
}
//---------------------------------------------------------------------------
wstring TK7Statement::DisCompile(void) const
{
	wstring retstr;
	for(size_t i=0; i<list.size(); i++) {
		if(i) retstr+=L" ";
		retstr+=list[i]->DisCompile();
	}
	return retstr;
}
//---------------------------------------------------------------------------
wstring TK7Statement::GetArg0(void) const
{
	if(list.size()==0) return L"";
	const TKVMCodeString *str=dynamic_cast<const TKVMCodeString *>(list[0]);
	if(!str) return L"";
	return str->s;
}
//---------------------------------------------------------------------------
// 7 の真偽 ("" "0" "false" が偽)
static inline bool IsTrue7(const wstring &s)
{
	return !((s.size()==0)||(s==L"0")||(s==L"false"));
}
//---------------------------------------------------------------------------
// インラインスクリプト (kawari_iscript.cpp の TKisEngine::Run)
wstring TK7InlineScript::Run(TKawariVM &vm)
{
	TNS_KawariDictionary &dict=vm.Dictionary();
	unsigned int frame=dict.LinkFrame();

	wstring retstr;

	for(size_t pc=0; pc<list.size(); pc++) {
		TK7Statement *st=static_cast<TK7Statement *>(list[pc]);
		const vector<TKVMCode_base *> &code=st->list;
		wstring arg0=st->GetArg0();
		wstring result;

		if(arg0==L"NULL") {
			result=L"";
		} else if(arg0==L"if") {
			if((code.size()==3)||(code.size()==4)) {
				if(IsTrue7(code[1]->Run(vm))) {
					result=code[2]->Run(vm);
				} else {
					if(code.size()==4) result=code[3]->Run(vm);
				}
			}
		} else if(arg0==L"while") {
			if(code.size()==3) {
				while(IsTrue7(code[1]->Run(vm))) {
					dict.CreateContext();
					result+=code[2]->Run(vm);
					dict.DeleteContext();
				}
			}
		} else if(arg0==L"until") {
			if(code.size()==3) {
				while(!IsTrue7(code[1]->Run(vm))) {
					dict.CreateContext();
					result+=code[2]->Run(vm);
					dict.DeleteContext();
				}
			}
		} else if(arg0==L"loop") {
			if(code.size()==3) {
				int limit=StringToInt(code[1]->Run(vm));
				for(int i=0; (i<limit)&&(limit>0); i++) {
					dict.CreateContext();
					result+=code[2]->Run(vm);
					dict.DeleteContext();
				}
			}
		} else if(arg0==L"foreach") {
			if(code.size()==4) {
				wstring tmpentryname=code[1]->Run(vm);
				wstring listrefname=code[2]->Run(vm);
				if(listrefname.size()) {
					vector<TWordID> wordcol;
					TEntry listentry=dict.GetEntry(listrefname);
					if(listentry.IsValid()) listentry.FindAll(wordcol);
					for(size_t i=0; i<wordcol.size(); i++) {
						TKVMCode_base *item=dict.GetWordFromID(wordcol[i]);
						if(!item) continue;
						vm.GetEngine().PushAfterClear(tmpentryname, vm.RunWithNewContext(item));

						dict.CreateContext();
						result+=code[3]->Run(vm);
						dict.DeleteContext();
					}
				}
			}
		} else if(arg0==L"?") {
			if(code.size()>1) {
				size_t index=static_cast<size_t>(Random(static_cast<int>(code.size()-1)))+1;
				result+=code[index]->Run(vm);
			}
		} else if(arg0==L"silent") {
			retstr=L"";
		} else {
			result=st->Run(vm);
		}

		retstr+=result;
	}

	// $( ) の中で積まれた履歴は、外に残さない
	dict.UnlinkFrame(frame);
	return retstr;
}
//---------------------------------------------------------------------------
wstring TK7InlineScript::DisCompile(void) const
{
	wstring retstr=L"$(";
	for(size_t i=0; i<list.size(); i++) {
		if(i) retstr+=L";";
		retstr+=list[i]->DisCompile();
	}
	retstr+=L")";
	return retstr;
}
//===========================================================================
// コンパイラ
//===========================================================================
// 引用符 ('...' か "...") の次の位置。閉じていなければ文字列の長さ
static size_t SkipQuoted(const wstring &s, size_t i)
{
	wchar_t q=s[i];
	i++;
	while(i<s.size()) {
		if(s[i]==L'\\') i++;
		else if(s[i]==q) return i+1;
		i++;
	}
	return s.size();
}
//---------------------------------------------------------------------------
// $[ ... ] の中身の始まり (i) から、対応する ] の位置を探す。無ければ npos
// 引用符の中の ] と、${ } / $( ) の中の ] は数えない
static size_t FindExpressionEnd(const wstring &s, size_t i)
{
	int depth=1;
	while(i<s.size()) {
		wchar_t c=s[i];
		if((c==L'"')||(c==L'\'')) {
			i=SkipQuoted(s, i);
			continue;
		}
		if((c==L'$')&&((i+1)<s.size())&&((s[i+1]==L'(')||(s[i+1]==L'{'))) {
			// ${ ... } / $( ... ) を対応する閉じまで飛ばす
			wchar_t open=s[i+1];
			wchar_t close=(open==L'(')?L')':L'}';
			int d=1;
			i+=2;
			while((i<s.size())&&(d>0)) {
				if((s[i]==L'"')||(s[i]==L'\'')) {
					i=SkipQuoted(s, i);
					continue;
				}
				if(s[i]==open) d++;
				else if(s[i]==close) d--;
				i++;
			}
			continue;
		}
		if(c==L'[') depth++;
		else if(c==L']') {
			depth--;
			if(depth==0) return i;
		}
		i++;
	}
	return wstring::npos;
}
//---------------------------------------------------------------------------
// 文字列 (TKawariCodeString::Compile)
// 文字列だけの連続を 1 つに結合する。区切り文字のうち ] だけは文字列の一部
bool TKawari7Compiler::compileText(TKawari7Lexer &lex, wstring &out)
{
	wstring word;
	bool flag=false;
	while((!lex.IsEnd())&&(!flag)) {
		size_t pos=lex.pos;
		TKawari7Lexer::Token token=lex.GetToken();

		switch(token.type) {
		case TKawari7Lexer::Token::T_SENTENCE:
			word+=token.str;
			break;
		case TKawari7Lexer::Token::T_SEPARATOR:
			if(token.str[0]==L']') word+=token.str;
			else flag=true;
			break;
		default:
			flag=true;
		}

		if(flag) lex.pos=pos;
	}

	if(word.size()==0) return false;
	out=word;
	return true;
}
//---------------------------------------------------------------------------
// 単語の部品 1 つ以上 (TKawariCodeWord::Compile)
// 文字列・${...}・$(...) のどれか。空白が来たら終わる
bool TKawari7Compiler::compileWordParts(TKawari7Lexer &lex, vector<TKVMCode_base *> &codelist)
{
	size_t oldsize=codelist.size();

	while(!lex.IsEnd()) {
		// 9 の式 $[ ... ] (7 には無い。取り込む指定のときだけ。中身は 9 の文法で最後まで読む)
		if(lex.logger&&(lex.At(lex.pos)==L'$')&&(lex.At(lex.pos+1)==L'[')) {
			size_t end=FindExpressionEnd(lex.src, lex.pos+2);
			if(end!=wstring::npos) {
				TKVMCode_base *code=TKawariCompiler::Compile(lex.src.substr(lex.pos, end+1-lex.pos), *lex.logger);
				if(code) {
					codelist.push_back(code);
					lex.pos=end+1;
					continue;
				}
			}
		}

		size_t pos=lex.pos;
		TKawari7Lexer::Token token=lex.GetToken();

		bool flag=false;
		switch(token.type) {
		case TKawari7Lexer::Token::T_SPACE:
			lex.pos=pos;
			flag=true;
			break;
		case TKawari7Lexer::Token::T_MACRO_ENTRY:
			{
				vector<wstring> key;
				lex.GetEntryList(key);

				if((!lex.IsEnd())&&(lex.src[lex.pos]==L'}')) {
					lex.pos++;
					if(key.size()==1) {
						wchar_t c0=key[0][0];
						if((c0>=L'0')&&(c0<=L'9')) {
							// 履歴参照
							codelist.push_back(new TK7HistoryCall(ParseUInt(key[0])));
						} else {
							codelist.push_back(new TK7EntryCall(key));
						}
					} else if(key.size()>1) {
						codelist.push_back(new TK7EntryCall(key));
					}
				} else {
					// ${エントリ名} として解釈できなかったので文字列にする
					codelist.push_back(new TK7String(lex.src.substr(pos, lex.pos-pos)));
				}
			}
			break;
		case TKawari7Lexer::Token::T_MACRO_INLINE:
			lex.pos=pos;
			{
				vector<TKVMCode_base *> stmts;
				if(compileInlineScript(lex, stmts)) {
					codelist.push_back(new TK7InlineScript(stmts));
				} else {
					// $(インラインスクリプト) として解釈できなかったので文字列にする
					codelist.push_back(new TK7String(lex.src.substr(pos, lex.pos-pos)));
				}
			}
			break;
		default:
			lex.pos=pos;
			{
				wstring text;
				if(compileText(lex, text)) {
					codelist.push_back(new TK7String(text));
				} else {
					// 文字列ではなかったのでバックトラック
					lex.pos=pos;
					flag=true;
				}
			}
		}

		if(flag) break;
	}

	return codelist.size()!=oldsize;
}
//---------------------------------------------------------------------------
// 単語 (TKawariCodeWord::Compile2)
// 指定の区切り以外は全て文字列とする。区切りが指定されたときは、末尾の空白は無視する。
bool TKawari7Compiler::CompileWord(TKawari7Lexer &lex, vector<TKVMCode_base *> &codelist, wchar_t separator)
{
	size_t oldsize=codelist.size();
	wstring ws;

	while(!lex.IsEnd()) {
		size_t pos=lex.pos;
		TKawari7Lexer::Token token=lex.GetToken();

		// 単語の区切りの , が来たら終わる (区切りは読み終えた状態にする)
		if((token.type==TKawari7Lexer::Token::T_SEPARATOR)&&(separator!=0)&&(token.str[0]==separator))
			break;

		if(token.type==TKawari7Lexer::Token::T_SPACE) {
			ws=token.str;
		} else {
			lex.pos=pos;
			vector<TKVMCode_base *> codelist2;

			if(compileWordParts(lex, codelist2)) {
				if(ws.size()) {
					codelist.push_back(new TK7String(ws));
					ws=L"";
				}
				for(size_t i=0; i<codelist2.size(); i++)
					codelist.push_back(codelist2[i]);
			} else {
				token=lex.GetToken();
				if(token.str.size()) {
					codelist.push_back(new TK7String(ws+token.str));
					ws=L"";
				}
			}
		}
	}

	if(ws.size()&&(separator==0))
		codelist.push_back(new TK7String(ws));

	return codelist.size()!=oldsize;
}
//---------------------------------------------------------------------------
// 部品を 1 つの単語にまとめる (1 つならそのまま、2 つ以上なら連結)
static void FlushWord(vector<TKVMCode_base *> &parts, vector<TKVMCode_base *> &out)
{
	if(parts.size()==1) {
		out.push_back(parts[0]);
	} else if(parts.size()>1) {
		out.push_back(new TKVMCodeList(parts));
	}
	parts.clear();
}
//---------------------------------------------------------------------------
// インラインスクリプトの 1 文 (TKawariCodeInlineScriptStatement::Compile)
// 戻り値 : 次の文が続くとき (; で終わったとき) true
bool TKawari7Compiler::compileStatement(TKawari7Lexer &lex, vector<TKVMCode_base *> &codelist)
{
	size_t oldsize=codelist.size();
	bool statusflag=false;

	vector<TKVMCode_base *> codelist2;
	while(!lex.IsEnd()) {
		size_t pos=lex.pos;
		TKawari7Lexer::Token token=lex.GetToken();

		if(token.type==TKawari7Lexer::Token::T_SEPARATOR) {
			if(token.str==L")") {
				// インラインスクリプトの終わり
				break;
			} else if(token.str==L";") {
				statusflag=true;
				break;
			}
		}

		if(token.type==TKawari7Lexer::Token::T_SPACE) {
			FlushWord(codelist2, codelist);
		} else {
			lex.pos=pos;
			if(!compileWordParts(lex, codelist2)) {
				// 文字列にならないものは、とりあえずそのまま文字列にする
				token=lex.GetToken();
				if(token.str.size())
					codelist2.push_back(new TK7String(token.str));
			}
		}
	}
	FlushWord(codelist2, codelist);

	return (codelist.size()!=oldsize)&&statusflag;
}
//---------------------------------------------------------------------------
// インラインスクリプト (TKawariCodeInlineScript::Compile)
// 戻り値 : 文を 1 つ以上読めたら true
bool TKawari7Compiler::compileInlineScript(TKawari7Lexer &lex, vector<TKVMCode_base *> &stmts)
{
	if(lex.IsEnd()) return false;

	size_t oldsize=stmts.size();
	size_t pos=lex.pos;
	TKawari7Lexer::Token token=lex.GetToken();

	if(token.type!=TKawari7Lexer::Token::T_MACRO_INLINE) {
		lex.pos=pos;
		return false;
	}

	while(true) {
		vector<TKVMCode_base *> args;
		bool status=compileStatement(lex, args);

		if(args.size())
			stmts.push_back(new TK7Statement(args));

		if(!status) break;
	}

	return stmts.size()!=oldsize;
}
//---------------------------------------------------------------------------
// 辞書の行の続きから、カンマで区切られた次の単語を 1 つ得る
TKVMCode_base *TKawari7Compiler::ParseDictWord(TKawari7Lexer &lex)
{
	lex.pos=lex.AcceptSpace();

	vector<TKVMCode_base *> codelist;
	if(!CompileWord(lex, codelist, L',')) return NULL;

	if(codelist.size()==1) return codelist[0];
	return new TKVMCodeList(codelist);
}
//---------------------------------------------------------------------------
// 文字列全体を 1 つの単語として中間コード化する
TKVMCode_base *TKawari7Compiler::CompileString(const wstring &src, TKawariLogger *logger)
{
	TKawari7Lexer lex(src, logger);

	vector<TKVMCode_base *> codelist;
	if(!CompileWord(lex, codelist, 0)) return NULL;

	if(codelist.size()==1) return codelist[0];
	return new TKVMCodeList(codelist);
}
//---------------------------------------------------------------------------
// 文字列をエントリ名で使える文字列にする (TKawariEngine::EncodeEntryName)
// ASCII の英数字以外 (ASCII の記号など) は全て '.' にする。2 バイト文字はそのまま。
wstring TKawari7Compiler::EncodeEntryName(const wstring &orgsen)
{
	wstring entsen;
	for(size_t pos=0; pos<orgsen.size(); pos++) {
		wchar_t c=orgsen[pos];
		if(IsWideNameChar(c)) entsen+=c;
		else if(IsAsciiAlnum(c)) entsen+=c;
		else entsen+=L'.';
	}
	return entsen;
}
//---------------------------------------------------------------------------
