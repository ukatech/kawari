//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 華和梨7互換モード : 7.5.0 の KIS コマンド
//
//  2026.11.16  Phase 9.3.0   華和梨 7.5.0 の kis_dict.cpp / kis_file.cpp / kis_expr.cpp /
//                            kis_test.cpp / kis_cpiro.cpp から移植
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <set>
//---------------------------------------------------------------------------
#include "kis/kis7.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari7.h"
#include "misc/misc.h"
#include "misc/wfile.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
// 7.5.0 の atoi 相当 (空白・符号・数字。数字以外で止まる)
static int Atoi7(const wstring &s)
{
	size_t i=0;
	while((i<s.size())&&((s[i]==L' ')||(s[i]==L'\t')||(s[i]==L'\n')||(s[i]==L'\r'))) i++;
	bool neg=false;
	if((i<s.size())&&((s[i]==L'+')||(s[i]==L'-'))) {
		neg=(s[i]==L'-');
		i++;
	}
	unsigned int v=0;
	while((i<s.size())&&(s[i]>=L'0')&&(s[i]<=L'9')) {
		v=v*10+static_cast<unsigned int>(s[i]-L'0');
		i++;
	}
	return neg?static_cast<int>(0u-v):static_cast<int>(v);
}
//---------------------------------------------------------------------------
// Atoi7 の 64bit 版 (expr / test の数値演算用。範囲外は 64bit で回り込む)
static kawari_int64 Atoi7_64(const wstring &s)
{
	size_t i=0;
	while((i<s.size())&&((s[i]==L' ')||(s[i]==L'\t')||(s[i]==L'\n')||(s[i]==L'\r'))) i++;
	bool neg=false;
	if((i<s.size())&&((s[i]==L'+')||(s[i]==L'-'))) {
		neg=(s[i]==L'-');
		i++;
	}
	kawari_uint64 v=0;
	while((i<s.size())&&(s[i]>=L'0')&&(s[i]<=L'9')) {
		v=v*10+static_cast<kawari_uint64>(s[i]-L'0');
		i++;
	}
	return neg?static_cast<kawari_int64>(static_cast<kawari_uint64>(0)-v):static_cast<kawari_int64>(v);
}
//---------------------------------------------------------------------------
static inline bool IsFalse7(const wstring &s)
{
	return (s.size()==0)||(s==L"0")||(s==L"false");
}
//---------------------------------------------------------------------------
// 相対パスを、ゴーストのフォルダからのパスにする
static wstring ResolvePath(TKawariEngine *engine, const wstring &path)
{
	if(path.size()&&IsAbsolutePath(path)) return path;
	return engine->GetDataPath()+path;
}
//---------------------------------------------------------------------------
// 7 のコマンド 1 つ。実体は関数 1 つ。
class TKis7Command : public TKisFunction_base {
public:
	typedef wstring (*Impl)(TKawariEngine *engine, const vector<wstring> &args);
private:
	Impl impl;
public:
	TKis7Command(const wchar_t *name, const wchar_t *format, const wchar_t *info, Impl i) : impl(i)
	{
		Name_=name;
		Format_=format;
		Returnval_=L"(Kawari 7.5.0)";
		Information_=info;
	}
	virtual bool Init(void) { return true; }
	virtual wstring Function(const vector<wstring> &args) { return impl(Engine, args); }
};
//---------------------------------------------------------------------------
// get エントリ 番号 : 番号番目の単語を、実行せずに逆コンパイルして返す。引数が足りなければ空
static wstring Kis7_get(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()!=3) return L"";
	TEntry entry=engine->GetEntry(args[1]);
	if(!entry.IsValid()) return L"";
	vector<TWordID> wordcol;
	entry.FindAll(wordcol);
	unsigned int i=static_cast<unsigned int>(Atoi7(args[2]));
	if(wordcol.size()>i) return engine->GetWordFromID(wordcol[i]);
	return L"";
}
//---------------------------------------------------------------------------
// array エントリ 番号 : 番号番目の単語を実行して返す
static wstring Kis7_array(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()!=3) return L"";
	TEntry entry=engine->GetEntry(args[1]);
	if(!entry.IsValid()) return L"";
	vector<TWordID> wordcol;
	entry.FindAll(wordcol);
	unsigned int i=static_cast<unsigned int>(Atoi7(args[2]));
	if(wordcol.size()>i) return engine->Parse(wordcol[i]);
	return L"";
}
//---------------------------------------------------------------------------
// enumerate エントリ : 全単語を逆コンパイルして空白で区切って返す
static wstring Kis7_enumerate(TKawariEngine *engine, const vector<wstring> &args)
{
	wstring retstr;
	if(args.size()>=2) {
		TEntry entry=engine->GetEntry(args[1]);
		if(entry.IsValid()) {
			vector<TWordID> wordcol;
			entry.FindAll(wordcol);
			for(size_t i=0; i<wordcol.size(); i++) {
				if(i) retstr+=L" ";
				retstr+=engine->GetWordFromID(wordcol[i]);
			}
		}
	}
	return retstr;
}
//---------------------------------------------------------------------------
// 行ごとに エントリ.N へ入れる (textload と split で共通)
//   空の行はエントリを空にする。全部が空でなければ エントリ.size に行数を入れる。
static void StoreLines7(TKawariEngine *engine, const wstring &base, const vector<wstring> &lines)
{
	int line=0;
	int nline=0;
	for(size_t i=0; i<lines.size(); i++) {
		line++;
		wstring entry=base+L"."+IntToString(line);
		if(lines[i].size()>0) {
			engine->PushAfterClear(entry, lines[i]);
		} else {
			nline++;
			engine->ClearEntry(entry);
		}
	}
	if((line>0)&&(nline!=line))
		engine->PushAfterClear(base+L".size", IntToString(line));
}
//---------------------------------------------------------------------------
// textload ファイル エントリ : テキストファイルを 1 行ずつ エントリ.1 エントリ.2 ... へ読む
static wstring Kis7_textload(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()!=3) return L"";

	wstring text;
	if(!ReadTextFile(ResolvePath(engine, args[1]), text)) return L"";

	// getline と同じ : 改行で区切り、最後の改行の後ろに空の行は作らない
	vector<wstring> lines;
	size_t st=0;
	while(st<text.size()) {
		size_t nl=text.find(L'\n', st);
		wstring one;
		if(nl==wstring::npos) {
			one=text.substr(st);
			st=text.size();
		} else {
			one=text.substr(st, nl-st);
			st=nl+1;
		}
		if(one.size()&&(one[one.size()-1]==L'\r')) one.erase(one.size()-1);
		lines.push_back(one);
	}
	StoreLines7(engine, args[2], lines);
	return L"";
}
//---------------------------------------------------------------------------
// readdir フォルダ エントリ : フォルダの中のファイル名・フォルダ名を エントリ に入れる ("." と ".." も入る)
static wstring Kis7_readdir(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()!=3) return L"";

	wstring dir=ResolvePath(engine, args[1]);
	bool isdir=false;
	if(!PathStat(dir, &isdir, NULL)||!isdir) return L"";

	vector<wstring> names;
	if(!ListDirectory(dir, names)) return L"";

	engine->ClearEntry(args[2]);
	bool hasdot=false, hasdotdot=false;
	for(size_t i=0; i<names.size(); i++) {
		if(names[i]==L".") hasdot=true;
		if(names[i]==L"..") hasdotdot=true;
	}
	if(!hasdot) engine->Push(args[2], L".");
	if(!hasdotdot) engine->Push(args[2], L"..");
	for(size_t i=0; i<names.size(); i++)
		engine->Push(args[2], names[i]);
	return L"";
}
//---------------------------------------------------------------------------
// split エントリ 文字列 区切り : 区切りで分けて エントリ.1 エントリ.2 ... へ入れる
static wstring Kis7_split(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()!=4) return L"";

	const wstring &str=args[2];
	const wstring &cs=args[3];
	vector<wstring> lines;

	size_t pos=0;
	while(pos<str.size()) {
		size_t idx=str.find(cs, pos);
		if(idx!=wstring::npos) {
			lines.push_back(str.substr(pos, idx-pos));
			pos=idx+cs.size();
			if(cs.size()==0) break;
		} else {
			lines.push_back(str.substr(pos));
			pos=str.size();
		}
	}
	StoreLines7(engine, args[1], lines);
	return L"";
}
//---------------------------------------------------------------------------
// pirocall #ラベル [既定値] : kawari.ini の外の [SAKURA] 形式で読んだ piro から選ぶ
static wstring Kis7_pirocall(TKawariEngine *engine, const vector<wstring> &args)
{
	if((args.size()!=2)&&(args.size()!=3)) return L"";

	wstring entry=L"piro."+TKawari7Compiler::EncodeEntryName(args[1]);
	wstring retstr=engine->RandomSelect(entry);
	if(retstr.size()) return retstr;
	if(args.size()==3) return args[2];
	return L"";
}
//---------------------------------------------------------------------------
// test の本体
static wstring TestFunction7(const vector<wstring> &args)
{
	bool status=false;

	switch(args.size()) {
	case 2:
		status=!IsFalse7(args[1]);
		break;
	case 3:
		if(args[1]==L"!") status=IsFalse7(args[2]);
		break;
	case 4:
		if((args[2]==L"==")||(args[2]==L"=")) {
			status=(args[1]==args[3]);
		} else if(args[2]==L"!=") {
			status=(args[1]!=args[3]);
		} else if(args[2]==L"<") {
			status=(args[1]<args[3]);
		} else if(args[2]==L">") {
			status=(args[1]>args[3]);
		} else if(args[2]==L"<=") {
			status=(args[1]<=args[3]);
		} else if(args[2]==L">=") {
			status=(args[1]>=args[3]);
		} else if(args[2]==L"-eq") {
			status=(Atoi7_64(args[1])==Atoi7_64(args[3]));
		} else if(args[2]==L"-ne") {
			status=(Atoi7_64(args[1])!=Atoi7_64(args[3]));
		} else if(args[2]==L"-lt") {
			status=(Atoi7_64(args[1])<Atoi7_64(args[3]));
		} else if(args[2]==L"-gt") {
			status=(Atoi7_64(args[1])>Atoi7_64(args[3]));
		} else if(args[2]==L"-le") {
			status=(Atoi7_64(args[1])<=Atoi7_64(args[3]));
		} else if(args[2]==L"-ge") {
			status=(Atoi7_64(args[1])>=Atoi7_64(args[3]));
		} else if(args[2]==L"&&") {
			status=(!IsFalse7(args[1]))&&(!IsFalse7(args[3]));
		} else if(args[2]==L"||") {
			status=(!IsFalse7(args[1]))||(!IsFalse7(args[3]));
		}
		break;
	default:
		break;
	}
	return status?L"true":L"false";
}
//---------------------------------------------------------------------------
static wstring Kis7_test(TKawariEngine *engine, const vector<wstring> &args)
{
	return TestFunction7(args);
}
//---------------------------------------------------------------------------
// [ 式 ] : 最後の ] は無視して test と同じ
static wstring Kis7_bracket(TKawariEngine *engine, const vector<wstring> &args)
{
	vector<wstring> args2(args);
	if(args2.back()==L"]") args2.pop_back();
	return TestFunction7(args2);
}
//---------------------------------------------------------------------------
// expr : 7.5.0 の演算 (引数が 1 トークンずつの GNU expr 風)
//---------------------------------------------------------------------------
namespace {

struct TExprError {};

// 7.5.0 の is_integer : 先頭の 1 文字しか見ない (先頭が数字か - ならよい。空もよい)
bool IsInteger7(const wstring &x)
{
	if(x.size()==0) return true;
	return ((x[0]>=L'0')&&(x[0]<=L'9'))||(x[0]==L'-');
}

class TExpr7 {
	const vector<wstring> &Args;
	size_t Index;

