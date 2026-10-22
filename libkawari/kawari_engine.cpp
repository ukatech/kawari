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
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
//---------------------------------------------------------------------------
#include <iostream>
#include <fstream>
using namespace std;
//---------------------------------------------------------------------------
#include "libkawari/kawari_crypt.h"
#include "libkawari/sequence_gen.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 文字列をエントリ名で使用可能な文字列にエンコードする
#if defined(STRING_APPEND_INATOMIC)
string TKawariEngine::EncodeEntryName(const string &orgsen) const
{
	// 2001-10-29 少しいじってみた...
	string entsen(orgsen);
	unsigned cntsen = entsen.size();
	
	for(unsigned int pos=0;pos<cntsen;pos++) {
		char	c = entsen[pos];
		if(iskanji1st(c)) {
			// 無いとは思うが漢字は受容
			if((pos+1)<cntsen) pos++;
			else break;
		} else if(!isalnum(c)) {
			// 使えない文字は全て'.'に変換
			entsen[pos] = '.';
		}
	}
	
	return(entsen);
}
#else
string TKawariEngine::EncodeEntryName(const string &orgsen) const
{
	string entsen;

	for(unsigned int pos=0;pos<orgsen.size();pos++) {
		if(iskanji1st(orgsen[pos])) {
			// 無いとは思うが漢字は受容
			if((pos+1)<orgsen.size()) {
				entsen+=orgsen[pos++];
				entsen+=orgsen[pos];
			} else {
				break;
			}
		} else if(isalnum(orgsen[pos])) {
			// イベント名部分には'.'以外の記号を使わない
			entsen+=orgsen[pos];
		} else {
			// 使えない文字は全て'.'に変換
			entsen+='.';
		}
	}

	return(entsen);
}
#endif
//---------------------------------------------------------------------------
// 指定されたエントリを空にする
void TKawariEngine::ClearEntry(const string& entryname)
{
	TEntryID entry=Dictionary->GetEntryID(entryname);
	if(entry==0) return;
	if(ProtectedEntry.count(entry)) return;

	Dictionary->ClearEntry(entry);

	return;
}
//---------------------------------------------------------------------------
// 辞書への単語の追加
void TKawariEngine::Insert(const string& entryname,const string& word)
{
	TEntryID entry=Dictionary->CreateEntry(entryname);
	if(ProtectedEntry.count(entry)) return;

	Dictionary->Insert(entry,word);

	return;
}
//---------------------------------------------------------------------------
// 指定されたエントリを空にしてから単語を追加
void TKawariEngine::InsertAfterClear(const string& entryname,const string& word)
{
	TEntryID entry=Dictionary->CreateEntry(entryname);
	if(ProtectedEntry.count(entry)) return;


	Dictionary->InsertAfterClear(entry,word);

	return;
}
//---------------------------------------------------------------------------
// 指定されたエントリへの書き込みを禁止する
void TKawariEngine::WriteProtect(const string& entryname)
{
	TEntryID entry=Dictionary->CreateEntry(entryname);

	ProtectedEntry.insert(entry);

	return;
}
//---------------------------------------------------------------------------
// 指定されたエントリの先頭の単語を返す
string TKawariEngine::FindFirst(const string& entryname) const
{
	TWordID id=Dictionary->FindFirst(Dictionary->GetEntryID(entryname));

	if(id==0) return("");

	TKawariCode_base *code=Dictionary->GetWordFromID(id);

	if(!code) return("");

	return(code->Run(KisEngine));
}
//---------------------------------------------------------------------------
// 指定されたエントリの単語をランダムに選択して返す
string TKawariEngine::RandomSelect(const string& entryname) const
{
	vector<TWordID> wordlist;

	Dictionary->FindAll(Dictionary->GetEntryID(entryname),wordlist);

	if(wordlist.size()==0) return("");

	TWordID id=wordlist[Random(wordlist.size())];
	TKawariCode_base *code=Dictionary->GetWordFromID(id);

	if(!code) return("");

	return(code->Run(KisEngine));
}
//---------------------------------------------------------------------------
// 発動条件のそろったマッチエントリの単語をランダムに選択して返す
string TKawariEngine::MatchEntryRandomSelect(const string& entryname) const
{
	vector<TEntryID> entrylist;

	if(MatchDictionary->Evaluation(entrylist)==0) return("");

	TEntryID entry=entrylist[Random(entrylist.size())];

	vector<TWordID> wordlist;

	Dictionary->FindAll(entry,wordlist);

	if(wordlist.size()==0) return("");

	TWordID id=wordlist[Random(wordlist.size())];
	TKawariCode_base *code=Dictionary->GetWordFromID(id);

	if(!code) return("");

	return(code->Run(KisEngine));
}
//---------------------------------------------------------------------------
// 華和梨フォーマット辞書ファイルを読み込む
bool TKawariEngine::LoadKawariDict(const string &filename)
{
	ifstream ifs;
	ifs.open(filename.c_str());

	if(!ifs.is_open()) return(false);

	string localid="LocalID"+SequenceGenerator.GetString("Local")+"@";

//	if(LogFS) (*LogFS) << "Load:" << (filename) << endl;
//	if(LogFS) (*LogFS) << "Load(id):" << (localid) << endl;

	string strbuff;
	while(getline(ifs,strbuff)) {
		strbuff=StringTrim(strbuff);
		if((strbuff.size()>0)&&(strbuff[0]=='!')) {
			if(CheckCrypt(strbuff)) {
				strbuff=StringTrim(DecryptString(strbuff));
			} else {
				strbuff="";
			}
		}

		if(strbuff.size()==0) continue;

		TKawariLexer buff(strbuff);

		if(buff[0]=='#') {
			// コメント
		} else if(buff[0]==':') {
			// プリプロセッサ
		} else if(buff[0]=='[') {
			// マッチエントリ
			buff.Pos=1;

			TKawariMatchFind *match=new TKawariMatchFind;

			try {
				match->SetEntryName("system.Sentence");

				while(!buff.IsEnd()) {
					string key=buff.GetWordString(",&]");

					if(key.size()) match->Add(key);

					buff.Pos=buff.AcceptSpace();

					if(buff.IsEnd()) throw("error");

					if(buff[buff.Pos]==']') {
						buff.Pos++;
						break;
					}

					if((buff[buff.Pos]!=',')&&(buff[buff.Pos]!='&')) throw("error");

					buff.Pos++;
				}

				buff.Pos=buff.AcceptSpace();
				if(buff.IsEnd()||(buff[buff.Pos]!=':')) throw("error");
				buff.Pos++;

				TEntryID entry=MatchDictionary->CreateMatchEntry(match);
				match=NULL;

				while(!buff.IsEnd()) {
					TKawariCode_base *code=ParseKawariDict(buff,localid);
					if(code) Dictionary->Insert(entry,code);
				}
			} catch(...) {
				if(match) delete match;
//				if(LogFS) (*LogFS) << "ERROR:Load(matchentry):" << ((string)buff) << endl;
			}
		} else {
			// 単語登録
			try {
				vector<string> keystr;
				buff.GetEntryList(keystr,',');

				if(keystr.size()) {
					if(buff.IsEnd()) throw("error");

					if(buff[buff.Pos]==':') {
						buff.Pos++;
#if 0
// 継続行やめ(要文法再検討)
					} else if(buff[buff.Pos]=='{') {
						buff.Pos++;
						buff.Pos=buff.AcceptSpace();
						// 「エントリ名 {」の行だった(「}」まで継続)
						string linebuff=buff.substr(buff.Pos);
						string strbuff;
						while(getline(ifs,strbuff)) {
							strbuff=StringTrim(strbuff);
							if((strbuff.size()>0)&&(strbuff[0]=='!')) {
								if(CheckCrypt(strbuff)) {
									strbuff=StringTrim(DecryptString(strbuff));
								} else {
									strbuff="";
								}
							}

							if((strbuff.size()==0)||(strbuff[0]=='#')||(strbuff[0]==':')) continue;
							if(strbuff[0]=='}') break;

							if(linebuff.size()>0) linebuff+=" ";
							linebuff+=strbuff;
						}

						buff=linebuff;
#endif
					} else throw("error");

					vector<TEntryID> key;
					for(unsigned int i=0;i<keystr.size();i++) {
						key.push_back(Dictionary->CreateEntry(keystr[i]));
					}

					while(!buff.IsEnd()) {
						TKawariCode_base *code=ParseKawariDict(buff,localid);

						if(code) {
							TWordID id=Dictionary->Insert(key[0],code);
							for(unsigned int i=1;i<key.size();i++) {
								Dictionary->Insert(key[i],id);
							}
    					}
	    			}
                }
			} catch(...) {
//				if(LogFS) (*LogFS) << "ERROR:Load(entry):" << ((string)buff) << endl;
			}
		}
	}
	ifs.close();

	return(true);
}
//---------------------------------------------------------------------------
// テキストファイル読み込み
bool TKawariEngine::LoadTextFile(const string &filename,const string &baseentry)
{
	ifstream ifs;
	ifs.open(filename.c_str());

	if(!ifs.is_open()) return(false);

	string buff;
	int line=0;
	int nline=0;
	while(getline(ifs,buff)) {
//		buff=StringTrim(buff);
		line++;
		string entry=baseentry+string(".")+IntToString(line);
		if(buff.size()>0) { // 空行ではなかったらエントリに追加
			InsertAfterClear(entry,buff);
		} else { // 空行をカウント
			nline++;
			ClearEntry(entry);
		}
	}

	if((line>0)&&(nline!=line)) { // 空のファイルではなかったら行数をセット
		InsertAfterClear(baseentry+string(".size"),IntToString(line));
	}
	ifs.close();

	return(true);
}
//---------------------------------------------------------------------------
// 華和梨フォーマットファイルの単語を解釈して取得
TKawariCode_base *TKawariEngine::ParseKawariDict(TKawariLexer &buff,const string& localid)
{
	buff.Pos=buff.AcceptSpace();

	vector<TKawariCode_base*> codelist;

	if(!TKawariCodeWord::Compile2(codelist,buff,',')) return(NULL);

	if(codelist.size()==1) return(codelist[0]);

	TKawariCodeWord *code=new TKawariCodeWord;
	code->Add(codelist);

	return(code);
}
//---------------------------------------------------------------------------
// 華和梨フォーマット辞書ファイルを書き込む
bool TKawariEngine::SaveKawariDict(const string &filename,const vector<string>& entry,bool crypt) const
{
	ofstream ofs;
	ofs.open(filename.c_str());

	if(!ofs.is_open()) return(false);

	ofs << "#" << endl << "# Kawari saved file" << endl << "#" << endl;

	for(unsigned int i=0;i<entry.size();i++) {
		ofs << "# Entry " << entry[i] << endl;

		vector<TWordID> wordcol;
		Dictionary->FindAll(Dictionary->GetEntryID(entry[i]),wordcol);
		if(wordcol.size()) {
			string line;
			line=entry[i]+" : "+Dictionary->GetWordFromID(wordcol[0])->DisCompile();

			for(unsigned int j=1;j<wordcol.size();j++) {
				line+=" , ";
				line+=Dictionary->GetWordFromID(wordcol[j])->DisCompile();
			}

			if(!crypt) {
				ofs << line << endl;
			} else {
				ofs << EncryptString(line) << endl;
			}
		}
	}

	ofs.close();

	return(true);
}
//---------------------------------------------------------------------------
// 与えられた単語を解釈する
string TKawariEngine::Parse(const string& word)
{
	TKawariCode_base *code=Dictionary->StringToCode(word);
    if (!code) return string("");
	code=code->CreateContext();
	string retstr=code->Run(KisEngine);
	delete code;
	return(retstr);
}
//---------------------------------------------------------------------------
// ログストリームを得る
ostream *TKawariEngine::GetLogStream(void){
	if (Kawari)
		return Kawari->LogFS;
	else
		return NULL;
}
//---------------------------------------------------------------------------
// SAORIモジュールの登録
void TKawariEngine::RegisterSAORIModule(const string &aliasname, const string &path, const SAORILOADTYPE type){
	SaoriPark->RegisterModule(aliasname, path, (TSaoriModule::LOADTYPE)type);
}
//---------------------------------------------------------------------------
// SAORIモジュール登録の削除
void TKawariEngine::EraseSAORIModule(const string &aliasname){
	SaoriPark->EraseModule(aliasname);
}
//---------------------------------------------------------------------------
// SAORIリクエストを行う
bool TKawariEngine::RequestToSAORIModule(
	const string &aliasname,
	const TPHMessage &request, TPHMessage &response)
{
	TSaoriModule *module=SaoriPark->GetModule(aliasname);
	if (!module)
		return false;
	else
		return module->Request(request, response);
}
//---------------------------------------------------------------------------
TKawariEngine::TKawariEngine(TNS_KawariANI *kawari)
{
	Dictionary=new TNS_KawariDictionary;
	KisEngine=new TKisEngine(Dictionary,this);
	MatchDictionary=new TNS_KawariMatchDictionary(Dictionary,this);
	SaoriPark=new TSaoriPark();
	Kawari=kawari;
}
//---------------------------------------------------------------------------
