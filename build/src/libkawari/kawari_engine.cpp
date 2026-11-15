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
//  2001.06.10  Phase 5.3.1   イベント名で使える記号を「_」から「.」に変更
//  2001.06.17  Phase 5.4     複数エントリへの同時追加のバグ修正
//                            save
//  2001.08.06  Phase 6.2     継続行に対応
//  2001.08.07  Phase 6.2     ostrstreamバグ修正
//                            継続行やめ(要文法再検討)
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
//  2004.08.28  Phase 8.2.1   DecodeEntryNameが中括弧検索に失敗する問題対処
//  2005.06.28  Phase 8.2.3   DecodeEntryNameに代えてGetEntryRange導入
//  2026.10.18  Phase 9.0.0   wchar_t化 (辞書ファイルはBOM/内容で文字コード判定、保存はUTF-8 BOMあり)
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_compiler.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_crypt.h"
#include "libkawari/kawari_codeset.h"
#include "libkawari/kawari_rc.h"
#include "misc/misc.h"
#include "misc/wfile.h"
#include "saori/saori.h"
using namespace kawari::resource;
//---------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
// 範囲外のインデックス
const unsigned int TKawariEngine::NPos=TNS_KawariDictionary::NPos;
//---------------------------------------------------------------------------
TKawariEngine::TKawariEngine(void)
{
	logger=new TKawariLogger();
	Dictionary=new TNS_KawariDictionary((*logger));
	KawariVM=new TKawariVM(*this, *Dictionary, (*logger));
	Baseware=new dsstp::TBasewareLink();
	SaoriPark=new saori::TSaoriPark((*logger), Baseware);
	Sandbox=false;
	KeepZones=false;
}
//---------------------------------------------------------------------------
// 静的検査のために =kis ゾーンの中間コードを残すか
void TKawariEngine::SetKeepZones(bool enable)
{
	KeepZones=enable;
}
//---------------------------------------------------------------------------
// 静的検査 (kosui --check)
unsigned int TKawariEngine::Lint(void)
{
	return TKawariLint::Check(*Dictionary, *KawariVM, *logger, Zones, &LintNotes);
}
//---------------------------------------------------------------------------
// 安全モードの切り替え
void TKawariEngine::SetSandbox(bool enable)
{
	Sandbox=enable;
	SaoriPark->SetSandbox(enable);
	Baseware->SetAllowed(!enable);
	SetFileWritesDisabled(enable);
}
//---------------------------------------------------------------------------
TKawariEngine::~TKawariEngine()
{
	if (Sandbox) SetFileWritesDisabled(false);
	for (vector<TKawariLint::Zone>::iterator it=Zones.begin(); it!=Zones.end(); ++it)
		delete const_cast<TKVMCode_base *>(it->code);
	delete SaoriPark;
	delete Baseware;
	delete KawariVM;
	delete Dictionary;
	delete logger;
}
//---------------------------------------------------------------------------
wstring TKawariEngine::EncodeEntryName(const wstring &orgsen){
	return TKawariCompiler::EncodeEntryName(orgsen);
}
//---------------------------------------------------------------------------
// 文字列をエントリ名とインデックスにデコードする
#if 0
int TKawariEngine::DecodeEntryName(const wstring &orgsen,wstring &entryname,int &st,int &end)
{
	entryname=orgsen;
	st=0; end=-1;

	wstring::size_type pos3=orgsen.size()-1;
	wstring::size_type pos1=orgsen.rfind(L'[');
	if((orgsen.size()==0)||(orgsen[pos3]!=L']')||(pos1==wstring::npos))
		// インデックスが無かった
		return 0;

	wstring::size_type pos2=orgsen.find(L"..",pos1);
	if(pos2!=wstring::npos) {
		// 範囲指定
		wstring s_st=orgsen.substr(pos1+1,pos2-(pos1+1));
		wstring s_end=orgsen.substr(pos2+2,pos3-(pos2+2));

		if ((s_st.find_first_not_of(L"0123456789- ")==wstring::npos)&&
			(s_end.find_first_not_of(L"0123456789- ")==wstring::npos)){

			entryname=orgsen.substr(0,pos1);
			st=StringToInt(s_st);
			end=StringToInt(s_end);
			return 2;
		}

		// 何かの間違いだった
		return 0;
	}

	wstring s_idx=orgsen.substr(pos1+1,pos3-(pos1+1));
	if (s_idx.find_first_not_of(L"0123456789- ")!=wstring::npos)
		// おかしいですよカテジナさん
		return 0;

	// 単独要素指定
	entryname=orgsen.substr(0, pos1);
	st=end=StringToInt(s_idx);
	return 1;
}
#endif
//---------------------------------------------------------------------------
TEntryRange TKawariEngine::GetEntryRange(const wstring &orgsen)
{
	wstring::size_type idx_last=orgsen.size()-1;
	wstring::size_type idx_obrk=orgsen.rfind(L'[');
	if((orgsen.size()==0)||(orgsen[idx_last]!=L']')||(idx_obrk==wstring::npos)){
		// インデックス無し
		TEntry entry=CreateEntry(orgsen);
		return TEntryRange(orgsen, entry);
	}

	wstring::size_type idx_range=orgsen.find(L"..",idx_obrk);
	if(idx_range!=wstring::npos) {
		// 領域指定
		wstring s_st=StringTrim(orgsen.substr(idx_obrk+1,idx_range-(idx_obrk+1)));
		wstring s_end=StringTrim(orgsen.substr(idx_range+2,idx_last-(idx_range+2)));
		wstring entryname=orgsen.substr(0,idx_obrk);
		TEntry entry=CreateEntry(entryname);

		// 数字のみを受け付ける
		if(IsInteger(s_st)&&IsInteger(s_end)){
			int st=StringToInt(s_st);
			int end=StringToInt(s_end);
			if(st<0) st+=entry.Size();
			if(end<0) end+=entry.Size();
			if((st>=0)&&(end>=0)&&(st<=end))
				return TEntryRange(entryname, entry,
								   static_cast<unsigned int>(st),
								   static_cast<unsigned int>(end));
		}
		// error
		return TEntryRange(entryname, entry, NPos, NPos);

	}else{
		// 単独要素指定
		wstring s_idx=StringTrim(orgsen.substr(idx_obrk+1,idx_last-(idx_obrk+1)));
		wstring entryname=orgsen.substr(0, idx_obrk);
		TEntry entry=CreateEntry(entryname);

		if (IsInteger(s_idx)){
			int idx=StringToInt(s_idx);
			if(idx<0) idx+=entry.Size();
			if(idx>=0)
				return TEntryRange(entryname, entry,
								   static_cast<unsigned int>(idx),
								   static_cast<unsigned int>(idx));
		}
		// error
		return TEntryRange(entryname, entry, NPos, NPos);
	}
}
//---------------------------------------------------------------------------
// IDから単語に変換
wstring TKawariEngine::GetWordFromID(TWordID id) const
{
	if(!id) return(L"");
	TKVMCode_base *code=Dictionary->GetWordFromID(id);
	if(!code) return(L"");

	return(code->DisCompile());
}
//---------------------------------------------------------------------------
// 指定されたエントリ名から始まるエントリを全て空にする
// メモリに空エントリと単語が残っても良い
void TKawariEngine::ClearTree(const wstring& spacename)
{
	if(spacename!=L".") {
		TEntry space=CreateEntry(spacename);
		space.ClearTree();
	}else{
		vector<TEntry> entrycol;
		Dictionary->FindAllEntry(entrycol);
		for(vector<TEntry>::iterator it=entrycol.begin(); it!=entrycol.end(); it++)
			it->Clear();
	}
}
//---------------------------------------------------------------------------
// エントリ集合演算を行い、結果を列挙する
// 戻り値 : 単語の個数
unsigned int TKawariEngine::CalcEntryExpression(const wstring &entryexpr, set<TWordID> &wordcol) const{
	TKVMSetCode_base *code=TKawariCompiler::CompileAsEntryExpression(entryexpr, (*logger));
	if (!code) return 0;
	code->Evaluate(*KawariVM, wordcol);
	delete code;
	return static_cast<unsigned int>(wordcol.size());
}
//---------------------------------------------------------------------------
// 指定されたIDの単語(スクリプト)を実行する
wstring TKawariEngine::Parse(TWordID id)
{
	if(id==0) return(L"");

	TKVMCode_base *code=Dictionary->GetWordFromID(id);
	if(!code) return(L"");

	// どのエントリの単語かは分からない
	TKawariVM::TWordFrame frame(*KawariVM, TEntry(NULL, 0), id);
	return(KawariVM->RunWithNewContext(code));
}
//---------------------------------------------------------------------------
// 与えられたスクリプトを解釈・実行する
wstring TKawariEngine::Parse(const wstring& script)
{
	TKVMCode_base *code=TKawariCompiler::Compile(script, GetLogger());
	if(!code) return(L"");

	// デバッグ
	if(logger->Check(kawari_log::LOG_DUMP))
		code->Debug(logger->GetStream());

	wstring retstr=KawariVM->RunWithNewContext(code);
	delete code;

	return(retstr);
}

