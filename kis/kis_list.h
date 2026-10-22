//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- リスト --
//
//      Programed by Kouji.U (Suikyo)
//
//  2001.05.16  Phase 0.01
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_shift);
INLINE_SCRIPT_REGIST(KIS_unshift);
INLINE_SCRIPT_REGIST(KIS_push);
INLINE_SCRIPT_REGIST(KIS_pop);
#else
//---------------------------------------------------------------------------
#ifndef KIS_LIST_H
#define KIS_LIST_H
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
//---------------------------------------------------------------------------
class KIS_shift : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="shift";
		Format_="shift エントリ1";
		Returnval_="単語";
		Information_="エントリ1の先頭単語を抜き取り、その値を返す";

		return(true);
	}

	// インタープリタ
  virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
class KIS_unshift : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="unshift";
		Format_="unshift エントリ1 単語1";
		Returnval_="なし";
		Information_="エントリ1の先頭に単語1を加える";

		return(true);
	}

	// インタープリタ
  virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
class KIS_push : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="push";
		Format_="push エントリ1 単語1";
		Returnval_="なし";
		Information_="エントリ1の最後に単語1を加える(adddictと等価)";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args)
	{
		if(args.size()>=3) KawariDictionary->Insert(args[1],args[2]);
		return("");
	}
};
//---------------------------------------------------------------------------
class KIS_pop : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void)
	{
		Name_="pop";
		Format_="pop エントリ1";
		Returnval_="単語";
		Information_="エントリ1の末尾単語を抜き取り、その値を返す";

		return(true);
	}

	// インタープリタ
  virtual string Function(const vector<string>& args);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#endif