	bool testNext(const wchar_t *s) const { return (Index<Args.size())&&(Args[Index]==s); }

	// 比較 (両方整数なら数値、でなければ文字列)
	static bool Compare(const wstring &l, const wstring &r, int op)
	{
		if(IsInteger7(l)&&IsInteger7(r)) {
			kawari_int64 il=Atoi7_64(l), ir=Atoi7_64(r);
			switch(op) {
			case 0: return il<ir;
			case 1: return il<=ir;
			case 2: return il==ir;
			case 3: return il!=ir;
			case 4: return il>=ir;
			default: return il>ir;
			}
		}
		switch(op) {
		case 0: return l<r;
		case 1: return l<=r;
		case 2: return l==r;
		case 3: return l!=r;
		case 4: return l>=r;
		default: return l>r;
		}
	}

	static wstring Arith(const wstring &l, const wstring &r, wchar_t op)
	{
		if(!IsInteger7(l)||!IsInteger7(r)) throw TExprError();
		// 符号付きのオーバーフローは未定義動作なので、+ - * と -1 での除算は unsigned で計算する
		kawari_int64 ia=Atoi7_64(l);
		kawari_int64 ib=Atoi7_64(r);
		kawari_uint64 a=static_cast<kawari_uint64>(ia);
		kawari_uint64 b=static_cast<kawari_uint64>(ib);
		switch(op) {
		case L'+': return Int64ToString(static_cast<kawari_int64>(a+b));
		case L'-': return Int64ToString(static_cast<kawari_int64>(a-b));
		case L'*': return Int64ToString(static_cast<kawari_int64>(a*b));
		default:
			if(ib==0) throw TExprError();
			if(ib==-1) {
				// 最小値 / -1 は例外で落ちる。割り算は符号反転、余りは 0
				if(op==L'/') return Int64ToString(static_cast<kawari_int64>(static_cast<kawari_uint64>(0)-a));
				return Int64ToString(0);
			}
			if(op==L'/') return Int64ToString(ia/ib);
			return Int64ToString(ia%ib);
		}
	}

