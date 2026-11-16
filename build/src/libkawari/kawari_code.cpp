//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 中間コード
//
//      Programed by NAKAUE.T (Meister) / Suikyo.
//
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.12  Phase 5.3.2   純粋仮想エントリにおけるコンテキストのバグ修正
//  2001.06.17  Phase 5.4     インラインスクリプト内の単語切り出しのバグ修正
//                            インラインスクリプト内の履歴参照のバグ修正
//                            逆コンパイラ
//  2002.03.10  Phase 7.9.0   辞書の直接アクセス禁止
//  2002.03.17                KIUに合わせてTKisEngineからTKawariVMに名称変更
//                            同じくTKawariCodeからTKVMCodeに名称変更
//  2002.03.18                KIUに合わせてTKawariCompiler分離
//  2002.04.18  Phase 8.0.0   中間コードクラス全取っ替え。
//  2003.01.07  Phase 8.1.0   function定義の獲得、削除を追加
//  2004.01.04  Phase 8.2.0   continue追加
//  2026.10.18  Phase 9.0.0   wchar_t化
//  2026.10.19  Phase 9.1.0   lambda追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_codeexpr.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <cctype>
#include <cstring>
#include <typeinfo>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// Comparator (for STL)
bool TKVMCode_baseP_Less::operator()(const TKVMCode_base *L,const TKVMCode_base *R) const{
	const type_info &lt=typeid(*L);
	const type_info &rt=typeid(*R);
	if(lt==rt) return (L->Less(*R));
	// 型名での比較(std::stringの一時オブジェクトを作らない)
	return(strcmp(lt.name(), rt.name())<0);
}

//---------------------------------------------------------------------------
// 中間コードの基底クラス
wostream& TKVMCode_base::DebugIndent(wostream& os,unsigned int level) const
{
	for(unsigned int i=0;i<level;i++) os << L"  ";
	return(os);
}

//---------------------------------------------------------------------------
// 中間コードの基底クラス(リストを持つもの)
bool TKVMCodeList_base::Less(const TKVMCode_base &r_) const{
	// 比較子で同一クラスであることを確認済みなのでstatic_cast
	const TKVMCodeList_base& r=static_cast<const TKVMCodeList_base&>(r_);
	unsigned int lsize=static_cast<unsigned int>(list.size());
	if(lsize!=r.list.size()) return(lsize<r.list.size());

	for(unsigned int i=0;i<lsize;i++) {
		if (TKVMCode_baseP_Less()(list[i], r.list[i])) return(true);
		if (TKVMCode_baseP_Less()(r.list[i], list[i])) return(false);
	}
	return(false);
}
wostream &TKVMCodeList_base::Debug(wostream& os, unsigned int level) const{
	DebugIndent(os, level) << GetName() << L"(" << endl;
	for (TCodePVector::const_iterator it=list.begin(); it!=list.end(); it++){
		if (*it)
			(*it)->Debug(os, level+1);
		// ASSERTすべきだなぁ
	}
	return DebugIndent(os, level) << L")" << endl;
}

TKVMCodeList_base::TKVMCodeList_base(const vector<TKVMCode_base *> &tmplist){
	list.insert(list.end(), tmplist.begin(), tmplist.end());
	list.resize(list.size());
}

TKVMCodeList_base::~TKVMCodeList_base(){
	for (TCodePVector::iterator it=list.begin(); it!=list.end(); it++){
		if (*it)
			delete (*it);
	}
}

//---------------------------------------------------------------------------
// 中間コードリスト
// 持っているCodeを連続的に実行し、全ての結果を結合して出力
wstring TKVMCodeList::Run(TKawariVM &vm){
	wstring retstr;
	// ここでVMの例外チェック
	for (TCodePVector::iterator it=list.begin(); (it!=list.end())&&(!vm.IsOnException()); it++){
		retstr+=(*it)->Run(vm);
	}
	return retstr;
}

wstring TKVMCodeList::DisCompile(void) const{
	wstring retstr;
	for (TCodePVector::const_iterator it=list.begin(); it!=list.end(); it++){
		retstr+=(*it)->DisCompile();
	}
	return retstr;
}

//--------------------------------------------------------------------------
// 文字列 (Literal)
TKVMCodeString::TKVMCodeString(const wstring &str) : s(str)
{ }

