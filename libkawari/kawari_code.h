//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 中間コード
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.17  Phase 5.4     逆コンパイラ
//
//---------------------------------------------------------------------------
#ifndef KAWARI_CODE_H
#define KAWARI_CODE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <iostream>
#include <typeinfo>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_lexer.h"
#include "libkawari/kawari_iscript.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 中間コードの基底クラス
class TKawariCode_base {
public:

	// 実行
	virtual string Run(TKisEngine *engine)=0;

	// 逆コンパイル
	virtual string DisCompile(void) const =0;

	// コンテキスト生成
	virtual TKawariCode_base* CreateContext(void) {return(this);}

	// デバッグ用ツリー表示
	virtual void Debug(ostream& os,unsigned int level=0) const =0;

	// Debugで出力をそろえる
	ostream& DebugIndent(ostream& os,unsigned int level) const;

	// 序列
	virtual bool Less(const TKawariCode_base& R_) const =0;

	virtual ~TKawariCode_base() {}
};
//---------------------------------------------------------------------------
class TKawariCode_baseP_Less {
public:
	bool operator()(const TKawariCode_base *L,const TKawariCode_base *R) const
	{
		if(typeid(*L)==typeid(*R)) return (L->Less(*R));
		return(string(typeid(*L).name())<string(typeid(*R).name()));
	}
};
//---------------------------------------------------------------------------
// コンテキストを生成する
class TKawariCodeContext : public TKawariCode_base {
private:

	TKawariCode_base* Code;

public:

	TKawariCodeContext(TKawariCode_base *code) : Code(code) {}


	virtual string Run(TKisEngine *engine);

	virtual string DisCompile(void) const
	{
		return(Code->DisCompile());
	}

	virtual void Debug(ostream& os,unsigned int level=0) const
	{
		DebugIndent(os,level) << "Context:" << endl;
		Code->Debug(os,level+1);
	}

	virtual bool Less(const TKawariCode_base &R_) const
	{
		const TKawariCodeContext& R=dynamic_cast<const TKawariCodeContext&>(R_);
		return TKawariCode_baseP_Less()(Code,R.Code);	// 括弧が無いのはGCC2.95.2のバグ回避
	}

	virtual ~TKawariCodeContext()
	{
		delete Code;
	}

};
//---------------------------------------------------------------------------
// 文字列
class TKawariCodeString : public TKawariCode_base {
private:

	string Data;

public:

	TKawariCodeString(void) {}

	TKawariCodeString(const string& str) : Data(str) {}

	bool Compile(TKawariLexer &source);

	virtual string Get(void) const
	{
		return(Data);
	}

	virtual string Run(TKisEngine *)
	{
		return(Data);
	}

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const
	{
		DebugIndent(os,level) << "String:" << Data << endl;
	}

	virtual bool Less(const TKawariCode_base &R_) const
	{
		const TKawariCodeString& R=dynamic_cast<const TKawariCodeString&>(R_);
		return(Data<R.Data);
	}

};
//---------------------------------------------------------------------------
// ${エントリ名}
class TKawariCodeEntryCall : public TKawariCode_base {
private:

	string EntryName;

public:

	bool Set(const string& entryname)
	{
		EntryName=entryname;
		return(true);
	}

	const string& Get(void)
	{
		return(EntryName);
	}

	virtual string Run(TKisEngine *engine);

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const
	{
		DebugIndent(os,level) << "EntryCall:" << EntryName << endl;
	}

	virtual bool Less(const TKawariCode_base &R_) const
	{
		const TKawariCodeEntryCall& R=dynamic_cast<const TKawariCodeEntryCall&>(R_);
		return(EntryName<R.EntryName);
	}

};
//---------------------------------------------------------------------------
// ${0}
class TKawariCodeTempEntryCall : public TKawariCode_base {
private:

	unsigned int EntryNo;

public:

	bool Set(unsigned int no)
	{
		EntryNo=no;
		return(true);
	}

	virtual string Run(TKisEngine *engine);

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const
	{
		DebugIndent(os,level) << "TempEntryCall:" << EntryNo << endl;
	}

