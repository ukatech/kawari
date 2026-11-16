//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨エンジン
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.05.27  Phase 5.1     インタープリタ・コンパイラ化
//                            クラス階層整理
//  2001.05.31  Phase 5.2     保守的piro
//  2001.06.09  Phase 5.3     マッチエントリ
//  2001.06.10  Phase 5.3.1   微調整
//  2001.06.17  Phase 5.4     save
//  2001.08.25  Phase 7.0     セキュリティ対策(WriteProtect)
//  2001.12.08  Phase 7.1.2   テキストファイル読み込み対応
//  2002.03.10  Phase 7.9.0   Parseの宣言をbaseに移動
//                            テキストファイルの読み込みを外す(KISじゃないの？)
//                            そもそもFISの開発中止で意味のなくなってた
//                            TNS_Engine_baseを廃止
//                            ついでにマッチ辞書廃止
//                            辞書アクセスインターフェース強化
//  2002.04.18  Phase 8.0.0   VMの変更に追従
//                            SAORI対応
//  2005.06.28  Phase 8.2.3   DecodeEntryNameに代えてGetEntryRange導入
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#ifndef KAWARI_ENGINE_H
#define KAWARI_ENGINE_H
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <set>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_compiler.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_lint.h"
#include "libkawari/kawari7.h"
#include "saori/saori.h"
#include "misc/dsstp.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
struct TEntryRange;
//---------------------------------------------------------------------------
class TKawariEngine {
private:
	// 辞書ファイルのパス名
	wstring DataPath;

	// ログ
	TKawariLogger *logger;

	// 辞書
	TNS_KawariDictionary *Dictionary;

	// スクリプトエンジン
	class TKawariVM *KawariVM;

	// SAORI
	class saori::TSaoriPark *SaoriPark;

	// 本体(SSP, ninixなど)への接続 (Direct SSTP)
	dsstp::TBasewareLink *Baseware;

	// 安全モード
	bool Sandbox;

	// 静的検査のために残す =kis ゾーンの中間コード
	bool KeepZones;
	vector<TKawariLint::Zone> Zones;
	// 読み込み中にコンパイラが集める所見と抑制コメント (KeepZones のときだけ集める)
	TLintNotes LintNotes;

	// 華和梨7互換モード (ゴースト単位。辞書を読む前に SetLegacy7 で決める)
	bool Legacy7;
	TKawari7State *Legacy7State;

	// 単語の文字列を、モードに応じた文法で中間コードにする
	// 7 モードで空の文字列なら NULL (7 は空の単語を作らない)
	TKVMCode_base *CompileWordString(const wstring &word) const;

	// 7 の辞書ファイル (1 行 1 定義) を読む。LoadKawariDict が 7 モードのときに呼ぶ。
	bool LoadKawari7Dict(const wstring &filename);

	// 7 の辞書の 1 行を処理する (cs : 暗号化行を復号した後の文字コード)
	void LoadKawari7Line(const wstring &line, const wstring &filename, unsigned int lineno);

public:
	// 範囲外のインデックス
	static const unsigned int NPos;

	// 情報系API

	// 総単語数を取得
	// 戻り値 : 単語の個数
	unsigned int WordCollectionSize(void) const;

	// 有効エントリ数を取得
	// 戻り値 : エントリの個数
	unsigned int EntryCollectionSize(void) const;

	// 指定されたエントリの単語数を取得
	// 戻り値 : 単語の個数
	unsigned int EntrySize(const wstring& entryname) const;


	// エントリ名API

	// 文字列をエントリ名で使用可能な文字列にエンコードする
	static wstring EncodeEntryName(const wstring &orgsen);