wstring TKVMCodeString::DisCompile(void) const{
	const static wstring illchs=L"\\\"";
	const static wstring wbs=L"\\";
	const static wstring wdq=L"\"";
	const wstring &ws=s;
	wstring retstr=L"\"";
	unsigned int len=static_cast<unsigned int>(ws.size());
	unsigned int pos=0;

	while(pos<len) {
		wstring::size_type pos2=ws.find_first_of(illchs, pos);
		if (pos2==wstring::npos) {
			retstr+=ws.substr(pos);
			break;
		}else{
			retstr+=ws.substr(pos, pos2-pos)+wbs+ws[pos2];
			pos=static_cast<unsigned int>(pos2+1);
		}
	}

	retstr+=wdq;
	return(retstr);
}

//--------------------------------------------------------------------------
// スクリプト文 ( WS ( Word WS ) * )
// 最初の単語がTKVMCodeStringの時、内容を返す
wstring TKVMCodeScriptStatement::GetArg0(void){
	if (!list.size()) return L"";
	TKVMCodeString *str=dynamic_cast<TKVMCodeString *>(list[0]);
	if (!str) return L"";
	return str->s;
}

// 先頭の単語から構文の種類を決める (コンストラクタから一度だけ呼ぶ)
void TKVMCodeScriptStatement::Classify(void){
	kind=K_CALL;
	arg0lit=NULL;
	if (!list.size()) return;
	const TKVMCodeString *str=dynamic_cast<const TKVMCodeString *>(list[0]);
	if (!str) return;
	arg0lit=&(str->s);
	const wstring &a=str->s;
	if(a==L"NULL") kind=K_NULL;
	else if(a==L"break") kind=K_BREAK;
	else if(a==L"continue") kind=K_CONTINUE;
	else if(a==L"return") kind=K_RETURN;
	else if(a==L"function") kind=K_FUNCTION;
	else if(a==L"lambda") kind=K_LAMBDA;
	else if(a==L"rmfunc") kind=K_RMFUNC;
	else if(a==L"while") kind=K_WHILE;
	else if(a==L"until") kind=K_UNTIL;
	else if(a==L"loop") kind=K_LOOP;
	else if(a==L"foreach") kind=K_FOREACH;
	else if(a==L"?") kind=K_RANDOM;
	else if(a==L"silent") kind=K_SILENT;
}

TKVMCodeScriptStatement::~TKVMCodeScriptStatement(){
	delete fcache;
}

wstring TKVMCodeScriptStatement::Run(TKawariVM &vm){
	if (!list.size()) return L"";
	wstring retstr;
	// 2段パースは気持ち悪いよぅ。
	if(kind==K_NULL) {
		retstr=L"";
	}else if(kind==K_BREAK){
		if (vm.Dictionary().CurrentLoop()){
			vm.SetState(TKawariVM::InterpState(TKawariVM::InterpState::BREAK, L""));
		}else{
			// error
			vm.GetLogger().Diag(kawari_log::LOG_ERROR, kawari_log::E_BREAK_OUTOF_LOOP)
				<< RC.S(kawari::resource::ERR_CODE_BREAK_OUTOF_LOOP);
		}
		retstr=L"";
	}else if(kind==K_CONTINUE){
		if (vm.Dictionary().CurrentLoop()){
			vm.SetState(TKawariVM::InterpState(TKawariVM::InterpState::CONTINUE, L""));
		}else{
			// error
			vm.GetLogger().Diag(kawari_log::LOG_ERROR, kawari_log::E_CONTINUE_OUTOF_LOOP)
				<< RC.S(kawari::resource::ERR_CODE_CONTINUE_OUTOF_LOOP);
		}
		retstr=L"";
	}else if(kind==K_RETURN){
		if(list.size()==2){
			unsigned int frame=vm.Dictionary().LinkFrame();
			vm.SetState(TKawariVM::InterpState(
				TKawariVM::InterpState::RETURN, list[1]->Run(vm)));
			vm.Dictionary().UnlinkFrame(frame);
		}else{
			vm.SetState(TKawariVM::InterpState(
				TKawariVM::InterpState::RETURN, L"", false));
		}
	}else if(kind==K_FUNCTION){
		if(list.size()==3){
			wstring funcname=vm.RunWithCurrentContext(list[1]);
			// 確実なコピーは逆コンパイルしかない。
			wstring funcdecl=list[2]->DisCompile();
			if (funcname.size()&&funcdecl.size()){
				TEntry entry=vm.Dictionary().CreateEntry(
					TKawariVM::SYSTEM_FUNCTION_PREFIX+funcname);
				TWordID id=vm.Dictionary().CreateWord(
					TKawariCompiler::Compile(funcdecl, vm.GetLogger()));
				entry.Clear();
				entry.Push(id);
				// 定義した場所を覚える (診断用)
				unsigned int srcfile, srcline;
				if (vm.GetCurrentSource(srcfile, srcline))
					vm.Dictionary().SourceMap().RecordWord(entry.GetID(), id, srcfile, srcline);
			}
		}else if(list.size()==2){
			wstring funcname=vm.RunWithCurrentContext(list[1]);
			if (funcname.size()){
				TEntry entry=vm.Dictionary().GetEntry(
					TKawariVM::SYSTEM_FUNCTION_PREFIX+funcname);
				if (entry.Size()){
					TWordID wid=entry.Index();
					TKVMCode_base *code=vm.Dictionary().GetWordFromID(wid);
					retstr=code->DisCompile();
				}
			}
		}else{
			// エラー
		}
	}else if(kind==K_LAMBDA){
		if(list.size()==2){
			// 確実なコピーは逆コンパイルしかない。
			wstring funcdecl=list[1]->DisCompile();
			if (funcdecl.size()){
				// 既存の関数と重ならない名前を作る。
				wstring funcname;
				do {
					funcname=L"System.Lambda."+IntToString(vm.GetEngine().NextLambdaNumber());
				} while (vm.Dictionary().GetEntry(
					TKawariVM::SYSTEM_FUNCTION_PREFIX+funcname).Size());
				TEntry entry=vm.Dictionary().CreateEntry(
					TKawariVM::SYSTEM_FUNCTION_PREFIX+funcname);
				TWordID id=vm.Dictionary().CreateWord(
					TKawariCompiler::Compile(funcdecl, vm.GetLogger()));
				entry.Clear();
				entry.Push(id);
				// 定義した場所を覚える (診断用)
				unsigned int srcfile, srcline;
				if (vm.GetCurrentSource(srcfile, srcline))
					vm.Dictionary().SourceMap().RecordWord(entry.GetID(), id, srcfile, srcline);
				retstr=funcname;
			}
		}else{
			// エラー
		}
	}else if(kind==K_RMFUNC){
		if (list.size()==2){
			wstring funcname=vm.RunWithCurrentContext(list[1]);
			if (funcname.size())
				vm.Dictionary().GetEntry(
					TKawariVM::SYSTEM_FUNCTION_PREFIX+funcname).Clear();
		}
	}else if(kind==K_WHILE){
		if(list.size()==3) {
			unsigned int frame=vm.Dictionary().LinkFrame();
			vm.Dictionary().StartLoop();
			while(!vm.IsOnWeakException()) {
				// 式文の評価(第1パラメータ)
				wstring cond=list[1]->Run(vm);
				if (!IsTrue(cond)) break;
				vm.Dictionary().UnlinkFrame(frame);
				vm.Dictionary().PushToHistory(cond);
				// trueの時(第2パラメータ)
				retstr+=vm.RunWithCurrentContext(list[2]);
				vm.ResetState(TKawariVM::InterpState::CONTINUE);
			}
			vm.ResetState(TKawariVM::InterpState::BREAK);
			vm.Dictionary().EndLoop();
			vm.Dictionary().UnlinkFrame(frame);
			vm.Dictionary().PushToHistory(retstr);
		} else {
			// エラー
		}
	}else if(kind==K_UNTIL){
		if(list.size()==3) {
			unsigned int frame=vm.Dictionary().LinkFrame();
			vm.Dictionary().StartLoop();
			// 式文の評価(第1パラメータ)
			while(!vm.IsOnWeakException()) {
				wstring cond=list[1]->Run(vm);
				if (IsTrue(cond)) break;
				vm.Dictionary().UnlinkFrame(frame);
				vm.Dictionary().PushToHistory(cond);

				retstr+=vm.RunWithCurrentContext(list[2]);
				vm.ResetState(TKawariVM::InterpState::CONTINUE);
			}
			vm.ResetState(TKawariVM::InterpState::BREAK);
			vm.Dictionary().EndLoop();
			vm.Dictionary().UnlinkFrame(frame);
			vm.Dictionary().PushToHistory(retstr);
		} else {
			// エラー
		}
	}else if(kind==K_LOOP){
		if(list.size()==3) {
			unsigned int frame=vm.Dictionary().LinkFrame();
			vm.Dictionary().StartLoop();
			// 回数の評価(第1パラメータ)
			int limit=StringToInt(list[1]->Run(vm));
			for(int i=0;(i<limit)&&(limit>0)&&(!vm.IsOnWeakException());i++) {
				vm.Dictionary().PushToHistory(IntToString(i));
				// 実行(第2パラメータ)
				retstr+=list[2]->Run(vm);
				vm.Dictionary().UnlinkFrame(frame);
				vm.ResetState(TKawariVM::InterpState::CONTINUE);
			}
			vm.ResetState(TKawariVM::InterpState::BREAK);
			vm.Dictionary().EndLoop();
			vm.Dictionary().UnlinkFrame(frame);
			vm.Dictionary().PushToHistory(retstr);
		} else {
			// エラー
		}
	}else if(kind==K_FOREACH){
		if(list.size()==4) {
			TNS_KawariDictionary &dict=vm.Dictionary();
			unsigned int frame=dict.LinkFrame();
			vm.Dictionary().StartLoop();
			// テンポラリエントリ(第1パラメータ)
			TEntry tmpentry=dict.CreateEntry(
				vm.RunWithCurrentContext(list[1]));

			// 列挙対象(第2パラメータ)
			const wstring listentryname=vm.RunWithCurrentContext(list[2]);
			TEntry listentry=dict.GetEntry(listentryname);

			if((!listentry.AssertIfEmpty(listentryname))&&
			   (tmpentry.IsValid()&&listentry.IsValid())){
				vector<TWordID> wordcol;
				listentry.FindAll(wordcol);
				vector<TWordID>::iterator it=wordcol.begin();
				for(;(it!=wordcol.end())&&(!vm.IsOnWeakException());it++) {
					// 敵は別エントリ中の単語なので、コンテキストを作成
					TKVMCode_base *code=dict.GetWordFromID(*it);
					if (!code) continue;
					wstring wordstr;
					{
						TKawariVM::TWordFrame wordframe(vm, listentry, *it);
						wordstr=vm.RunWithNewContext(code);
					}
					TWordID wid=dict.CreateWord(
						TKawariCompiler::CompileAsString(wordstr));
					tmpentry.Clear();
					tmpentry.Push(wid);

					// 実行(第3パラメータ)
					retstr+=vm.RunWithCurrentContext(list[3]);
					vm.ResetState(TKawariVM::InterpState::CONTINUE);
				}
				vm.ResetState(TKawariVM::InterpState::BREAK);
			}
			vm.Dictionary().EndLoop();
			dict.UnlinkFrame(frame);
			dict.PushToHistory(retstr);
		}else{
			// エラー
		}
	}else if(kind==K_RANDOM){
		if(list.size()>1) {
			// 第1パラメータ以降から一つ選択
			unsigned int index=vm.GetEngine().Random(static_cast<int>(list.size()-1))+1;
			retstr=vm.RunWithCurrentContext(list[index]);
			vm.Dictionary().PushToHistory(retstr);
		}
	}else{
		// 「関数」の処理
		vector<wstring> args;
		args.reserve(list.size());
		for(TCodePVector::iterator it=list.begin();it!=list.end();it++)
			args.push_back((*it)->Run(vm));
		TFunctionCache *fc=NULL;
		if (arg0lit && arg0lit->size() && ((*arg0lit)[0]!=L'.')){
			// 関数名が固定なので、エントリ検索の覚えを使う
			if (!fcache) fcache=new TFunctionCache(TKawariVM::SYSTEM_FUNCTION_PREFIX+*arg0lit);
			fc=fcache;
		}
		retstr=vm.FunctionCall(args, fc);
	}
	vm.Dictionary().PushToHistory(retstr);
	return retstr;
}

wstring TKVMCodeScriptStatement::DisCompile(void) const{
	wstring retstr;
	if (list.size()){
		for (unsigned int i=0,max=static_cast<unsigned int>(list.size()-1);i<max;i++){
			retstr+=list[i]->DisCompile()+L" ";
		}
		retstr+=list.back()->DisCompile();
	}
	return retstr;
}

//-------------------------------------------------------------------------
// インラインスクリプト ( '$(' ScriptStatementSeq ') )
wstring TKVMCodeInlineScript::Run(TKawariVM &vm){
	wstring retstr;
	// $( ... ; ... ; ... )の中では、基本的にはスタックは巻き戻らない
	unsigned int frame=vm.Dictionary().LinkFrame();
	for (TCodePVector::iterator it=list.begin(); (it!=list.end())&&(!vm.IsOnException()); it++){
		// おのおのの行について
		if (lines.size()){
			unsigned int idx=static_cast<unsigned int>(it-list.begin());
			if (idx<lines.size()) vm.SetZoneLine(lines[idx]);
		}
		TKVMCodeScriptStatement *code=dynamic_cast<TKVMCodeScriptStatement *>(*it);
		if (code){
			if(code->IsSilent()){
				retstr=L"";
				continue;
			}
		}
		retstr+=(*it)->Run(vm);
	}
	vm.Dictionary().UnlinkFrame(frame);
	vm.Dictionary().PushToHistory(retstr);
	return retstr;
}

wstring TKVMCodeInlineScript::DisCompile(void) const{
	if(list.size()){
		wstring retstr=L"$(";
		for (unsigned int i=0,max=static_cast<unsigned int>(list.size()-1); i<max; i++)
			retstr+=list[i]->DisCompile()+L";";
		retstr+=list.back()->DisCompile()+L")";
		return retstr;
	}else{
		return L"$( )";
	}
}

//--------------------------------------------------------------------------
// 添え字付きエントリ呼び出し ( '$' EntryWord '[' WS Expression WS ']' )
wstring TKVMCodeEntryIndex::Run(TKawariVM &vm){
	wstring entry_name=entry_id->Run(vm);
	if (!entry_name.size()) return(L"");

	TEntry entry=vm.Dictionary().GetEntry(entry_name);
	if (entry.AssertIfEmpty(entry_name)) return(L"");

	wstring num=expr->Run(vm);
	int index=StringToInt(num);
	if (index<0) index+=entry.Size();
	if (index<0) return(L"");

	TWordID id=entry.Index(index);
	if (!id) return(L"");
	TKVMCode_base *code=vm.Dictionary().GetWordFromID(id);
	if (!code) return(L"");

	TKawariVM::TWordFrame frame(vm, entry, id);
	wstring retstr=vm.RunWithNewContext(code);
	vm.Dictionary().PushToHistory(retstr);
	return retstr;
}

wstring TKVMCodeEntryIndex::DisCompile(void) const{
	const TKVMCodeExpression *code_expr=dynamic_cast<const TKVMCodeExpression *>(expr);
	if (code_expr){
		return L"$"+entry_id->DisCompile()+L"["+code_expr->DisCompileExpression()+L"]";
	}else{
		return L"$"+entry_id->DisCompile()+L"["+expr->DisCompile()+L"]";
	}
}
bool TKVMCodeEntryIndex::Less(const TKVMCode_base &r_) const{
	const TKVMCodeEntryIndex& r=dynamic_cast<const TKVMCodeEntryIndex&>(r_);
	if (TKVMCode_baseP_Less()(entry_id, r.entry_id)) return true;
	if (TKVMCode_baseP_Less()(r.entry_id, entry_id)) return false;

	return TKVMCode_baseP_Less()(expr, r.expr);
}
wostream &TKVMCodeEntryIndex::Debug(wostream& os, unsigned int level) const{
	DebugIndent(os, level) << L"EntryIndex(" << endl;
	entry_id->Debug(os, level+1);
	DebugIndent(os, level) << L")[" << endl;
	expr->Debug(os, level+1);
	return DebugIndent(os, level) << L"]" << endl;
}
TKVMCodeEntryIndex::~TKVMCodeEntryIndex(){
	if (entry_id)
		delete entry_id;
	if (expr)
		delete expr;
}
//-------------------------------------------------------------------------