	virtual bool Less(const TKawariCode_base &R_) const
	{
		const TKawariCodeTempEntryCall& R=dynamic_cast<const TKawariCodeTempEntryCall&>(R_);
		return(EntryNo<R.EntryNo);
	}

};
//---------------------------------------------------------------------------
// ${エントリ名&エントリ名}
class TKawariCodeAndEntryCall : public TKawariCode_base {
private:

	vector<string> EntryName;

public:

	bool Set(const vector<string>& key)
	{
		EntryName.insert(EntryName.end(),key.begin(),key.end());
		return(true);
	}


	virtual string Run(TKisEngine *engine);

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const;

	virtual bool Less(const TKawariCode_base &R_) const;

};
//---------------------------------------------------------------------------
// 1単語を保持する
class TKawariCodeWord : public TKawariCode_base {
private:

	vector<TKawariCode_base*> Code;

public:

	bool Add(const vector<TKawariCode_base*>& codelist)
	{
		Code.insert(Code.end(),codelist.begin(),codelist.end());
		return(true);
	}


	bool Add(TKawariCode_base* code)
	{
		Code.push_back(code);
		return(true);
	}


	// 文字列から一単語を切り出し、中間コードにコンパイルする
	static bool Compile(vector<TKawariCode_base*> &codelist,TKawariLexer &source);

	// 文字列から一単語を切り出し、中間コードにコンパイルする
	// 指定の区切り文字以外は全て文字列とする
	// 区切り文字が指定された時は、末尾の空白を除去する
	static bool Compile2(vector<TKawariCode_base*> &codelist,TKawariLexer &source,char separator=0);

	virtual string Run(TKisEngine *engine);

	virtual TKawariCode_base* CreateContext(void)
	{
		return(new TKawariCodeContext(this));
	}

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const;

	virtual bool Less(const TKawariCode_base &R_) const;

	virtual ~TKawariCodeWord();

};
//---------------------------------------------------------------------------
// インラインスクリプト一文
class TKawariCodeInlineScriptStatement : public TKawariCode_base {
private:
public:

	vector<TKawariCode_base*> Code;

	// 文字列からインラインスクリプトを一文切り出し、中間コードにコンパイルする
	// trueで継続,falseでインラインスクリプト終了
	static bool Compile(vector<TKawariCode_base*> &codelist,TKawariLexer &source);

	// 与えられた中間コードを格納(追加)する
	bool Add(const vector<TKawariCode_base*>& codelist)
	{
		Code.insert(Code.end(),codelist.begin(),codelist.end());
		return(true);
	}

	// 与えられた中間コードを格納(追加)する
	bool Add(TKawariCode_base* code)
	{
		Code.push_back(code);
		return(true);
	}

	// 先頭パラメータを評価せずに取得する
	// 先頭パラメータが文字列ではない場合には空文字列を返す
	string GetArg0(void) const;

	virtual string Run(TKisEngine *engine);

	virtual TKawariCode_base* CreateContext(void)
	{
		return(new TKawariCodeContext(this));
	}

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const;

	virtual bool Less(const TKawariCode_base &R_) const;

	virtual ~TKawariCodeInlineScriptStatement();
};
//---------------------------------------------------------------------------
// インラインスクリプト
class TKawariCodeInlineScript : public TKawariCode_base {
private:

	vector<TKawariCodeInlineScriptStatement*> Code;

public:


	bool Add(const vector<TKawariCodeInlineScriptStatement*>& codelist)
	{
		Code.insert(Code.end(),codelist.begin(),codelist.end());
		return(true);
	}


	bool Add(TKawariCodeInlineScriptStatement* code)
	{
		Code.push_back(code);
		return(true);
	}

	// 文字列からインラインスクリプトを切り出し、中間コードにコンパイルする
	static bool Compile(vector<TKawariCodeInlineScriptStatement*> &codelist,TKawariLexer &source);

	virtual string Run(TKisEngine *engine);

	virtual TKawariCode_base* CreateContext(void)
	{
		return(new TKawariCodeContext(this));
	}

	virtual string DisCompile(void) const;

	virtual void Debug(ostream& os,unsigned int level=0) const;

	virtual bool Less(const TKawariCode_base &R_) const;

	virtual ~TKawariCodeInlineScript();
};
//---------------------------------------------------------------------------
#endif