	// 文字列をエントリ名とインデックスにデコードする
	// 例 : Entry        単独のエントリ名
	//      Entry[i]     エントリのi番目の要素
	//      Entry[i..j]  エントリのi番目からj番目までの要素
	//      インデックスが負の場合は、後ろから数えた位置
	// 戻り値 :  0=添え字無し
	//           1=単独要素指定
	//           2=範囲指定
	//          -1=エラー
//	static int DecodeEntryName(const string &orgsen,string &entryname,int &st,int &end);
	// 戻り値の範囲は必ず0以上になるが、エントリサイズ内とは限らない。
	TEntryRange GetEntryRange(const wstring &orgsen);

	// 辞書ID変換系API

	// 単語ID取得
	// 戻り値 : 1オリジン、見つからなければ0を返す
	TWordID GetWordID(const wstring& word) const;

	// IDから単語に変換
	// 戻り値 : 逆コンパイルした単語文字列
	wstring GetWordFromID(TWordID id) const;

	// エントリ取得
	// 戻り値 : 1オリジン、見つからなければ0を返す
	TEntry GetEntry(const wstring& entryname) const;


	// 辞書追加・削除系API

	// エントリを生成する
	// 既にエントリが存在する場合は、生成せずにIDを返す
	// 戻り値 : 生成したエントリのID
	TEntry CreateEntry(const wstring& entryname);

	// 単語を生成する
	// 既に単語が存在する場合は、生成せずにIDを返す
	// 戻り値 : 生成した単語のID
	TWordID CreateWord(const wstring& word);
	TWordID CreateStrWord(const wstring& word);

	// 指定されたエントリを空にする
	// メモリに空エントリと単語が残っても良い
	void ClearEntry(const wstring& entryname);

	// エントリ最後尾への単語の追加
	// 戻り値 : 成功でtrue
	void Push(const wstring& entryname,const wstring& word);

	// エントリ最後尾の単語の削除
	void Pop(const wstring& entryname);

	// エントリ途中への単語の挿入
	void Insert(const wstring& entryname,unsigned int pos,const wstring& word);

	// エントリ途中の単語の削除
	// endも削除されることに注意。
	void Erase(const wstring& entryname,unsigned int st,unsigned int end);

	// エントリ途中の単語の入れ替え
	void Replace(const wstring& entryname,unsigned int pos,const wstring& word);

	// エントリ途中の単語の入れ替え(インデックスが範囲外の場合、id2を追加)
	void Replace2(const wstring& entryname,unsigned int pos,const wstring& word,const wstring& word2);

	// 指定されたエントリを空にしてから単語を追加
	// エントリを変数的に利用する時に使用する
	void PushAfterClear(const wstring& entryname,const wstring& word);
	void PushStrAfterClear(const wstring& entryname,const wstring& word);

	// 指定されたエントリへの書き込みを禁止する
	void WriteProtect(const wstring& entryname);

	// 指定されたエントリ名から始まるエントリを全て空にする
	// メモリに空エントリと単語が残っても良い
	// "."で全エントリ・・・危険すぎ？
	void ClearTree(const wstring& spacename);


	// 検索系API

	// 指定されたエントリが空かどうか？
	// 戻り値 : 空ならtrue
	bool Empty(const wstring& entryname) const;

	// 指定されたエントリの指定した順番(0オリジン)の単語を返す
	// 戻り値 : 逆コンパイルした単語文字列
	wstring IndexWord(TEntry entry,unsigned int index=0) const;
	wstring IndexWord(const wstring& entryname,unsigned int index=0) const;

	// 指定されたエントリ内から指定した単語を検索
	// 戻り値 : インデックス(見つからなければNPos)
	unsigned int Find(const wstring& entryname,const wstring& word,unsigned int pos=0) const;

	// 指定されたエントリ内から指定した単語を検索(逆順)
	// 戻り値 : インデックス(見つからなければNPos)
	unsigned int RFind(const wstring& entryname,const wstring& word,unsigned int pos=NPos) const;

	// エントリ集合演算を行い、結果を列挙する
	// 戻り値 : 単語の個数
	unsigned int CalcEntryExpression(const wstring &entryexpr, set<TWordID> &wordcol) const;

