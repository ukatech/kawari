//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript基本クラス
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.14  Phase 0.50    インラインスクリプト整理
//  2001.05.24  Phase 5.1     インタープリタ・コンパイラ化
//
//---------------------------------------------------------------------------
#ifndef KIS_BASE_H
#define KIS_BASE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
// インラインスクリプト関数の基本クラス
// (Th!rdのアイデアをリファイン)
class TKisFunction_base {
protected:

	// 関数情報
	char *Name_;
	char *Format_;
	char *Returnval_;
	char *Information_;

	// インラインスクリプトエンジン
	class TKisEngine *KisEngine;

public:

	TKisFunction_base(void)
	{
		Name_="none.";
		Format_="none.";
		Returnval_="none.";
		Information_="none.";
	}

	// 関数情報取得
	const char* Name(void) const {return(Name_);}
	const char* Format(void) const {return(Format_);}
	const char* Returnval(void) const {return(Returnval_);}
	const char* Information(void) const {return(Information_);}

	// インラインスクリプトエンジン設定
	void NotifyEngine(class TKisEngine *engine)
	{
		KisEngine=engine;
	}

	// 初期化
	virtual bool Init(void)=0;

	// インタープリタ
	virtual string Function(const vector<string>& args)=0;
};
//---------------------------------------------------------------------------
#if 0
class KIS_SampleFunction : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="SampleFunction";
		Format_="SampleFunction [Arg1 ...]";
		Returnval_="string which join all argument";
		Information_="KawariInlineScript sample";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args)
	{
		string ret;
		for(unsigned int i=0;i<args;i++) ret=ret+args[i]+" ";
		return(ret);
	}
};
// これで登録
INLINE_SCRIPT_REGIST(KIS_SampleFunction);
#endif
//---------------------------------------------------------------------------
#endif