	// ( 式 ) と、そのままの引数
	wstring Eval7(void)
	{
		if(Index>=Args.size()) throw TExprError();
		if(testNext(L"(")) {
			Index++;
			wstring val=Eval();
			if(!testNext(L")")) throw TExprError();
			Index++;
			return val;
		} else if(testNext(L")")) {
			throw TExprError();
		}
		return Args[Index++];
	}

	// quote / length / find / findpos / index / substr
	wstring Eval6(void)
	{
		if(testNext(L"quote")) {
			Index++;
			if(Index>=Args.size()) throw TExprError();
			return Args[Index++];
		} else if(testNext(L"length")) {
			Index++;
			return IntToString(static_cast<int>(Eval6().size()));
		} else if(testNext(L"find")) {
			Index++;
			wstring l=Eval6();
			wstring r=Eval6();
			return (l.find(r)!=wstring::npos)?r:wstring(L"");
		} else if(testNext(L"findpos")) {
			Index++;
			wstring l=Eval6();
			wstring r=Eval6();
			size_t idx=l.find(r);
			if(idx!=wstring::npos) return IntToString(static_cast<int>(idx+1));
			return L"";
		} else if(testNext(L"index")) {
			Index++;
			wstring str=Eval6();
			wstring cs=Eval7();
			size_t idx=wstring::npos;
			for(size_t i=0; i<cs.size(); i++) {
				size_t pos=str.find(cs[i]);
				if(pos!=wstring::npos) idx=(pos<idx)?pos:idx;
			}
			if(idx!=wstring::npos) return IntToString(static_cast<int>(idx+1));
			return L"";
		} else if(testNext(L"substr")) {
			Index++;
			wstring l=Eval6();
			wstring si1=Eval6();
			wstring si2=Eval6();
			if(!IsInteger7(si1)||!IsInteger7(si2)) return L"";
			int i1=Atoi7(si1);
			int i2=Atoi7(si2);
			if((i1>static_cast<int>(l.size()))||(i1<=0)||(i2<=0)) return L"";
			return l.substr(static_cast<size_t>(i1-1), static_cast<size_t>(i2));
		}
		return Eval7();
	}