//---------------------------------------------------------------------------
// 華和梨フォーマット辞書ファイルを読み込む
// 文字コードはBOM/内容から自動判定する (暗号化行はBOMがあればUTF-8、なければSJISとして復号)
bool TKawariEngine::LoadKawariDict(const wstring &filename)
{
	// Debug Assertion避け
	if(!filename.size()) return(false);

	string raw;
	if(!ReadWholeFile(filename, raw)) return(false);

	bool bom=HasUTF8BOM(raw);
	TKawariCharset cs=bom ? KC_UTF8 : DetectCharset(raw);
	if(bom) raw.erase(0, 3);

	wistringstream wis(MBtoW(raw, cs));
	return LoadKawariDictStream(wis, filename, bom ? KC_UTF8 : KC_SJIS);
}
//---------------------------------------------------------------------------
// 華和梨フォーマット辞書をwide文字ストリームから読み込む
// cryptcs : 暗号化行を復号した後の文字コード
bool TKawariEngine::LoadKawariDictStream(wistream &ifs, const wstring &filename, TKawariCharset cryptcs)
{
	// 辞書中の記述モードの管理
	vector<TKawariCompiler::Mode> mode;
	mode.push_back(TKawariCompiler::M_DICT);

	TKawariCompiler compiler(ifs, (*logger), filename, true, cryptcs);
	if (KeepZones) compiler.SetLintNotes(&LintNotes);

	// 診断用 : 単語の定義位置を覚える
	TSourceMap &srcmap=Dictionary->SourceMap();
	const unsigned int fileidx=srcmap.FileIndex(filename);

	while(true) {
		if(mode.back()==TKawariCompiler::M_DICT){
			// 一定義ずつ辞書読み込み
			while(true){
				vector<wstring> entrynames;
				vector<TKVMCode_base *> sentence_list;
				vector<unsigned int> sentence_lines;
				if (!compiler.LoadEntryDefinition(entrynames, sentence_list, &sentence_lines))
					break;

				vector <TEntry> entrylist;
				for(vector<wstring>::iterator it=entrynames.begin(); it!=entrynames.end(); it++) {
					TEntry entry=Dictionary->CreateEntry(*it);
					if(entry.IsValid())
						entrylist.push_back(entry);
				}

				unsigned int sentence_index=0;
				for (vector<TKVMCode_base *>::iterator it=sentence_list.begin(); it!=sentence_list.end(); it++, sentence_index++){
					TWordID wid=Dictionary->CreateWord(*it);
					unsigned int sentence_line=(sentence_index<sentence_lines.size()) ? sentence_lines[sentence_index] : 0;
					if (wid)
						for (vector<TEntry>::iterator eit=entrylist.begin(); eit!=entrylist.end(); eit++){
							eit->Push(wid);
							srcmap.RecordWord(eit->GetID(), wid, fileidx, sentence_line);
						}
				}
			}

		}else if(mode.back()==TKawariCompiler::M_KIS){
			// KIS読み込み
			TKVMCode_base *code=compiler.LoadInlineScript();

			wstring outstr;
			{
				// 実行中の文の行は TKVMCodeInlineScript が更新する
				TKawariVM::TZoneFrame zoneframe(*KawariVM, fileidx, 0);
				outstr=KawariVM->RunWithNewContext(code);
			}
			if (outstr.size())
				logger->GetStream(kawari_log::LOG_INFO) << L"KIS zone output:" << outstr << endl;
			if (KeepZones && code) {
				// 静的検査のために、読み込みが終わるまで残しておく
				TKawariLint::Zone zone;
				zone.file=fileidx;
				zone.code=code;
				Zones.push_back(zone);
			} else {
				delete code;
			}
		}else{
			// ??
		}
		TKawariCompiler::Mode m=compiler.GetNextMode();
		if (m==TKawariCompiler::M_DICT){
			// 辞書モード
			mode.push_back(TKawariCompiler::M_DICT);
		}else if (m==TKawariCompiler::M_KIS){
			// スクリプトモード
			mode.push_back(TKawariCompiler::M_KIS);
		}else if (m==TKawariCompiler::M_END){
			// モード終了
			if(mode.size()>1) mode.pop_back();
		}else if (m==TKawariCompiler::M_UNKNOWN){
			// 知らないモード
			logger->Diag(kawari_log::LOG_ERROR, kawari_log::E_ENGINE_UNKNOWN_MODE).At(filename, compiler.GetLineNo())
				<< RC.S(ERR_ENGINE_UNKNOWN_MODE);
		}else if (m==TKawariCompiler::M_EOF){
			break;
		}
	}

	return(true);
}

