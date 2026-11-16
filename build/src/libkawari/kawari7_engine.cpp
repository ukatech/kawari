//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨7互換モード : kawari.ini と 7 の辞書ファイルの読み込み
//
//  2026.11.16  Phase 9.3.0   華和梨 7.5.0 の kawari.cpp (LoadINI / LoadSub / LoadFile)、
//                            kawari_engine.cpp (LoadKawariDict / MatchEntryRandomSelect) から移植
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari7.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_crypt.h"
#include "misc/misc.h"
#include "misc/wfile.h"
#include "misc/charset.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// バイト列の両端の空白 (半角スペース・タブ・改行) を取り除く
static string TrimBytes(const string &s)
{
	size_t st=0;
	size_t en=s.size();
	while((st<en)&&((s[st]==' ')||(s[st]=='\t')||(s[st]=='\r')||(s[st]=='\n'))) st++;
	while((en>st)&&((s[en-1]==' ')||(s[en-1]=='\t')||(s[en-1]=='\r')||(s[en-1]=='\n'))) en--;
	return s.substr(st, en-st);
}
//---------------------------------------------------------------------------
// バイト列を行に分ける (改行は \n。行末の \r は取る)
static void SplitLines(const string &raw, vector<string> &lines)
{
	size_t st=0;
	while(st<raw.size()) {
		size_t nl=raw.find('\n', st);
		string one;
		if(nl==string::npos) {
			one=raw.substr(st);
			st=raw.size();
		} else {
			one=raw.substr(st, nl-st);
			st=nl+1;
		}
		if(one.size()&&(one[one.size()-1]=='\r')) one.erase(one.size()-1);
		lines.push_back(one);
	}
}
//---------------------------------------------------------------------------
// ファイルを読み、文字コードを決めて、行に分ける
//   cs : 辞書の文字コード (ini の locale があればそれ、なければ BOM / 内容から判定)
static bool ReadLines7(const wstring &filename, TKawariCharset locale, vector<string> &lines, TKawariCharset &cs)
{
	string raw;
	if(!ReadWholeFile(filename, raw)) return false;

	bool bom=HasUTF8BOM(raw);
	if(bom) raw.erase(0, 3);
	if(locale!=KC_NULL) cs=locale;
	else cs=bom?KC_UTF8:DetectCharset(raw);

	SplitLines(raw, lines);
	return true;
}
//---------------------------------------------------------------------------
// 7 の kawari.ini で動かすかを決める
bool TKawariEngine::DetectLegacy7(const wstring &datapath, const wstring &rcfile, wstring &inipath)
{
	wstring full=(rcfile.size()&&IsAbsolutePath(rcfile))?rcfile:(datapath+rcfile);

	if(rcfile.size()>=4) {
		wstring ext=rcfile.substr(rcfile.size()-4);
		for(size_t i=0; i<ext.size(); i++)
			if((ext[i]>=L'A')&&(ext[i]<=L'Z')) ext[i]=static_cast<wchar_t>(ext[i]-L'A'+L'a');
		if(ext==L".ini") {
			inipath=full;
			return true;
		}
	}

	if(rcfile==L"kawarirc.kis") {
		bool isfile=false;
		if(!PathStat(full, NULL, &isfile)||!isfile) {
			wstring ini=datapath+L"kawari.ini";
			if(PathStat(ini, NULL, &isfile)&&isfile) {
				inipath=ini;
				return true;
			}
		}
	}
	return false;
}
//---------------------------------------------------------------------------
// kawari.ini を読んでいる途中の、ファイルごとの状態 (include の入れ子)
struct TIniFrame7 {
	vector<string> lines;
	TKawariCharset cs;
	size_t li;
	TIniFrame7() : cs(KC_NULL), li(0) {}
};
//---------------------------------------------------------------------------
// 華和梨7互換モードにする
void TKawariEngine::SetLegacy7(bool enable)
{
	Legacy7=enable;
	Dictionary->SetFlatEntryNames(enable);
	if(enable) {
		if(!Legacy7State) Legacy7State=new TKawari7State();
		KawariVM->EnableLegacy7();
	}
}
//---------------------------------------------------------------------------
// 指定エントリから単語を一様に (重複も数えて) 選んで実行する
wstring TKawariEngine::RandomSelect(const wstring &entryname)
{
	TEntry entry=Dictionary->GetEntry(entryname);
	if(!entry.IsValid()) return L"";

	vector<TWordID> wordlist;
	entry.FindAll(wordlist);
	if(wordlist.size()==0) return L"";

	TWordID id=wordlist[Random(static_cast<int>(wordlist.size()))];
	return Parse(id);
}
//---------------------------------------------------------------------------
// マッチエントリのうち system.Sentence に合うものから応答を選ぶ
wstring TKawariEngine::MatchRandomSelect(void)
{
	if(!Legacy7State) return L"";

	wstring sentence=IndexParse(L"system.Sentence");

	vector<wstring> candidates;
	for(size_t i=0; i<Legacy7State->matches.size(); i++) {
		const TKawari7Match &m=Legacy7State->matches[i];
		bool all=true;
		for(size_t j=0; j<m.keywords.size(); j++) {
			if(sentence.find(m.keywords[j])==wstring::npos) {
				all=false;
				break;
			}
		}
		if(all) candidates.push_back(m.entryname);
	}
	if(candidates.size()==0) return L"";

	return RandomSelect(candidates[static_cast<size_t>(Random(static_cast<int>(candidates.size())))]);
}
//---------------------------------------------------------------------------
// 7 の辞書の 1 行を処理する
//   #...                  コメント
//   :...                  プリプロセッサ (無視)
//   ["単語", ...] : 応答   マッチエントリ
//   エントリ, ... : 単語, 単語 ...
void TKawariEngine::LoadKawari7Line(const wstring &line, const wstring &filename, unsigned int lineno)
{
	TKawari7Lexer buff(line, logger);
	wchar_t head=buff.At(0);

	if(head==L'#') {
		// コメント
	} else if(head==L':') {
		// プリプロセッサ
	} else if(head==L'[') {
		// マッチエントリ
		buff.pos=1;
		vector<wstring> keywords;
		bool ok=true;

		while(!buff.IsEnd()) {
			wstring key=buff.GetWordString(L",&]");
			if(key.size()) keywords.push_back(key);

			buff.pos=buff.AcceptSpace();
			if(buff.IsEnd()) {
				ok=false;
				break;
			}
			if(buff.At(buff.pos)==L']') {
				buff.pos++;
				break;
			}
			if((buff.At(buff.pos)!=L',')&&(buff.At(buff.pos)!=L'&')) {
				ok=false;
				break;
			}
			buff.pos++;
		}
		if(ok) {
			buff.pos=buff.AcceptSpace();
			if(buff.IsEnd()||(buff.At(buff.pos)!=L':')) ok=false;
		}
		if(!ok) return;
		buff.pos++;

		// 同じ条件のマッチエントリは 1 つにまとめる
		wstring entryname;
		for(size_t i=0; i<Legacy7State->matches.size(); i++) {
			if(Legacy7State->matches[i].keywords==keywords) {
				entryname=Legacy7State->matches[i].entryname;
				break;
			}
		}
		if(entryname.size()==0) {
			entryname=L"Kawari7.match."+IntToString(static_cast<int>(Legacy7State->matches.size()));
			TKawari7Match m;
			m.keywords=keywords;
			m.entryname=entryname;
			Legacy7State->matches.push_back(m);
		}

		TEntry entry=Dictionary->CreateEntry(entryname);
		while(!buff.IsEnd()) {
			TKVMCode_base *code=TKawari7Compiler::ParseDictWord(buff);
			if(code) {
				TWordID id=Dictionary->CreateWord(code);
				if(id&&entry.IsValid()) entry.Push(id);
			}
		}
	} else {
		// 単語の登録
		vector<wstring> keystr;
		buff.GetEntryList(keystr, L',');
		if(keystr.size()==0) return;
		if(buff.IsEnd()||(buff.At(buff.pos)!=L':')) return;
		buff.pos++;

		vector<TEntry> keys;
		for(size_t i=0; i<keystr.size(); i++) {
			TEntry entry=Dictionary->CreateEntry(keystr[i]);
			if(entry.IsValid()) keys.push_back(entry);
		}
		if(keys.size()==0) return;

		while(!buff.IsEnd()) {
			TKVMCode_base *code=TKawari7Compiler::ParseDictWord(buff);
			if(code) {
				TWordID id=Dictionary->CreateWord(code);
				if(id) {
					for(size_t i=0; i<keys.size(); i++)
						keys[i].Push(id);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
// 7 の辞書ファイルを読む
//   ・暗号化行 (!KAWA0000...) は復号して 1 行として扱う
//   ・1 行目が [SAKURA] ならピロ形式 (piro.<ラベル> エントリに入れる)
//   ・=kis 〜 =end は 9 の KIS ゾーンとして 9 の文法で実行する (ゾーンの中は最後まで 9)
bool TKawariEngine::LoadKawari7Dict(const wstring &filename)
{
	TKawariCharset cs=KC_NULL;
	vector<string> lines;
	TKawariCharset locale=Legacy7State?Legacy7State->localecs:KC_NULL;
	if(!ReadLines7(filename, locale, lines, cs)) return false;

	if((lines.size()>0)&&(TrimBytes(lines[0])=="[SAKURA]")) {
		// ピロ形式
		wstring key;
		bool countflag=false;
		wstring countstring;
		for(size_t i=0; i<lines.size(); i++) {
			wstring buff=MBtoW(TrimBytes(lines[i]), cs);
			if(buff.size()==0) continue;
			if(buff[0]==L'#') {
				if(!countflag) {
					key=buff;
				} else {
					if(buff.substr(0, buff.size()-1)==countstring) {
						key=countstring;
					} else {
						key=buff;
						countflag=false;
					}
				}
			} else if(key.size()>0) {
				if(buff.substr(0, 5)==L"count") {
					countflag=true;
					countstring=key;
				} else {
					Push(L"piro."+TKawari7Compiler::EncodeEntryName(key), buff);
					key=L"";
				}
			}
		}
		return true;
	}

	for(size_t i=0; i<lines.size(); i++) {
		string bl=TrimBytes(lines[i]);
		if(bl.size()&&(bl[0]=='!')) {
			if(CheckCrypt(bl)) bl=TrimBytes(DecryptString(bl));
			else bl="";
		}
		if(bl.size()==0) continue;

		wstring wl=MBtoW(bl, cs);
		if(wl==L"=kis") {
			// =end までを 9 の辞書として読む
			wstring zone=L"=kis\n";
			size_t j=i+1;
			for(; j<lines.size(); j++) {
				wstring zl=MBtoW(lines[j], cs);
				zone+=zl+L"\n";
				if(StringTrim(zl)==L"=end") break;
			}
			i=j;
			wistringstream wis(zone);
			LoadKawariDictStream(wis, filename, (cs==KC_UTF8)?KC_UTF8:KC_SJIS);
			continue;
		}
		LoadKawari7Line(wl, filename, static_cast<unsigned int>(i+1));
	}
	return true;
}
//---------------------------------------------------------------------------
// kawari.ini を読み、辞書を読み込み、system.OnLoad を呼ぶ (7.5.0 の LoadSub)
bool TKawariEngine::LoadKawari7Ini(const wstring &inifile)
{
	if(!Legacy7) return false;

	// 乱数の種 (ini の randomseed で上書きされる)
	SRandom(static_cast<int>(time(NULL)));

	vector<wstring> dictfiles;

	// ini を読む。include は、その位置で読み込んでから続きを読む (7.5.0 は再帰で読んだ)
	vector<TIniFrame7> frames;
	{
		TIniFrame7 first;
		if(!ReadLines7(inifile, KC_NULL, first.lines, first.cs)) return false;
		frames.push_back(first);
	}
	bool opened=true;
	while(frames.size()) {
		if(frames.back().li>=frames.back().lines.size()) {
			frames.pop_back();
			continue;
		}
		const string rawline=frames.back().lines[frames.back().li];
		TKawariCharset cs=frames.back().cs;
		frames.back().li++;

		{
			wstring buff=StringTrim(MBtoW(rawline, cs));
			if((buff.size()==0)||(buff[0]==L'#')) continue;

			wstring key, value;
			wstring::size_type pos=buff.find(L':');
			if(pos==wstring::npos) {
				key=buff;
			} else {
				key=StringTrim(buff.substr(0, pos));
				value=StringTrim(buff.substr(pos+1));
			}

			if(key==L"security") {
				Legacy7State->securitylevel=static_cast<unsigned int>(StringToInt(value));
			} else if(key==L"randomseed") {
				SRandom(StringToInt(value));
			} else if(key==L"dict") {
				dictfiles.push_back(value);
			} else if(key==L"text") {
				wstring::size_type sp=value.find_first_of(L" \t");
				if(sp!=wstring::npos) {
					wstring entry=StringTrim(value.substr(0, sp));
					wstring file=StringTrim(value.substr(sp+1));
					vector<wstring> args;
					args.push_back(L"textload");
					args.push_back(file);
					args.push_back(entry);
					KawariVM->FunctionCall7(args);
				}
			} else if(key==L"adddict") {
				wstring::size_type sp=value.find_first_of(L" \t");
				if(sp!=wstring::npos) {
					Push(StringTrim(value.substr(0, sp)), StringTrim(value.substr(sp+1)));
				}
			} else if(key==L"set") {
				wstring::size_type sp=value.find_first_of(L" \t");
				if(sp!=wstring::npos) {
					PushAfterClear(StringTrim(value.substr(0, sp)), StringTrim(value.substr(sp+1)));
				}
			} else if(key==L"include") {
				// 入れ子は 16 段まで (7.5.0 は互いに include すると終わらなかった)
				if(frames.size()<16) {
					TIniFrame7 sub;
					if(ReadLines7(DataPath+value, KC_NULL, sub.lines, sub.cs))
						frames.push_back(sub);
				}
			} else if(key==L"locale") {
				TKawariCharset lcs=CharsetFromNameExt(value);
				Legacy7State->locale=value;
				Legacy7State->localecs=lcs;
			} else if(key==L"debug") {
				if(value==L"log") {
					// kawari.log に、エラー・警告と、OnMouseMove 以外の要求と応答を記録する
					// (記録の書式は 9 のもの。7.5.0 のログとは同じではない)
					vector<wstring> args;
					args.push_back(L"logfile");
					args.push_back(L"kawari.log");
					KawariVM->FunctionCall(args);
					args.clear();
					args.push_back(L"loglevel");
					args.push_back(L"error");
					args.push_back(L"warning");
					args.push_back(L"info");
					args.push_back(L"baseevents");
					args.push_back(L"rscevents");
					args.push_back(L"timeevents");
					KawariVM->FunctionCall(args);
				} else if(value==L"debugger") {
					Push(L"event.ShioriEcho", L"${system.Reference0}\\e");
					Push(L"event.KawariCommand", L"$(echo \"$(\"${system.Reference0}\")\" )\\e");
				}
			} else if(key==L"saori") {
				vector<wstring> args;
				wstring::size_type curp=0;
				while(true) {
					wstring::size_type nextp=value.find(L',', curp);
					wstring param;
					if(nextp==wstring::npos) param=value.substr(curp);
					else param=value.substr(curp, nextp-curp);
					args.push_back(StringTrim(param));
					if(nextp==wstring::npos) break;
					curp=nextp+1;
				}
				if((args.size()==2)||(args.size()==3)) {
					SAORILOADTYPE loadopt=LOADONCALL;
					if(args.size()>=3) {
						if(args[2]==L"preload") loadopt=PRELOAD;
						else if(args[2]==L"noresident") loadopt=NORESIDENT;
					}
					RegisterSAORIModule(args[1], GetDataPath()+args[0], loadopt);
				}
			}
		}
	}

	// 辞書の読み込み
	for(size_t i=0; i<dictfiles.size(); i++)
		LoadKawari7Dict(DataPath+dictfiles[i]);

	// 読み込み完了のイベント
	RandomSelect(L"system.OnLoad");

	// セキュリティレベルが低いときは、homeurl の変更を禁止する
	if(Legacy7State->securitylevel<3) WriteProtect(L"resource.homeurl");

	return opened;
}
//---------------------------------------------------------------------------
