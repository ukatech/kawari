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
//                            同じくTKawariCode召靴�ら�KVMCode召靴北松諒儿//  2002.03.18                KIUに合わせてTKawariCompiler分離
//  2002.04.18  Phase 8.0.0   中間コードクラス全取っ替え。
//
//---------------------------------------------------------------------------
#ifndef CODE_H__
#define CODE_H__
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <iostream>
#include <typeinfo>
//---------------------------------------------------------------------------
class TKawariVM;			// VM

class TKVMCode_base;		// 中間コードのインターフェース
class TKVMCodeList_base;	// 中間コードのインターフェース(基底クラス)
class TKVMCodeList;			// 中間コード片

class TKVMCodeString;		// 文字列
class TKVMCodeIDString;		// ID文字列(逆コンパイル結果が異なる)
class TKVMCodeInlineScript;	// インラインスクリプト
class TKVMCodeScriptStatment;	// スクリプト文
class TKVMCodeEntryIndex;	// エントリ配列アクセス
//--------------------------------------------------------------------------
// class TKVMCode_base;
//
// Interface class.
// 中間コードのインターフェース
class TKVMCode_base {
public:
	// 実行
	virtual std::wstring Run(TKawariVM &vm)=0;
	// 逆コンパイル
	virtual std::wstring DisCompile(void) const { return L""; }
	// Debugで出力をそろえる
	virtual std::wostream &DebugIndent(std::wostream& os, unsigned int level=0) const;
	// デバッグ用ツリー表示
	virtual std::wostream &Debug(std::wostream& os, unsigned int level=0) const=0;
	// 子の中間コードを列挙する (静的検査 kosui --check 用。実行経路では使わない)
	// 子を持つクラスは必ず上書きすること (漏れると検査が黙って素通りする)
	virtual void GetChildren(std::vector<const TKVMCode_base *> &out) const {}
	// 序列 同じクラスの場合のみ呼ばれる。
	virtual bool Less(const TKVMCode_base& R_) const =0;
	// デストラクタ
	virtual ~TKVMCode_base () {};
};
//---------------------------------------------------------------------------
// Comparator (for STL)
class TKVMCode_baseP_Less {
public:
	bool operator()(const TKVMCode_base *L,const TKVMCode_base *R) const;
};
//--------------------------------------------------------------------------
typedef std::vector<TKVMCode_base *> TCodePVector;
//--------------------------------------------------------------------------
// class TKVMCodeList_base : public TKVMCode_base;
//
// Interface class.
// リストを持つ中間コードのインターフェース
// (良くない抽象化の例)
class TKVMCodeList_base : public TKVMCode_base{
public:
	std::vector<TKVMCode_base *> list;

	virtual bool Less(const TKVMCode_base &R_) const;
	virtual std::wostream &Debug(std::wostream& os, unsigned int level=0) const;
	virtual void GetChildren(std::vector<const TKVMCode_base *> &out) const {
		for (std::vector<TKVMCode_base *>::const_iterator it=list.begin(); it!=list.end(); ++it)
			if (*it) out.push_back(*it);
	}
	virtual ~TKVMCodeList_base();
	TKVMCodeList_base (const std::vector<TKVMCode_base *> &tmplist);
protected:
	virtual const std::wstring GetName(void) const=0;
};

//--------------------------------------------------------------------------
// 中間コードリスト
// 持っているCodeを連続的に実行し、全ての結果を結合して出力
class TKVMCodeList : public TKVMCodeList_base {
public:
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	virtual ~TKVMCodeList() {}
	TKVMCodeList(const std::vector<TKVMCode_base *> &tmplist) : TKVMCodeList_base(tmplist) {}
protected:
	virtual const std::wstring GetName(void) const { return L""; }
};

//--------------------------------------------------------------------------
// 文字列 (Literal)
// done.
class TKVMCodeString : public TKVMCode_base {
public:
	std::wstring s;