//---------------------------------------------------------------------------
// 華和梨フォーマット辞書ファイルを書き込む
// UTF-8 (BOMあり) で書き出す。暗号化する場合は、UTF-8バイト列を暗号化する。
bool TKawariEngine::SaveKawariDict(const wstring &filename,const vector<wstring>& entry,bool crypt) const
{
#ifdef KAWARI_MS
	const char *eol="\r\n";
#else
	const char *eol="\n";
#endif
	string out("\xef\xbb\xbf");

	out+=string("#")+eol+"# Kawari saved file"+eol+"#"+eol;

	for(vector<wstring>::const_iterator it=entry.begin(); it!=entry.end(); it++) {
		out+="# Entry "+WtoUTF8(*it)+eol;

		vector<TWordID> wordcol;
		TEntry entry=GetEntry(*it);
		if (entry.IsValid())
			entry.FindAll(wordcol);
		if(wordcol.size()) {
			wstring line;
			line=(*it)+L" : "+GetWordFromID(wordcol[0]);

			unsigned int wordcolmax=static_cast<unsigned int>(wordcol.size());
			for(unsigned int j=1;j<wordcolmax;j++) {
				line+=L" , ";
				line+=GetWordFromID(wordcol[j]);
			}

			if(!crypt) {
				out+=WtoUTF8(line)+eol;
			} else {
				out+=EncryptString(WtoUTF8(line))+eol;
			}
		}
	}

	return WriteWholeFile(filename, out);
}
//---------------------------------------------------------------------------
// SAORIモジュールの登録
void TKawariEngine::RegisterSAORIModule(const wstring &aliasname, const wstring &path, const SAORILOADTYPE type){
	SaoriPark->RegisterModule(aliasname, path, (saori::LOADTYPE)type);
}
//---------------------------------------------------------------------------
// SAORIモジュール登録の削除
void TKawariEngine::EraseSAORIModule(const wstring &aliasname){
	SaoriPark->EraseModule(aliasname);
}
//---------------------------------------------------------------------------
// SAORIリクエストを行う
bool TKawariEngine::RequestToSAORIModule(
	const wstring &aliasname,
	const TPHMessage &request, TPHMessage &response)
{
	if (SaoriPark->IsSandboxStub(aliasname)){
		// 安全モード : 実体は無いので、何もせずに成功した(Result: 0)ことにする
		response.SetStartline(L"SAORI/1.0 200 OK");
		response[L"Result"]=L"0";
		return true;
	}
	saori::TBind *bind=SaoriPark->GetModule(aliasname);
	if (!bind)
		return false;
	else
		return bind->Request(request, response);
}
//---------------------------------------------------------------------------
// 登録されたSAORIモジュールのリストを得る
int TKawariEngine::ListSAORIModule(vector<wstring> &list)
{
	return SaoriPark->ListModule(list);
}
//---------------------------------------------------------------------------
