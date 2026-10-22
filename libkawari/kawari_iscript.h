//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// インラインスクリプトエンジン
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.24  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.12  Phase 5.3.2   ダミーコンテキスト
//  2001.06.17  Phase 6.0     インラインスクリプト内の履歴参照のバグ修正
//  2001.07.21  Phase 6.2     関数情報参照
//  2001.08.08  Phase 6.2     関数テーブル参照
//
//---------------------------------------------------------------------------
#ifndef KAWARI_ISCRIPT_H
#define KAWARI_ISCRIPT_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <map>
using namespace std;
//---------------------------------------------------------------------------
class TKisEngine {
public:
	struct TContext {
		vector<string> TempDict;
	};

private:

	// 関数テーブル
	map<string,class TKisFunction_base*> FunctionTable;

	// 関数リスト
	vector<class TKisFunction_base*> FunctionList;

	// 辞書
	class TNS_KawariDictionary *KawariDictionary;

	// エンジン
	class TKawariEngine *KawariEngine;

	vector<TContext*> ContextStack;


public:

	TKisEngine(class TNS_KawariDictionary *dict,class TKawariEngine *engine);
	~TKisEngine();

	// 関数実行
	string FunctionCall(const vector<string>& args);

	// 実行
	string Run(const vector<class TKawariCodeInlineScriptStatement*> codelist);

	// 真偽判定
	bool IsTrue(const string &str)
	{
		return(!((str=="")||(str=="0")||(str=="false")));
	}


	// コンテキスト生成
	void CreateContext(void)
	{
		ContextStack.push_back(new TContext);
	}

	// ダミーコンテキスト生成
	void CreateNullContext(void)
	{
		ContextStack.push_back(NULL);
	}

	// コンテキスト破棄
	void DeleteContext(void)
	{
		if(ContextStack.size()) {
			if(ContextStack.back()) delete ContextStack.back();
			ContextStack.pop_back();
		}
	}

	// コンテキスト取得
	TContext *Context(void)
	{
		if(ContextStack.size()==0) return(NULL);
		return(ContextStack.back());
	}

	// 現在のコンテキストのスタックフレームのポインタを取得
	// (えーいっ、ようはMC68000のLINKだ)
	unsigned int LinkContext(void)
	{
		if(Context()) return(Context()->TempDict.size());
		 else return(0);
	}

	// 現在のコンテキストのスタックフレームのポインタを復帰
	// (えーいっ、ようはMC68000のUNLINKだ)
	void UnlinkContext(unsigned int pos)
	{
		if(Context()) {
			if(pos<Context()->TempDict.size()) Context()->TempDict.resize(pos);
		}
	}


	// 辞書の取得
	class TNS_KawariDictionary *Dictionary(void)
	{
		return(KawariDictionary);
	}

	// エンジンの取得
	class TKawariEngine *Engine(void)
	{
		return(KawariEngine);
	}


	// 関数情報の取得
	const TKisFunction_base* Function(const string& name)
	{
		if(FunctionTable.count(name)==0) return(NULL);
		return(FunctionTable[name]);
	}

	// 関数テーブルの取得
	const vector<class TKisFunction_base*>& GetFunctionTable(void)
	{
		return(FunctionList);
	}

};
//---------------------------------------------------------------------------
#endif
