//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 条件式 --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.30  Phase 5.1     First version
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_test);
INLINE_SCRIPT_REGIST(KIS_test2);
#else
//---------------------------------------------------------------------------
#ifndef KIS_TEST_H
#define KIS_TEST_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_test : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="test";
		Format_="test CONDITION_EXPRESSION";
		Returnval_="\"true\" or \"false\"";
		Information_="return a value of condition expression";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
class KIS_test2 : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="[";
		Format_="[ CONDITION_EXPRESSION ]";
		Returnval_="\"true\" or \"false\"";
		Information_="return a value of condition expression";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif

