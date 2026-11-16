//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
//  華和梨仮想機械(インラインスクリプトエンジン)
//
//      Programed by NAKAUE.T (Meister) / Suikyo
//
//  2001.05.24  Phase 5.1     インタープリタ・コンパイラ化
//  2001.06.12  Phase 5.3.2   ダミーコンテキスト
//  2001.06.17  Phase 6.0     インラインスクリプト内の履歴参照のバグ修正
//  2001.07.21  Phase 6.2     関数情報参照
//  2001.08.08  Phase 6.2     関数テーブル参照
//  2002.03.10  Phase 7.9.0   kawari_engine_base.h廃止
//                            辞書の直接アクセス禁止
//  2002.03.17                KIUに合わせてTKisEngineからTKawariVMに名称変更
//                            同じくTKawariCode~からTKVMCode~に名称変更
//  2002.04.18  Phase 8.0.0   書き直し
//                            コンテキスト周りは辞書へ
//                            例外状態の実現
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_compiler.h"
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_rc.h"
#include "kis/kis_config.h"
#include "kis/kis_base.h"
#include "kis/kis7.h"
#include "misc/misc.h"
using namespace kawari::resource;
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
// 関数を登録するエントリのプレフィックス
const wstring TKawariVM::SYSTEM_FUNCTION_PREFIX=L"System.Function.";

//---------------------------------------------------------------------------
// インタプリタの状態
TKawariVM::InterpState::InterpState (TKawariVM::InterpState::StateValue s, wstring str, bool ovr) : state(s), mes(str), override(ovr) {}
//---------------------------------------------------------------------------
// コンストラクタ
TKawariVM::TKawariVM(TKawariEngine &eng, TNS_KawariDictionary &d, TKawariLogger &lgr)
	 : engine(eng), dictionary(d),
	   state(InterpState(TKawariVM::InterpState::NORMAL, L"")), logger(lgr) {
	#define INLINE_SCRIPT_REGIST(T) \
	{ \
		TKisFunction_base *func=new T; \
		func->NotifyEngine(&engine); \
		if(func->Init()) { \
			FunctionTable[func->Name()]=func; \
			FunctionList.push_back(func); \
		} else delete func; \
	}

	#include "kis/kis_config.h"

	#undef INLINE_SCRIPT_REGIST

	// 診断の位置情報の提供元になる
	frames.reserve(64);
	logger.SetDiagContext(this);
}

//---------------------------------------------------------------------------
// デストラクタ
TKawariVM::~TKawariVM(){
	if (logger.GetDiagContext()==this)
		logger.SetDiagContext(NULL);
	for (vector<TKisFunction_base *>::iterator it=FunctionList.begin();it!=FunctionList.end();it++)
		if (*it)
			delete (*it);
	for (vector<TKisFunction_base *>::iterator it=LegacyList.begin();it!=LegacyList.end();it++)
		if (*it)
			delete (*it);
}
//---------------------------------------------------------------------------
// 華和梨7互換モードの専用関数表を作る
void TKawariVM::EnableLegacy7(void){
	if (LegacyList.size()) return;
	CreateKis7Commands(LegacyList, &engine);
	for (vector<TKisFunction_base *>::iterator it=LegacyList.begin();it!=LegacyList.end();it++)
		LegacyTable[wstring((*it)->Name())]=(*it);
}
//---------------------------------------------------------------------------
// 華和梨7互換モードの関数実行
// 7 には「未定義の関数は空文字を返すだけ」「ユーザ関数 (System.Function.*) は無い」ので、
// 7 専用の表にあればそれを、なければ 9 の表に任せる。
wstring TKawariVM::FunctionCall7(const vector<wstring>& args){
	if (!args.size()) return L"";
	if (!args[0].size()) return L"";
	map<wstring, TKisFunction_base *>::const_iterator lit=LegacyTable.find(args[0]);
	if (lit!=LegacyTable.end())
		return (lit->second->Function(args));
	return FunctionCall(args);
}
//---------------------------------------------------------------------------
// フレームの位置 (ファイル名と行) を調べる。分からなければ false
bool TKawariVM::FrameLocation(const TFrame &f, wstring &file, int &line) const{
	if (f.zone){
		if (!f.file) return false;
		file=dictionary.SourceMap().FileName(f.file);
		line=f.line;
		return true;
	}
	unsigned int eid=0;
	if (f.entry.IsValid()){
		// 位置を記録しているのはグローバルなエントリだけ
		if (!dictionary.IsGlobalEntry(f.entry)) return false;
		eid=f.entry.GetID();
	}
	TSourceMap::Loc loc;
	if (!dictionary.SourceMap().FindWord(eid, f.wid, loc)) return false;
	file=dictionary.SourceMap().FileName(loc.file);
	line=loc.line;
	return true;
}
//---------------------------------------------------------------------------
// 位置を持つ最も内側のフレームの位置
bool TKawariVM::GetLocation(wstring &file, int &line) const{
	for (vector<TFrame>::const_reverse_iterator it=frames.rbegin(); it!=frames.rend(); ++it)
		if (FrameLocation(*it, file, line)) return true;
	return false;
}
//---------------------------------------------------------------------------
// 今実行している場所を、辞書ファイルの位置として得る
bool TKawariVM::GetCurrentSource(unsigned int &file, unsigned int &line) const{
	for (vector<TFrame>::const_reverse_iterator it=frames.rbegin(); it!=frames.rend(); ++it){
		if (it->zone){
			if (!it->file) continue;
			file=it->file;
			line=it->line;
			return true;
		}
		unsigned int eid=0;
		if (it->entry.IsValid()){
			if (!dictionary.IsGlobalEntry(it->entry)) continue;
			eid=it->entry.GetID();
		}
		TSourceMap::Loc loc;
		if (dictionary.SourceMap().FindWord(eid, it->wid, loc)){
			file=loc.file;
			line=loc.line;
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
// 呼び出し連鎖を内側から外側へ 1 フレーム 1 行で返す
// 書式 : in ${エントリ} at ファイル(行)    (ユーザ関数は in $(関数名))
void TKawariVM::GetCallChain(vector<wstring> &lines, unsigned int maxdepth) const{
	unsigned int count=0;
	for (vector<TFrame>::const_reverse_iterator it=frames.rbegin(); it!=frames.rend(); ++it){
		if (count>=maxdepth){
			unsigned int rest=static_cast<unsigned int>(frames.rend()-it);
			lines.push_back(L"... "+IntToString(rest)+L" more");
			break;
		}
		wstring text=L"in ";
		if (it->zone){
			text+=L"<kis zone>";
		}else if (it->entry.IsValid()){
			wstring name=it->entry.GetName();
			if ((name.size()>SYSTEM_FUNCTION_PREFIX.size())&&
				(name.compare(0, SYSTEM_FUNCTION_PREFIX.size(), SYSTEM_FUNCTION_PREFIX)==0))
				text+=L"$("+name.substr(SYSTEM_FUNCTION_PREFIX.size())+L")";
			else
				text+=L"${"+name+L"}";
		}else{
			text+=L"<word>";
		}
		wstring file;
		int line=0;
		if (FrameLocation(*it, file, line)){
			text+=L" at "+file;
			if (line>0) text+=L"("+IntToString(line)+L")";
		}
		lines.push_back(text);
		count++;
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// コード実行系
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// 新しいコンテキストを作成し、コードを実行する。
// 実行が終了するとコンテキストは破棄。
wstring TKawariVM::RunWithNewContext(class TKVMCode_base *code){
	if (!code) return L"";
	dictionary.CreateContext();
	wstring retstr=code->Run(*this);
	dictionary.DeleteContext();

	// Return状態の解決
	if (GetState().state==InterpState::RETURN){
		if (GetState().mes.size())
			retstr=GetState().mes;
	}

	// 状態のリセット
	ResetState();

	return retstr;
}
//---------------------------------------------------------------------------
// 現在のコンテキスト上でコードを実行する。
// 実行が終了すると履歴参照スタックを実行前のポイントまで巻き戻す。
// 現在のコンテキストがない場合はRunWithNewContext()を呼ぶ。
wstring TKawariVM::RunWithCurrentContext(TKVMCode_base *code){
	if (dictionary.GetContextStackDepth()){
		unsigned int frame=dictionary.LinkFrame();
		wstring retstr=code->Run(*this);
		dictionary.UnlinkFrame(frame);
		return retstr;
	}else{
		// コンテキストがありませんー
		return RunWithNewContext(code);
	}
}
//---------------------------------------------------------------------------
// 関数実行
wstring TKawariVM::FunctionCall(const vector<wstring>& args, TFunctionCache *cache){
	if (!args[0].size()) return (L"");
	// 先頭に'.'がある場合は強制ビルトインコマンド呼び出し
	if (args[0][0]==L'.') {
		wstring comname=args[0].substr(1);
		map<wstring, TKisFunction_base *>::const_iterator fit=FunctionTable.find(comname);
		if (fit!=FunctionTable.end())
			return (fit->second->Function(args));
	}else{
		TEntry entry(NULL, 0);
		if (cache){
			// 文ごとの覚え。エントリの名前→IDは登録総数が変わらない限り変わらない
			unsigned int gen=dictionary.GlobalEntryCount();
			if ((!cache->valid)||(cache->generation!=gen)){
				cache->entry=dictionary.GetEntry(cache->entryname);
				cache->generation=gen;
				cache->valid=true;
			}
			entry=cache->entry;
		}else{
			entry=dictionary.GetEntry(SYSTEM_FUNCTION_PREFIX+args[0]);
		}
		// ユーザ定義関数を優先
		if (entry.IsValid()&&entry.Size()){
			// 最初の単語を実体とする
			TWordID id=entry.Index();
			if (!id) return L"";
			TKVMCode_base *code=dictionary.GetWordFromID(id);

			// コンテキスト作成
			dictionary.CreateContext();

			// 引数格納
			if (args.size()){
				TEntry argentry=dictionary.CreateEntry(L"@arg");
				for(unsigned int i=0; i<args.size(); i++){
					argentry.Push(
						dictionary.CreateWord(
							TKawariCompiler::CompileAsString(args[i])));
				}
			}
			// コード実行
			wstring retstr;
			{
				TWordFrame frame(*this, entry, id);
				retstr=code->Run(*this);
			}

			// コンテキスト削除
			dictionary.DeleteContext();

			// Return状態の解決
			if (GetState().state==InterpState::RETURN){
				if (GetState().override)
					retstr=GetState().mes;
			}
			ResetState();

			return retstr;
		}else{
			// ビルトイン関数
			map<wstring, TKisFunction_base *>::const_iterator fit=FunctionTable.find(args[0]);
			if (fit!=FunctionTable.end())
				return (fit->second->Function(args));
		}
	}
	logger.Diag(kawari_log::LOG_ERROR, kawari_log::E_UNDEFINED_FUNCTION)
		<< RC.S(ERR_VM_UNDEFINED_FUNCTION1) << args[0] << RC.S(ERR_VM_UNDEFINED_FUNCTION2);
	return L"";
}
//---------------------------------------------------------------------------
// ビルトイン関数の情報を得る
bool TKawariVM::GetFunctionInfo(const wstring &name, TKisFunctionInfo &info){
	if (FunctionTable.count(name)){
		info=FunctionTable[name]->GetInformation();
		return true;
	}else{
		return false;
	}
}
//---------------------------------------------------------------------------
// ビルトイン関数のリスト
unsigned int TKawariVM::GetFunctionList(vector<wstring> &list) const{
	for (vector<TKisFunction_base*>::const_iterator it=FunctionList.begin();it!=FunctionList.end();it++)
		list.push_back(wstring((*it)->Name()));
	return static_cast<unsigned int>(FunctionList.size());
}
//---------------------------------------------------------------------------