	// エントリを全て列挙
	// 戻り値 : エントリの個数
	unsigned int FindAllEntry(vector<TEntry> &entrycol) const;


	// 実行系API

	// 指定されたエントリの指定した順番(0オリジン)の単語を実行する
	// 戻り値 string : 出力
	wstring IndexParse(TEntry entry,unsigned int index=0);
	wstring IndexParse(const wstring& entryname,unsigned int index=0);

	// 指定されたIDの単語(スクリプト)を実行する
	// 戻り値 string : 出力
	wstring Parse(TWordID id);

	// 与えられたスクリプトを解釈・実行する
	// 戻り値 string : 出力
	wstring Parse(const wstring& script);

	// KIS関数(ビルトイン及びユーザ定義)の実行
	wstring FunctionCall(const vector<wstring> &args);

	// ビルトイン関数の情報を得る
	// name : (入力)関数名
	// info : (出力)関数名、文法、戻り値、備考のリスト
	// 戻り値(bool) : 関数が存在すればtrue
	bool GetFunctionInfo(const std::wstring &name, struct TKisFunctionInfo &info);

	// ビルトイン関数のリスト
	// list : (出力)関数名リスト
	// 戻り値(unsigned int) : 関数の数
	unsigned int GetFunctionList(std::vector<std::wstring> &list) const;


	// SAORI関係API

	enum SAORILOADTYPE {
		PRELOAD=saori::PRELOAD,
		LOADONCALL,
		NORESIDENT
	};

	// SAORIモジュールの登録
	void RegisterSAORIModule(const std::wstring &aliasname, const std::wstring &path, const SAORILOADTYPE type);

	// SAORIモジュール登録の削除
	void EraseSAORIModule(const std::wstring &aliasname);

	// SAORIリクエストを行う
	bool RequestToSAORIModule(
		const std::wstring &aliasname,
		const TPHMessage &request, TPHMessage &response);

	// 登録されたSAORIモジュールのリストを得る
	int ListSAORIModule(std::vector<wstring> &list);

	// 指定SAORIモジュールの情報を得る


	// 本体との通信API (Direct SSTP)

	// 本体への接続を得る
	dsstp::TBasewareLink &GetBaseware(void) { return *Baseware; }


	// 安全モード API (kosui --check 用)

	// 安全モードにする。辞書を読み込む前に呼ぶこと。
	// 有効の間は、辞書の =kis や KIS が外へ影響を及ぼさない。
	//  - ファイルを書かない (save / textsave / logfile など。書いたことにして成功を返す)
	//  - SAORI の実体を作らない (DLL も exe も起動しない。呼び出しは Result: 0 で成功する)
	//  - 本体への通信 (Direct SSTP) をしない
	// ファイルを書き込めないのは、このプロセス全体に及ぶ (misc/wfile.h)。
	void SetSandbox(bool enable);
	bool IsSandbox(void) const { return Sandbox; }

	// 静的検査 API (kosui --check 用)

	// 辞書の =kis ゾーンの中間コードを、Lint() まで残す。辞書を読み込む前に呼ぶこと。
	void SetKeepZones(bool enable);

	// 読み込み済みの辞書と、残しておいたゾーンを静的に検査し、警告を出す (kawari_lint.h)。
	// 戻り値 : 出した警告の数
	unsigned int Lint(void);


	// コンテキスト関係 API

	// 履歴参照
	wstring GetHistory(int index);


	// ファイルAPI

	// 辞書ファイルのパス名を設定する
	void SetDataPath(const wstring &datapath);

	// 辞書ファイルのパス名を取得する
	wstring GetDataPath(void) const;

	// 華和梨フォーマット辞書ファイルを読み込む
	// BOM/内容からUTF-8かSJISかを自動判定する
	// 戻り値 : 成功でtrue
	bool LoadKawariDict(const wstring &filename);

