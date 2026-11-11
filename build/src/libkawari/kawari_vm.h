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
#ifndef KAWARI_VM_H__
#define KAWARI_VM_H__
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_code.h"
#include "libkawari/wordcollection.h"
#include "libkawari/kawari_dict.h"
//---------------------------------------------------------------------------
#include <vector>
#include <string>
#include <map>
#include <climits>
#include <cstdlib>
#include <set>
#include <iostream>
//---------------------------------------------------------------------------

//==========================================================================
// 関数呼び出しのたびに「System.Function.<名前>」エントリを探す費用を省くための覚え。
// 先頭の単語が固定の文字列である文ごとに 1 つ持つ。
// エントリは作られる一方で消えないので、登録済みエントリの総数 (generation) が
// 同じなら、前回の検索結果 (見つからなかった、を含む) がそのまま使える。
//==========================================================================
struct TFunctionCache {
	std::wstring entryname;		// "System.Function." + 関数名
	bool valid;					// entry と generation が入っているか
	unsigned int generation;	// 検索したときの登録済みエントリ総数
	TEntry entry;				// 検索結果 (無効なエントリなら無い)
	TFunctionCache(const std::wstring &name) : entryname(name), valid(false), generation(0), entry(NULL, 0) {}
};

//==========================================================================
//
//  華和梨仮想機械
//  Kawari Virtual Machine  (former TKisEngine)
//
//==========================================================================
class TKawariVM : public IDiagContext {
public:

	// 診断用の実行フレーム
	// 今どの単語(または =kis ゾーン)を実行しているかを積み、
	// エラー時に「どの辞書の何行目か」と呼び出し連鎖を示すために使う。
	// 実行の意味には一切関わらず、loglevel にも依存しない (途中で変わっても積み降ろしが崩れないため)。
	struct TFrame {
		bool zone;				// true : 辞書ファイルの =kis ゾーン
		TEntry entry;			// zone=false : 単語を持つエントリ。分からなければ無効なエントリ
		TWordID wid;			// zone=false : 実行中の単語
		unsigned int file;		// zone=true : ファイル名の添字 (TSourceMap)
		unsigned int line;		// zone=true : 実行中の文の行
		TFrame(const TEntry &e, TWordID w) : zone(false), entry(e), wid(w), file(0), line(0) {}
		TFrame(unsigned int f, unsigned int l) : zone(true), entry(NULL, 0), wid(0), file(f), line(l) {}
	};

	// 単語の実行を囲んで、フレームの積み降ろしを確実にする
	class TWordFrame {
	private:
		TKawariVM &vm;
	public:
		TWordFrame(TKawariVM &v, const TEntry &entry, TWordID wid) : vm(v) { vm.PushWordFrame(entry, wid); }
		~TWordFrame() { vm.PopFrame(); }
	};

	// =kis ゾーンの実行を囲む
	class TZoneFrame {
	private:
		TKawariVM &vm;
	public:
		TZoneFrame(TKawariVM &v, unsigned int file, unsigned int line) : vm(v) { vm.PushZoneFrame(file, line); }
		~TZoneFrame() { vm.PopFrame(); }
	};

	// インタプリタ内部状態
	struct InterpState {
		enum StateValue {
			NORMAL,
			CONTINUE,
			BREAK,
			RETURN,
			EXCEPTION
		} state;
		std::wstring mes;
		bool override;
		InterpState (StateValue s, std::wstring str, bool ovr=true);
	};

private:
	// 華和梨
	class TKawariEngine &engine;

	// 辞書
	class TNS_KawariDictionary &dictionary;

	// 関数テーブル
	std::map<std::wstring, class TKisFunction_base *> FunctionTable;

	// 関数リスト
	std::vector<class TKisFunction_base*> FunctionList;

	// 内部状態
	InterpState state;

	// ロガー
	class TKawariLogger &logger;

	// 診断用の実行フレーム
	std::vector<TFrame> frames;

	// フレームの位置 (ファイル名と行) を調べる。分からなければ false
	bool FrameLocation(const TFrame &f, std::wstring &file, int &line) const;
public:
	TKawariVM(class TKawariEngine &e, class TNS_KawariDictionary &d, class TKawariLogger &lgr);

	~TKawariVM();

	//=====================================================================
	// 診断 (実行位置と呼び出し連鎖)
	//=====================================================================

	void PushWordFrame(const TEntry &entry, TWordID wid) { frames.push_back(TFrame(entry, wid)); }
	void PushZoneFrame(unsigned int file, unsigned int line) { frames.push_back(TFrame(file, line)); }
	void PopFrame(void) { if (frames.size()) frames.pop_back(); }

	// 最も内側のフレームが =kis ゾーンなら、実行中の文の行を更新する
	void SetZoneLine(unsigned int line) { if (frames.size()&&frames.back().zone) frames.back().line=line; }

	// 今実行している場所を辞書ファイルの位置として得る (function 文などが定義位置を記録するため)
	bool GetCurrentSource(unsigned int &file, unsigned int &line) const;

	// IDiagContext
	virtual bool GetLocation(std::wstring &file, int &line) const;
	virtual void GetCallChain(std::vector<std::wstring> &lines, unsigned int maxdepth) const;

	// "System.function."
	static const std::wstring SYSTEM_FUNCTION_PREFIX;

	//=====================================================================
	// 中間コード実行機能
	//=====================================================================

	// 新しいコンテキストを作成し、コードを実行する。
	// 実行が終了するとコンテキストは破棄。
	std::wstring RunWithNewContext(class TKVMCode_base *code);

	// 現在のコンテキスト上でコードを実行する。
	// 実行が終了すると履歴参照スタックを実行前のポイントまで巻き戻す。
	// 現在のコンテキストがない場合はRunWithNewContext()を呼ぶ。
	std::wstring RunWithCurrentContext(class TKVMCode_base *code);

	// 関数実行
	// cache : 呼び出し元の文が持つ、ユーザ関数エントリの検索結果の覚え (無ければ NULL)
	std::wstring FunctionCall(const std::vector<std::wstring>& args, TFunctionCache *cache=NULL);

	// 例外状態(continue, break, return, exception)であるかを知る
	bool IsOnException(void) { return (state.state!=InterpState::NORMAL); }

	// 弱い例外状態(break, return, exception)であるかを知る
	bool IsOnWeakException(void) { return (state.state>InterpState::CONTINUE); }

	// 状態値を得る
	const InterpState &GetState(void) { return state; }

	// 状態のセット
	void SetState(const InterpState &st) { state=st; }

	// 状態のリセット
	void ResetState(void) { state=InterpState(InterpState::NORMAL, L""); }

	// 指定状態のリセット
	void ResetState(InterpState::StateValue test) {
		if (state.state==test)
			state=InterpState(InterpState::NORMAL, L"");
	}

	//=====================================================================
	// ユーティリティ
	//=====================================================================

	// ログ
	inline class TKawariLogger &GetLogger(void) {
		return logger;
	}

	// 辞書
	TNS_KawariDictionary &Dictionary(void){
		return dictionary;
	}

	// ビルトインコマンドの情報を得る
	// name : (入力)コマンド名
	// info : (出力)コマンド情報
	// 戻り値(bool) : 存在すればtrue
	bool GetFunctionInfo(const std::wstring &name, struct TKisFunctionInfo &info);

	// ビルトインコマンドのリスト
	// list : (出力)コマンド名リスト
	// 戻り値(unsigned int) : コマンドの数
	unsigned int GetFunctionList(std::vector<std::wstring> &list) const;
};
//---------------------------------------------------------------------------
#endif // KAWARI_VM_H__