	// * / %
	wstring Eval4(void)
	{
		wstring l=Eval6();
		while(true) {
			if(testNext(L"*")) {
				Index++;
				wstring r=Eval4();
				l=Arith(l, r, L'*');
			} else if(testNext(L"/")) {
				Index++;
				wstring r=Eval4();
				l=Arith(l, r, L'/');
			} else if(testNext(L"%")) {
				Index++;
				wstring r=Eval4();
				l=Arith(l, r, L'%');
			} else {
				return l;
			}
		}
	}

	// + -
	wstring Eval3(void)
	{
		wstring l=Eval4();
		while(true) {
			if(testNext(L"+")) {
				Index++;
				wstring r=Eval4();
				l=Arith(l, r, L'+');
			} else if(testNext(L"-")) {
				Index++;
				wstring r=Eval4();
				l=Arith(l, r, L'-');
			} else {
				return l;
			}
		}
	}

	// 比較
	wstring Eval2(void)
	{
		wstring l=Eval3();
		while(true) {
			int op=-1;
			if(testNext(L"<")) op=0;
			else if(testNext(L"<=")) op=1;
			else if(testNext(L"=")||testNext(L"==")) op=2;
			else if(testNext(L"!=")) op=3;
			else if(testNext(L">=")) op=4;
			else if(testNext(L">")) op=5;
			if(op<0) return l;
			Index++;
			wstring r=Eval3();
			l=Compare(l, r, op)?wstring(L"1"):wstring(L"");
		}
	}