	// 華和梨フォーマット辞書をwide文字ストリームから読み込む (BOMは除去済みであること)
	// filename : エラー表示用の名前
	// cryptcs : 暗号化行(!KAWA0000/!KAWA0001)の復号後の文字コード
	// 戻り値 : 成功でtrue
	bool LoadKawariDictStream(wistream &is, const wstring &filename, TKawariCharset cryptcs=KC_SJIS);

	// 華和梨フォーマット辞書ファイルを書き込む (UTF-8 BOMあり)
	// 戻り値 : 成功でtrue
	bool SaveKawariDict(const wstring &filename,const vector<wstring>& entry,bool crypt=false) const;


	// 華和梨7互換モード API (Phase 9.3.0)

	// 7 互換モードにする。辞書を読む前に呼ぶこと。ゴースト単位で、途中では切り替えない。
	void SetLegacy7(bool enable);
	bool IsLegacy7(void) const { return Legacy7; }

	// 7 の kawari.ini を読む (dict / include / text / adddict / set / saori / locale ... を実行し、
	// 辞書を読み込む)。7 モードでなければ何もしない。
	// 戻り値 : ini を開けたら true
	bool LoadKawari7Ini(const wstring &inifile);

	// 7 モードの状態 (7 モードでなければ NULL)
	TKawari7State *GetLegacy7State(void) { return Legacy7State; }

	// データフォルダと、最初に読むファイルの名前から、華和梨7の kawari.ini で動かすかを決める。
	//  ・rcfile の拡張子が .ini なら 7 (inipath = そのファイル)
	//  ・rcfile が既定の kawarirc.kis で、それが無く、kawari.ini があれば 7 (inipath = kawari.ini)
	// 戻り値 : 7 モードにするなら true
	static bool DetectLegacy7(const wstring &datapath, const wstring &rcfile, wstring &inipath);

	// 指定エントリから単語を一様に (重複も数えて) 選んで実行する。無ければ空。
	wstring RandomSelect(const wstring &entryname);

	// 7 のマッチエントリ (["単語", ...] : 応答) のうち、system.Sentence に合うものから応答を選ぶ。無ければ空。
	wstring MatchRandomSelect(void);

public:

	// ロガーの取得
	TKawariLogger &GetLogger(void) const;

	TKawariEngine(void);

	~TKawariEngine();

};
//---------------------------------------------------------------------------
// エントリの領域指定
// range == false : no index (Start=0, End=max(Size()-1, 0))
// start == end : single word
// start < end  : multiple words
// start == TKawariEngine::NPos  : error
struct TEntryRange {
	wstring Name;
	TEntry Entry;
	bool Range;
	unsigned int Start;
	unsigned int End;

