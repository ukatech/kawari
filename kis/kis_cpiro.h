//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 保守的piro --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.03.19  Phase 0.5     Piroject-X
//  2001.05.31  Phase 5.2     KISインターフェース
//  2001.08.25  Phase 6.3     pirocall仕様追加
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_pirocall);
#else
//---------------------------------------------------------------------------
#ifndef KIS_CPIRO_H
#define KIS_CPIRO_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_pirocall : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="pirocall";
		Format_="pirocall piro-type-entry [Word1]";
		Returnval_="string or word of \"script.txt\"";
		Information_="call a piro type entry from the innner piro enigne";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif

