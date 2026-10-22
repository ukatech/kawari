//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- expr --
//
//      Programed by Kouji.U (sky)
//
//  2001.06.02  Phase 0.50β2   First version
//  2001.06.09  BCC5.5.1でコンパイルできるように修正 (NAKAUE.T)
//  2001.07.21  Phase 6.2    config.hが抜けていた (NAKAUE.T)
//  2002.01.07  Phase 7.3     findposサブコマンド追加 (さとー)
//                            splitコマンド追加(さとー)
//
//---------------------------------------------------------------------------
// 関数テーブルへの登録
#ifdef INLINE_SCRIPT_REGIST
INLINE_SCRIPT_REGIST(KIS_expr);
INLINE_SCRIPT_REGIST(KIS_split);
#else
//---------------------------------------------------------------------------
#ifndef KIS_EXPR_H__
#define KIS_EXPR_H__
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_base.h"
#include <string>
#include <vector>
#ifdef EXPR_LOG
#	include <fstream>
#endif

using namespace std;

class KIS_expr : public TKisFunction_base {
public:

    // Initで名前その他の情報を設定してください
    virtual bool Init(void){
        Name_="expr";
        Format_="expr EXPRESSION";
        Returnval_="evalation result";
        Information_="return the value of expression";
        return(true);
    }

private:
    unsigned int Index;
    vector<string> Args;
    inline bool testNext(const string& ref){
        return ((Index<Args.size()) && (Args[Index]==ref));
    }
    string EvalRegex(const std::string& l, const std::string& r);
    string Eval7();
    string Eval6();
//    string Eval5();
    string Eval4();
    string Eval3();
    string Eval2();
    string Eval1();
    string Eval();

#ifdef EXPR_LOG
    ofstream* LogFS;
#endif
public:
    virtual string Function(const vector<string>& args);
};

/*
  eval = eval1 '|' eval1
  eval1 = eval2 '&' eval2
  eval2 = eval3 comparison eval3
  comparison = <, <=, =, ==, !=, >=, >
  eval3 = eval4 num-operator eval4
  num-operator = +, -
  eval4 = eval5 *,/,% eval5
  eval5 = eval6 : eval6
  eval6 = quote, length, match, index, substr / eval7
  eval7 = bare-operand / ( eval )
  */

//---------------------------------------------------------------------------
class KIS_split : public TKisFunction_base {
public:

	// Initで名前その他の情報を設定してください
	virtual bool Init(void){
		Name_="split";
		Format_="split Entry1 string delimiter";
		Returnval_="(NULL)";
		Information_="split a string by delimiter";

		return(true);
	}

	// インタープリタ
	virtual string Function(const vector<string>& args);
};

class tokenizer {
private:

	string str;					// 切り出す文字列
	string cs;					// 区切り文字列
	string::size_type pos;		// 解析開始位置
	string::size_type size;		// 切り出す文字列の長さ

public:

	// トークン切り出し結果を返す構造体
	struct Parse {
		enum TParseState {
			parseOK,	// トークン切り出し成功
			parseEND	// トークンはこれ以上ない
		} ParseState;
		
		string str;		// 切り出したトークン
	};

	// 初期化は切り出す文字列、区切り文字列の順番
	tokenizer(const string s, const string c);
	
	// トークンを切り出す
	Parse token(void);
};
//---------------------------------------------------------------------------
#endif // KIS_EXPR_H__
//---------------------------------------------------------------------------
#endif