	TEntryRange(const wstring &name, const TEntry &entry)
		:Name(name), Entry(entry), Range(false),
		 Start(0), End((entry.Size()>0)?entry.Size()-1:0) {}
	TEntryRange(const wstring &name, const TEntry &entry,
				unsigned int start, unsigned int end)
		:Name(name), Entry(entry), Range(true), Start(start), End(end) {}
	TEntryRange(const TEntryRange &range)
		:Name(range.Name), Entry(range.Entry),
		 Range(range.Range), Start(range.Start), End(range.End) {}
};
//---------------------------------------------------------------------------
// 総単語数を取得
inline unsigned int TKawariEngine::WordCollectionSize(void) const
{
	return(Dictionary->WordCollectionSize());
}
//---------------------------------------------------------------------------
// 有効エントリ数を取得
inline unsigned int TKawariEngine::EntryCollectionSize(void) const
{
	return(Dictionary->Size());
}
//---------------------------------------------------------------------------
// 指定されたエントリの単語数
inline unsigned int TKawariEngine::EntrySize(const wstring& entryname) const
{
	return(Dictionary->GetEntry(entryname).Size());
}
//---------------------------------------------------------------------------
// 単語ID取得
inline TKVMCode_base *TKawariEngine::CompileWordString(const wstring &word) const
{
	if(Legacy7) return TKawari7Compiler::CompileString(word);
	return TKawariCompiler::Compile(word, GetLogger());
}
//---------------------------------------------------------------------------
inline TWordID TKawariEngine::GetWordID(const wstring& word) const
{
	TKVMCode_base *code=CompileWordString(word);
	if(!code) return(0);
	TWordID id=Dictionary->GetWordID(code);
	delete code;

	return(id);
}
//---------------------------------------------------------------------------
// エントリ取得
inline TEntry TKawariEngine::GetEntry(const wstring& entryname) const
{
	return(Dictionary->GetEntry(entryname));
}
//---------------------------------------------------------------------------
// エントリを生成する
inline TEntry TKawariEngine::CreateEntry(const wstring& entryname)
{
	return(Dictionary->CreateEntry(entryname));
}
//---------------------------------------------------------------------------
// 単語を生成する
inline TWordID TKawariEngine::CreateWord(const wstring& word)
{
	return(Dictionary->CreateWord(CompileWordString(word)));
}
//---------------------------------------------------------------------------
inline TWordID TKawariEngine::CreateStrWord(const wstring& word)
{
	return(Dictionary->CreateWord(TKawariCompiler::CompileAsString(word)));
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にする
inline void TKawariEngine::ClearEntry(const wstring& entryname)
{
	Dictionary->GetEntry(entryname).Clear();
}
//---------------------------------------------------------------------------
// 辞書への単語の追加
inline void TKawariEngine::Push(const wstring& entryname,const wstring& word)
{
	TWordID id=CreateWord(word);
	// 7 は空の単語を追加しない
	if(Legacy7&&(id==0)) return;
	Dictionary->CreateEntry(entryname).Push(id);
}
//---------------------------------------------------------------------------
// エントリ最後尾の単語の削除
inline void TKawariEngine::Pop(const wstring& entryname)
{
	Dictionary->GetEntry(entryname).Pop();
}
//---------------------------------------------------------------------------
// エントリ途中への単語の挿入
inline void TKawariEngine::Insert(const wstring& entryname,unsigned int pos,const wstring& word)
{
	Dictionary->CreateEntry(entryname).Insert(pos,CreateWord(word));
}
//---------------------------------------------------------------------------
// エントリ途中の単語の削除
inline void TKawariEngine::Erase(const wstring& entryname,unsigned int st,unsigned int end)
{
	Dictionary->GetEntry(entryname).Erase(st, end);
}
//---------------------------------------------------------------------------
// エントリ途中の単語の入れ替え
inline void TKawariEngine::Replace(const wstring& entryname,unsigned int pos,const wstring& word)
{
	Dictionary->CreateEntry(entryname).Replace(pos,CreateWord(word));
}
//---------------------------------------------------------------------------
// エントリ途中の単語の入れ替え(インデックスが範囲外の場合、id2を追加)
inline void TKawariEngine::Replace2(const wstring& entryname,unsigned int pos,const wstring& word,const wstring& word2)
{
	Dictionary->CreateEntry(entryname).Replace2(pos,CreateWord(word),CreateWord(word2));
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にしてから単語を追加
inline void TKawariEngine::PushAfterClear(const wstring& entryname,const wstring& word)
{
	TWordID id=CreateWord(word);
	if(Legacy7&&(id==0)) {
		// 7 は空の単語なら、エントリを空にするだけ
		Dictionary->CreateEntry(entryname).Clear();
		return;
	}
	Dictionary->CreateEntry(entryname).PushAfterClear(id);
}
//---------------------------------------------------------------------------
inline void TKawariEngine::PushStrAfterClear(const wstring& entryname,const wstring& word)
{
	Dictionary->CreateEntry(entryname).PushAfterClear(CreateStrWord(word));
}
//---------------------------------------------------------------------------
// 指定されたエントリへの書き込みを禁止する
inline void TKawariEngine::WriteProtect(const wstring& entryname)
{
	Dictionary->CreateEntry(entryname).WriteProtect();
}
//---------------------------------------------------------------------------
// 指定されたエントリが空かどうか？
// 戻り値 : 空ならtrue
inline bool TKawariEngine::Empty(const wstring& entryname) const
{
	return(GetEntry(entryname).Size()==0);
}
//---------------------------------------------------------------------------
// 指定されたエントリの指定した順番(0オリジン)の単語を返す
inline wstring TKawariEngine::IndexWord(TEntry entry,unsigned int index) const
{
	TWordID id=entry.Index(index);
	if(id==0) return(L"");
	return(GetWordFromID(id));
}
//---------------------------------------------------------------------------
inline wstring TKawariEngine::IndexWord(const wstring& entryname,unsigned int index) const
{
	return(GetWordFromID(Dictionary->GetEntry(entryname).Index(index)));
}
//---------------------------------------------------------------------------
// 指定されたエントリ内から指定した単語を検索
// 戻り値 : インデックス(見つからなければNPos)
inline unsigned int TKawariEngine::Find(const wstring& entryname,const wstring& word,unsigned int pos) const
{
//	return(Find(GetEntryID(entryname),GetWordID(word),pos));
	return Dictionary->GetEntry(entryname).Find(GetWordID(word),pos);
}
//---------------------------------------------------------------------------
// 指定されたエントリ内から指定した単語を検索(逆順)
// 戻り値 : インデックス(見つからなければNPos)
inline unsigned int TKawariEngine::RFind(const wstring& entryname,const wstring& word,unsigned int pos) const
{
//	return(RFind(GetEntryID(entryname),GetWordID(word),pos));
	return Dictionary->GetEntry(entryname).RFind(GetWordID(word),pos);
}
//---------------------------------------------------------------------------
// エントリ名を全て列挙
inline unsigned int TKawariEngine::FindAllEntry(vector<TEntry> &entrycol) const
{
	return(Dictionary->FindAllEntry(entrycol));
}
//---------------------------------------------------------------------------
// 指定されたエントリの指定した順番(0オリジン)の単語を返す
inline wstring TKawariEngine::IndexParse(TEntry entry,unsigned int index)
{
	if(!entry.IsValid()) return(L"");
	return(Parse(entry.Index(index)));
}
//---------------------------------------------------------------------------
// 指定されたエントリの指定した順番(0オリジン)の単語を返す
inline wstring TKawariEngine::IndexParse(const wstring& entryname,unsigned int index)
{
	return(Parse(Dictionary->GetEntry(entryname).Index(index)));
}
//--------------------------------------------------------------------------
// KIS関数を実行する
inline wstring TKawariEngine::FunctionCall(const vector<wstring> &args){
	return KawariVM->FunctionCall(args);
}
//---------------------------------------------------------------------------
// ビルトイン関数の情報を得る
inline bool TKawariEngine::GetFunctionInfo(const wstring &name, struct TKisFunctionInfo &info){
	return KawariVM->GetFunctionInfo(name, info);
}
//---------------------------------------------------------------------------
// ビルトイン関数のリスト
inline unsigned int TKawariEngine::GetFunctionList(vector<wstring> &list) const{
	return KawariVM->GetFunctionList(list);
}
//--------------------------------------------------------------------------
// 履歴参照
inline std::wstring TKawariEngine::GetHistory(int index){
	return Dictionary->GetHistory(index);
}
//---------------------------------------------------------------------------
// 辞書ファイルのパス名を設定する
inline void TKawariEngine::SetDataPath(const wstring &datapath)
{
	DataPath=datapath;
}
//---------------------------------------------------------------------------
// 辞書ファイルのパス名を取得する
inline wstring TKawariEngine::GetDataPath(void) const
{
	return(DataPath);
}
//---------------------------------------------------------------------------
// ロガーの取得
inline TKawariLogger &TKawariEngine::GetLogger(void) const
{
	return (*logger);
}
//---------------------------------------------------------------------------
#endif
