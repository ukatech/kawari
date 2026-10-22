//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// インラインスクリプトエンジン
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.24  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.10  Phase 5.3.1   ifのバグフィックス
//  2001.06.17  Phase 5.4     foreach追加(by 酔狂さん)
//  2001.07.14  Phase 6.1     NULL追加
//  2001.07.21  Phase 6.2     foreachではコンテキストを生成
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_iscript.h"
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_engine.h"
#include "kis/kis_config.h"
//---------------------------------------------------------------------------
TKisEngine::TKisEngine(class TNS_KawariDictionary *dict,class TKawariEngine *engine)
 : KawariDictionary(dict),KawariEngine(engine)
{
	#define INLINE_SCRIPT_REGIST(T) \
	{ \
		TKisFunction_base *func=new T; \
		func->NotifyEngine(this); \
		if(func->Init()) { \
			FunctionTable[func->Name()]=func; \
			FunctionList.push_back(func); \
		} else delete func; \
	}

	#include "kis/kis_config.h"

	#undef INLINE_SCRIPT_REGIST
}
//---------------------------------------------------------------------------
TKisEngine::~TKisEngine()
{
	for(unsigned int i=0;i<FunctionList.size();i++) delete FunctionList[i];
}
//---------------------------------------------------------------------------
// 関数実行
string TKisEngine::FunctionCall(const vector<string>& args)
{
	if(FunctionTable.count(args[0])==0) return("");

	return(FunctionTable[args[0]]->Function(args));
}
//---------------------------------------------------------------------------
// 実行
string TKisEngine::Run(const vector<class TKawariCodeInlineScriptStatement*> codelist)
{
	unsigned int pc=0;

	string retstr;

	while(true) {
		if(pc>=codelist.size()) break;

		TKawariCodeInlineScriptStatement *code=codelist[pc];

		string result;

		// 最初のパラメータだけ展開する
		string arg0=code->GetArg0();

		// 「構文の処理」

		if(arg0=="NULL") {
			result="";
		} else if(arg0=="if") {
			if((code->Code.size()==3)||(code->Code.size()==4)) {
				// 式文の評価(第1パラメータ)
				if(IsTrue(code->Code[1]->Run(this))) {
					// trueの時(第2パラメータ)
					result=code->Code[2]->Run(this);
				} else {
					// falseの時(第3パラメータ)
					if(code->Code.size()==4) result=code->Code[3]->Run(this);
				}
			} else {
				// エラー
			}
		} else if(arg0=="while") {
			if(code->Code.size()==3) {
				// 式文の評価(第1パラメータ)
				while(IsTrue(code->Code[1]->Run(this))) {
					// trueの時(第2パラメータ)
					CreateContext();	// コンテキスト生成
					result+=code->Code[2]->Run(this);
					DeleteContext();	// コンテキスト破棄
				}
			} else {
				// エラー
			}
		} else if(arg0=="until") {
			if(code->Code.size()==3) {
				// 式文の評価(第1パラメータ)
				while(!IsTrue(code->Code[1]->Run(this))) {
					// trueの時(第2パラメータ)
					CreateContext();	// コンテキスト生成
					result+=code->Code[2]->Run(this);
					DeleteContext();	// コンテキスト破棄
				}
			} else {
				// エラー
			}
		} else if(arg0=="loop") {
			if(code->Code.size()==3) {
				// 回数の評価(第1パラメータ)
				int limit=atoi(code->Code[1]->Run(this).c_str());
				for(int i=0;(i<limit)&&(limit>0);i++) {
					// 実行(第2パラメータ)
					CreateContext();	// コンテキスト生成
					result+=code->Code[2]->Run(this);
					DeleteContext();	// コンテキスト破棄
				}
			} else {
				// エラー
			}
		} else if(arg0=="foreach") {
			// foreach 書いてくれた人：酔狂さん
			if(code->Code.size()==4) {
				// テンポラリエントリ(第1パラメータ)
				string tmpentryname=code->Code[1]->Run(this);
				// 列挙対象(第2パラメータ)
				string listrefname=code->Code[2]->Run(this);
				if(listrefname.size()) {
					TEntryID listref=KawariDictionary->GetEntryID(listrefname);
					vector<TWordID> wordcol;
					KawariDictionary->FindAll(listref,wordcol);
					for(unsigned int i=0;i<wordcol.size();i++) {
						TKawariCode_base *item=KawariDictionary->GetWordFromID(wordcol[i]);
						KawariEngine->InsertAfterClear(tmpentryname,item->Run(this));

						// 実行(第3パラメータ)
						CreateContext();	// コンテキスト生成
						result+=code->Code[3]->Run(this);
						DeleteContext();	// コンテキスト破棄
					}
				}
			}else{
				// エラー
			}
		} else if(arg0=="?") {
			if(code->Code.size()>1) {
				// 第1パラメータ以降から一つ選択
				unsigned int index=Random(code->Code.size()-1)+1;
				result+=code->Code[index]->Run(this);
			}
		} else if(arg0=="silent") {
			retstr="";
		} else {
			// 「関数」の処理
			result=code->Run(this);
		}

		retstr+=result;
		pc++;
	}

	return(retstr);
}
//---------------------------------------------------------------------------