	// &
	wstring Eval1(void)
	{
		wstring l=Eval2();
		while(true) {
			if(testNext(L"&")) {
				Index++;
				wstring r=Eval2();
				if(IsFalse7(l)||IsFalse7(r)) l=L"";
			} else {
				return l;
			}
		}
	}

	// |
	wstring Eval(void)
	{
		wstring l=Eval1();
		while(true) {
			if(testNext(L"|")) {
				Index++;
				wstring r=Eval1();
				if(IsFalse7(l)) l=r;
			} else {
				return l;
			}
		}
	}

public:
	TExpr7(const vector<wstring> &a) : Args(a), Index(0) {}

	wstring Run(void)
	{
		try {
			wstring ret=Eval();
			if(Index<Args.size()) return L"";
			return ret;
		} catch(TExprError &) {
			return L"";
		}
	}
};

} // namespace
//---------------------------------------------------------------------------
static wstring Kis7_expr(TKawariEngine *engine, const vector<wstring> &args)
{
	if(args.size()<2) return L"";
	vector<wstring> operands(args.begin()+1, args.end());
	TExpr7 expr(operands);
	return expr.Run();
}
//---------------------------------------------------------------------------
void CreateKis7Commands(vector<TKisFunction_base *> &out, TKawariEngine *engine)
{
	TKis7Command *cmds[]={
		new TKis7Command(L"get", L"get entry index", L"word source (not evaluated)", Kis7_get),
		new TKis7Command(L"array", L"array entry index", L"evaluate the index-th word", Kis7_array),
		new TKis7Command(L"enumerate", L"enumerate entry", L"all words, source form", Kis7_enumerate),
		new TKis7Command(L"textload", L"textload file entry", L"load lines into entry.N", Kis7_textload),
		new TKis7Command(L"readdir", L"readdir dir entry", L"list directory into entry", Kis7_readdir),
		new TKis7Command(L"split", L"split entry string delimiter", L"split into entry.N", Kis7_split),
		new TKis7Command(L"pirocall", L"pirocall #label [default]", L"call a piro word", Kis7_pirocall),
		new TKis7Command(L"test", L"test expression", L"true / false", Kis7_test),
		new TKis7Command(L"[", L"[ expression ]", L"true / false", Kis7_bracket),
		new TKis7Command(L"expr", L"expr expression", L"expression (GNU expr like)", Kis7_expr)
	};
	for(size_t i=0; i<sizeof(cmds)/sizeof(cmds[0]); i++) {
		cmds[i]->NotifyEngine(engine);
		out.push_back(cmds[i]);
	}
}
//---------------------------------------------------------------------------
