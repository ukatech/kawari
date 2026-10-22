//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 中間コード
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.12  Phase 5.3.2   純粋仮想エントリにおけるコンテキストのバグ修正
//  2001.06.17  Phase 5.4     インラインスクリプト内の単語切り出しのバグ修正
//                            インラインスクリプト内の履歴参照のバグ修正
//                            逆コンパイラ
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <cstdlib>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_dict.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 中間コードの基底クラス
ostream& TKawariCode_base::DebugIndent(ostream& os,unsigned int level) const
{
	for(unsigned int i=0;i<level;i++) os << "    ";
	return(os);
}
//---------------------------------------------------------------------------
// 文字列
bool TKawariCodeString::Compile(TKawariLexer &source)
{
	string word;

	bool flag=false;
	while((!source.IsEnd())&&(!flag)) {
		unsigned int pos=source.Pos;
		TKawariLexer::TToken token=source.GetToken();

		switch(token.Type) {
		case TKawariLexer::TToken::ttokSentence:
			// 文字列だった
			word+=token.Str;
			break;
		case TKawariLexer::TToken::ttokSeparator:
			// 区切り文字の中には単語扱いしてよいものもある
			switch(token.Str[0]) {
			case ']':
				word+=token.Str;
				break;
			default:
				flag=true;
			}
			break;
		default:
			flag=true;
		}

		if(flag) {
			// バックトラック
			source.Pos=pos;
		}
	}

	if(word.size()==0) return(false);

	Data=word;

	return(true);
}
//---------------------------------------------------------------------------
#if defined(STRING_APPEND_INATOMIC)
string TKawariCodeString::DisCompile(void) const
{
	string retstr="\"";
	unsigned int cnt = Data.size();
	unsigned int pos = 0;

	while(pos < cnt) {
		unsigned int pos2 = Data.find_first_of('"', pos);
		if (pos2 == string::npos) {
			if ((cnt - pos) > 0) retstr += Data.substr(pos, cnt - pos);
			break;
		}
		if ((pos2 - pos) > 0) retstr += Data.substr(pos, pos2 - pos);
		retstr += "\\\"";
		pos = pos2 + 1;
	}

	retstr+="\"";

	return(retstr);
}
#else
string TKawariCodeString::DisCompile(void) const
{
	string retstr="\"";

	for(unsigned int i=0;i<Data.size();i++) {
		if(Data[i]!='"') retstr+=Data[i];
		 else retstr+="\\\"";
	}

	retstr+="\"";

	return(retstr);
}
#endif
//---------------------------------------------------------------------------
// ${エントリ名}
string TKawariCodeEntryCall::Run(TKisEngine *engine)
{
	TEntryID entry=engine->Dictionary()->GetEntryID(EntryName);

	if(entry==0) return("");

	vector<TWordID> wordlist;
	engine->Dictionary()->FindAll(entry,wordlist);

	if(wordlist.size()==0) return("");

	TWordID id=wordlist[Random(wordlist.size())];
	TKawariCode_base *code=engine->Dictionary()->GetWordFromID(id);

	if(!code) return("");

	engine->CreateNullContext();
	string retstr=code->Run(engine);
	engine->DeleteContext();

	if(engine->Context()) engine->Context()->TempDict.push_back(retstr);

	return(retstr);
}
//---------------------------------------------------------------------------
string TKawariCodeEntryCall::DisCompile(void) const
{
	return(string("${")+EntryName+"}");
}
//---------------------------------------------------------------------------
// ${0}
string TKawariCodeTempEntryCall::Run(TKisEngine *engine)
{
	if(!engine->Context()) return("");

	if(engine->Context()->TempDict.size()<=EntryNo) return("");

	return(engine->Context()->TempDict[EntryNo]);
}
//---------------------------------------------------------------------------
string TKawariCodeTempEntryCall::DisCompile(void) const
{
	return(string("${")+IntToString(EntryNo)+"}");
}
//---------------------------------------------------------------------------
// ${エントリ名&エントリ名}
string TKawariCodeAndEntryCall::Run(TKisEngine *engine)
{
	vector<TEntryID> key;
	for(unsigned int i=0;i<EntryName.size();i++) {
		TEntryID entry=engine->Dictionary()->GetEntryID(EntryName[i]);
		if(entry) key.push_back(entry);
	}

	if(key.size()==0) return("");

	set<TWordID> wordset;
	engine->Dictionary()->GetWordCollection(key,wordset);

	if(wordset.size()==0) return("");

	unsigned int index=Random(wordset.size());

	set<TWordID>::iterator it=wordset.begin();
	for(unsigned int i=0;i<index;i++) it++;

	TKawariCode_base *code=engine->Dictionary()->GetWordFromID(*it);

	if(!code) return("");

	engine->CreateNullContext();
	string retstr=code->Run(engine);
	engine->DeleteContext();

	if(engine->Context()) engine->Context()->TempDict.push_back(retstr);

	return(retstr);
}
//---------------------------------------------------------------------------
string TKawariCodeAndEntryCall::DisCompile(void) const
{
	string retstr="${";

	for(unsigned int i=0;i<EntryName.size()-1;i++) retstr+=EntryName[i]+"&";
	retstr+=EntryName.back()+"}";

	return(retstr);
}
//---------------------------------------------------------------------------
void TKawariCodeAndEntryCall::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "EntryName:";
	for(unsigned int i=0;i<EntryName.size()-1;i++) os << EntryName[i] << "&";
	os << EntryName.back() << endl;

	return;
}
//---------------------------------------------------------------------------
bool TKawariCodeAndEntryCall::Less(const TKawariCode_base &R_) const
{
	const TKawariCodeAndEntryCall& R=dynamic_cast<const TKawariCodeAndEntryCall&>(R_);

	if(EntryName.size()!=R.EntryName.size()) return(EntryName.size()<R.EntryName.size());

	for(unsigned int i=0;i<EntryName.size();i++) {
		if(EntryName[i]<R.EntryName[i]) return(true);
		if(EntryName[i]>R.EntryName[i]) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
// 1単語を保持する
string TKawariCodeWord::Run(TKisEngine *engine)
{
	string retstr;
	for(unsigned int i=0;i<Code.size();i++) {
		retstr+=Code[i]->Run(engine);
	}

	return(retstr);
}
//---------------------------------------------------------------------------
string TKawariCodeWord::DisCompile(void) const
{
	string retstr;

	for(unsigned int i=0;i<Code.size();i++) retstr+=Code[i]->DisCompile();

	return(retstr);
}
//---------------------------------------------------------------------------
void TKawariCodeWord::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "Word:" << endl;
	for(unsigned int i=0;i<Code.size();i++) Code[i]->Debug(os,level+1);

	return;
}
//---------------------------------------------------------------------------
bool TKawariCodeWord::Less(const TKawariCode_base &R_) const
{
	const TKawariCodeWord& R=dynamic_cast<const TKawariCodeWord&>(R_);

	if(Code.size()!=R.Code.size()) return(Code.size()<R.Code.size());

	for(unsigned int i=0;i<Code.size();i++) {
		TKawariCode_base *l=Code[i];
		TKawariCode_base *r=R.Code[i];

		if(TKawariCode_baseP_Less()(l,r)) return(true);
		if(TKawariCode_baseP_Less()(r,l)) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
TKawariCodeWord::~TKawariCodeWord()
{
	for(unsigned int i=0;i<Code.size();i++) delete Code[i];
}
//---------------------------------------------------------------------------
// コンテキストを生成する
string TKawariCodeContext::Run(TKisEngine *engine)
{
	// コンテキスト生成
	engine->CreateContext();

	string retstr=Code->Run(engine);

	// コンテキスト破棄
	engine->DeleteContext();

	return(retstr);
}
//---------------------------------------------------------------------------
// インラインスクリプト一文
string TKawariCodeInlineScriptStatement::Run(TKisEngine *engine)
{
	vector<string> args;

	for(unsigned int i=0;i<Code.size();i++) {
		args.push_back(Code[i]->Run(engine));
	}

	return(engine->FunctionCall(args));
}
//---------------------------------------------------------------------------
string TKawariCodeInlineScriptStatement::DisCompile(void) const
{
	string retstr;

	for(unsigned int i=0;i<Code.size();i++) retstr+=Code[i]->DisCompile()+" ";

	return(retstr);
}
//---------------------------------------------------------------------------
void TKawariCodeInlineScriptStatement::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "Statement:" << endl;
	for(unsigned int i=0;i<Code.size();i++) Code[i]->Debug(os,level+1);

	return;
}
//---------------------------------------------------------------------------
bool TKawariCodeInlineScriptStatement::Less(const TKawariCode_base &R_) const
{
	const TKawariCodeInlineScriptStatement& R=dynamic_cast<const TKawariCodeInlineScriptStatement&>(R_);

	if(Code.size()!=R.Code.size()) return(Code.size()<R.Code.size());

	for(unsigned int i=0;i<Code.size();i++) {
		TKawariCode_base *l=Code[i];
		TKawariCode_base *r=R.Code[i];

		if(TKawariCode_baseP_Less()(l,r)) return(true);
		if(TKawariCode_baseP_Less()(r,l)) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
TKawariCodeInlineScriptStatement::~TKawariCodeInlineScriptStatement()
{
	for(unsigned int i=0;i<Code.size();i++) delete Code[i];
}
//---------------------------------------------------------------------------
// インラインスクリプト一文
// 先頭パラメータを評価せずに取得する
// 先頭パラメータが文字列ではない場合には空文字列を返す
string TKawariCodeInlineScriptStatement::GetArg0(void) const
{
	TKawariCodeString *str=dynamic_cast<TKawariCodeString*>(Code[0]);
	if(!str) return("");

	return(str->Get());
}
//---------------------------------------------------------------------------
// インラインスクリプト
string TKawariCodeInlineScript::Run(TKisEngine *engine)
{
	unsigned int frame=engine->LinkContext();

	string retstr=engine->Run(Code);

	engine->UnlinkContext(frame);

	return(retstr);
}
//---------------------------------------------------------------------------
string TKawariCodeInlineScript::DisCompile(void) const
{
	string retstr="$(";

	for(unsigned int i=0;i<Code.size()-1;i++) retstr+=Code[i]->DisCompile()+";";
	retstr+=Code.back()->DisCompile()+")";

	return(retstr);
}
//---------------------------------------------------------------------------
void TKawariCodeInlineScript::Debug(ostream& os,unsigned int level) const
{
	DebugIndent(os,level) << "InlineScript:" << endl;
	for(unsigned int i=0;i<Code.size();i++) Code[i]->Debug(os,level+1);

	return;
}
//---------------------------------------------------------------------------
bool TKawariCodeInlineScript::Less(const TKawariCode_base &R_) const
{
	const TKawariCodeInlineScript& R=dynamic_cast<const TKawariCodeInlineScript&>(R_);

	if(Code.size()!=R.Code.size()) return(Code.size()<R.Code.size());

	for(unsigned int i=0;i<Code.size();i++) {
		TKawariCode_base *l=Code[i];
		TKawariCode_base *r=R.Code[i];

		if(TKawariCode_baseP_Less()(l,r)) return(true);
		if(TKawariCode_baseP_Less()(r,l)) return(false);
	}

	return(false);
}
//---------------------------------------------------------------------------
TKawariCodeInlineScript::~TKawariCodeInlineScript()
{
	for(unsigned int i=0;i<Code.size();i++) delete Code[i];
}
//---------------------------------------------------------------------------
// 文字列から一単語を切り出し、中間コードにコンパイルする
bool TKawariCodeWord::Compile(vector<TKawariCode_base*> &codelist,TKawariLexer &source)
{
	unsigned int oldsize=codelist.size();

	while(!source.IsEnd()) {
		unsigned int pos=source.Pos;
		TKawariLexer::TToken token=source.GetToken();

		bool flag=false;
		switch(token.Type) {
		case TKawariLexer::TToken::ttokSpace :
			// 空白文字列だったのでバックトラック
			source.Pos=pos;
			flag=true;
			break;
		case TKawariLexer::TToken::ttokMacroEntry :
			// エントリ呼び出し
			{
				vector<string> key;
				source.GetEntryList(key);

				if(!source.IsEnd()&&(source[source.Pos]=='}')) {
					source.Pos++;
					if(key.size()==1) {
						if(('0'<=key[0][0])&&(key[0][0]<='9')) {
							// 一時エントリ呼び出し
							TKawariCodeTempEntryCall *code=new TKawariCodeTempEntryCall;
							code->Set((unsigned int)atoi(key[0].c_str()));
							codelist.push_back(code);
						} else {
							// 単独エントリ呼び出し
							TKawariCodeEntryCall *code=new TKawariCodeEntryCall;
							code->Set(key[0]);
							codelist.push_back(code);
						}
					} else if(key.size()>1) {
						// ANDエントリ呼び出し
						TKawariCodeAndEntryCall *code=new TKawariCodeAndEntryCall;
						code->Set(key);
						codelist.push_back(code);
					}
				} else {
					// ${エントリ名}として解釈できなかったので
					// 文字列にしてしまう
					TKawariCodeString *str=new TKawariCodeString(source.substr(pos,source.Pos-pos));
					codelist.push_back(str);
				}
			}
			break;
		case TKawariLexer::TToken::ttokMacroInline :
			// $(インラインスクリプト)
			source.Pos=pos;
			{
				vector<TKawariCodeInlineScriptStatement*> codelist2;
				if(TKawariCodeInlineScript::Compile(codelist2,source)) {
					TKawariCodeInlineScript *code=new TKawariCodeInlineScript;
					code->Add(codelist2);
					codelist.push_back(code);
				} else {
					// $(インラインスクリプト)として解釈できなかったので
					// 文字列にしてしまう
					TKawariCodeString *str=new TKawariCodeString(source.substr(pos,source.Pos-pos));
					codelist.push_back(str);
				}
			}
			break;
		default:
			// 文字列・・・かもしれない
			source.Pos=pos;
			{
				TKawariCodeString *code=new TKawariCodeString;
				if(code->Compile(source)) {
					codelist.push_back(code);
				} else {
					// 文字列ではなかったのでバックトラック
					delete code;
					source.Pos=pos;
					flag=true;
				}
			}
		}

		if(flag) break;
	}

	return(codelist.size()!=oldsize);
}
//---------------------------------------------------------------------------
// 文字列から一単語を切り出し、中間コードにコンパイルする
// 指定の区切り文字以外は全て文字列とする
// 区切り文字が指定された時は、末尾の空白を除去する
bool TKawariCodeWord::Compile2(vector<TKawariCode_base*> &codelist,TKawariLexer &source,char separator)
{
	unsigned int oldsize=codelist.size();
	string ws;

	while(!source.IsEnd()) {
		unsigned pos=source.Pos;
		TKawariLexer::TToken token=source.GetToken();

		// 単語の区切りの,だった
		if((token.Type==TKawariLexer::TToken::ttokSeparator)&&(token.Str[0]==separator)) break;

		// それ以外
		switch(token.Type) {
		case TKawariLexer::TToken::ttokSpace :
			// 空白文字列
			ws=token.Str;
			break;
		default:
			source.Pos=pos;
			{
				vector<TKawariCode_base*> codelist2;

				if(TKawariCodeWord::Compile(codelist2,source)) {
					if(ws.size()) {
						TKawariCodeString *str=new TKawariCodeString(ws);
						codelist.push_back(str);
						ws="";
					}
					for(unsigned int i=0;i<codelist2.size();i++) {
						codelist.push_back(codelist2[i]);
					}
				} else {
					token=source.GetToken();
					if(token.Str.size()) {
						TKawariCodeString *str=new TKawariCodeString(ws+token.Str);
						codelist.push_back(str);
						ws="";
					}
				}
			}
		}
	}

	if(ws.size()&&(separator==0)) {
		TKawariCodeString *str=new TKawariCodeString(ws);
		codelist.push_back(str);
	}

	return(codelist.size()!=oldsize);
}
//---------------------------------------------------------------------------
// 文字列からインラインスクリプトを一文切り出し、中間コードにコンパイルする
// trueで継続,falseでインラインスクリプト終了
bool TKawariCodeInlineScriptStatement::Compile(vector<TKawariCode_base*> &codelist,TKawariLexer &source)
{
	unsigned int oldsize=codelist.size();
	bool statusflag=false;

	vector<TKawariCode_base*> codelist2;
	while(!source.IsEnd()) {
		unsigned pos=source.Pos;
		TKawariLexer::TToken token=source.GetToken();

		if(token.Type==TKawariLexer::TToken::ttokSeparator) {
			if(token.Str==")") {
				// インラインスクリプト終了の")"だった
				break;
			} else if(token.Str==";") {
				// マルチステートメントの区切り
				statusflag=true;
				break;
			}
		}

		switch(token.Type) {
		case TKawariLexer::TToken::ttokSpace :
			// 空白文字列
			if(codelist2.size()==1) {
				codelist.push_back(codelist2[0]);
			} else if(codelist2.size()>1) {
				TKawariCodeWord *code=new TKawariCodeWord;
				code->Add(codelist2);
				codelist.push_back(code);
			}
			codelist2.clear();
			break;
		default:
			// 基本的に単語
			source.Pos=pos;
			if(!TKawariCodeWord::Compile(codelist2,source)) {
				// 分からないものはとりあえず文字列
				token=source.GetToken();
				if(token.Str.size()) {
					TKawariCodeString *str=new TKawariCodeString(token.Str);
					codelist2.push_back(str);
				}
			}
		}
	}

	if(codelist2.size()==1) {
		codelist.push_back(codelist2[0]);
	} else if(codelist2.size()>1) {
		TKawariCodeWord *code=new TKawariCodeWord;
		code->Add(codelist2);
		codelist.push_back(code);
	}

	return((codelist.size()!=oldsize)&&statusflag);
}
//---------------------------------------------------------------------------
// 文字列からインラインスクリプトを切り出し、中間コードにコンパイルする
bool TKawariCodeInlineScript::Compile(vector<TKawariCodeInlineScriptStatement*> &codelist,TKawariLexer &source)
{
	if(source.IsEnd()) return(false);

	unsigned int oldsize=codelist.size();
	unsigned int pos=source.Pos;
	TKawariLexer::TToken token=source.GetToken();

	if(token.Type!=TKawariLexer::TToken::ttokMacroInline) {
		// バックトラック
		source.Pos=pos;
		return(false);
	}

	while(true) {
		vector<TKawariCode_base*> codelist2;

		bool status=TKawariCodeInlineScriptStatement::Compile(codelist2,source);

		if(codelist2.size()) {
			TKawariCodeInlineScriptStatement *code=new TKawariCodeInlineScriptStatement;
			code->Add(codelist2);
			codelist.push_back(code);
		}

		if(!status) break;
	}

	return(codelist.size()!=oldsize);
}
//---------------------------------------------------------------------------