	virtual std::wstring Run(class TKawariVM &vm) { return s; }
	virtual std::wstring DisCompile(void) const;
	virtual bool Less(const TKVMCode_base &r_) const{
		// 比較子で同一クラスであることを確認済みなのでstatic_cast
		const TKVMCodeString& r=static_cast<const TKVMCodeString&>(r_);
		return(s<r.s);
	}
	virtual std::wostream &Debug(std::wostream& os, unsigned int level=0) const {
		return DebugIndent(os, level) << L"S(" << s << L")" << std::endl;
	}
	virtual ~TKVMCodeString() {}
	TKVMCodeString(const std::wstring &str);
};

//--------------------------------------------------------------------------
// ID文字列 (IdLiteral)
class TKVMCodeIDString : public TKVMCodeString {
public:
	virtual std::wstring DisCompile(void) const {
		return s;
	}
	virtual std::wostream &Debug(std::wostream& os, unsigned int level=0) const {
		return DebugIndent(os, level) << L"ID(" << s << L")" << std::endl;
	}
	TKVMCodeIDString(const std::wstring &str) : TKVMCodeString(str) {}
};

//--------------------------------------------------------------------------
// スクリプト文 ( WS ( Word WS ) * )
// 基本的にStatementと同じ。
class TKVMCodeScriptStatement : public TKVMCodeList_base {
public:
	// 先頭の単語が表す構文の種類 (コンパイル時に一度だけ決める。実行のたびに文字列比較しない)
	enum Kind {
		K_CALL=0,	// 関数呼び出し (下のどれでもない)
		K_NULL, K_BREAK, K_CONTINUE, K_RETURN, K_FUNCTION, K_LAMBDA, K_RMFUNC,
		K_WHILE, K_UNTIL, K_LOOP, K_FOREACH, K_RANDOM, K_SILENT
	};

	virtual std::wstring GetArg0(void);
	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	// 先頭の単語が "silent" か
	bool IsSilent(void) const { return kind==K_SILENT; }
	virtual ~TKVMCodeScriptStatement();
	TKVMCodeScriptStatement(const std::vector<TKVMCode_base *> &tmplist):TKVMCodeList_base(tmplist), fcache(NULL) { Classify(); }
private:
	// 先頭の単語から kind と arg0lit を決める
	void Classify(void);
	Kind kind;
	// 先頭の単語が固定の文字列ならその中身 (list[0] が持つので寿命は同じ)。そうでなければ NULL
	const std::wstring *arg0lit;
	// 関数呼び出しのときのユーザ関数エントリ検索の覚え (最初の呼び出しで作る)
	struct TFunctionCache *fcache;
protected:
	virtual const std::wstring GetName(void) const { return L"ScriptStatement"; }
};

//--------------------------------------------------------------------------
// インラインスクリプト ( '$(' ScriptStatementSeq ') )
class TKVMCodeInlineScript : public TKVMCodeList_base {
public:
	// 各文が始まった行 (辞書ファイルの =kis 〜 =end の中身だけ。他は空)
	// 診断で位置を示すためだけに使い、実行結果にも比較にも影響しない。
	std::vector<unsigned int> lines;

	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	virtual ~TKVMCodeInlineScript() {}
	TKVMCodeInlineScript(const std::vector<TKVMCode_base *> &tmplist):TKVMCodeList_base(tmplist) {}
protected:
	virtual const std::wstring GetName(void) const { return L"InlineScriptSubst"; }
};

//--------------------------------------------------------------------------
// 添え字付きエントリ呼び出し
// ( '$' EntryWord '[' WS Expr WS ']' )
class TKVMCodeEntryIndex : public TKVMCode_base {
public:
	TKVMCode_base *entry_id;
	TKVMCode_base *expr;	// 本来TKawariExprが入る

	virtual std::wstring Run(class TKawariVM &vm);
	virtual std::wstring DisCompile(void) const;
	virtual bool Less(const TKVMCode_base &R_) const;
	virtual std::wostream &Debug(std::wostream& os, unsigned int level=0) const;
	virtual void GetChildren(std::vector<const TKVMCode_base *> &out) const {
		if (entry_id) out.push_back(entry_id);
		if (expr) out.push_back(expr);
	}
	virtual ~TKVMCodeEntryIndex();
	TKVMCodeEntryIndex(TKVMCode_base *eid, TKVMCode_base *e):entry_id(eid), expr(e) {}
};
//--------------------------------------------------------------------------
#endif // CODE_H__
