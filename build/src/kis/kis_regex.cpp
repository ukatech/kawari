//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 正規表現 --
//
//      uka_regexp (build/src/uka_regexp, git submodule) を使う。
//      構文はPCRE2寄り。\d \w \s \b はASCIIのみ。詳しくは uka_regexp/README.md。
//
//  2026.10.18  Phase 9.0.0   First version
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <cwchar>
//---------------------------------------------------------------------------
#include "kis/kis_regex.h"
#include "uka_regexp/uka_regexp.h"
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
using namespace std;
using namespace kawari_log;
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
// 英語のメッセージ(char*)をwstringにする
wstring AsciiToW(const char *s)
{
	wstring r;
	for(;*s;s++) r+=static_cast<wchar_t>(static_cast<unsigned char>(*s));
	return r;
}
//---------------------------------------------------------------------------
// 省略された引数の代わりに使う空文字列
const wstring &Arg(const vector<wstring>& args, vector<wstring>::size_type n)
{
	static const wstring empty;
	return (n<args.size())?args[n]:empty;
}
//---------------------------------------------------------------------------
// パターンの中の(?&name)\g<name>で、パターンに定義の無いサブルーチン名を
// 辞書の${name}の展開結果に解決する。展開の中でまた正規表現を使うと
// 入れ子になるので、無限に再帰しないよう深さを制限する。
// 深さはエンジンが持つ (TKawariEngine::RegexResolveDepthRef)。
const int RESOLVE_MAX_DEPTH=16;

class TResolveDepthGuard {
public:
	TResolveDepthGuard(int &d) : depth(d) { depth++; }
	~TResolveDepthGuard() { depth--; }
private:
	int &depth;
};
//---------------------------------------------------------------------------
// コンパイル済みの正規表現と、エラーのログ出力
class TRegex {
public:
	TRegex(TKawariEngine *e, TKawariLogger &l, const wstring &cmd)
		: engine(e), logger(l), command(cmd) {}

	// パターンをコンパイルする。
	// options: i=大文字小文字を区別しない s=.が改行にも一致 m=^と$が行頭・行末に一致
	//          x=パターン中の空白とコメントを無視 (空白とカンマは読み飛ばす)
	// オプションが不正ならログを出してfalse。パターンが不正なら例外。
	bool Compile(const wstring &pattern, const wstring &options, bool global);

	CRegexpW &Get(void) { return re; }

	// パターン自身の括弧の数。
	// サブルーチンとして取り込まれた文字列のグループは、その後ろに番号が振られるので
	// 結果には入れない。
	int GroupCount(void) const;

	// 例外をログに出す
	void Report(const CRegexpException &ex);

	// 全体(0)と各グループの一致を並べる。一致しなかったグループは空文字列
	void CollectGroups(const CRegexpContext &ctx, const wstring &str, vector<wstring> &out) const;

private:
	static bool Resolve(const wchar_t *name, CRegexpString &pattern, void *user);

	CRegexpW re;
	TKawariEngine *engine;
	TKawariLogger &logger;
	wstring command;
	vector<wstring> resolved;	// 辞書から解決したサブルーチン名
};
//---------------------------------------------------------------------------
bool TRegex::Compile(const wstring &pattern, const wstring &options, bool global)
{
	unsigned int flags=global?CRegexpW::GLOBAL:0;
	for(wstring::const_iterator it=options.begin();it!=options.end();++it){
		switch(*it){
		case L'i': case L'I': flags|=CRegexpW::IGNORECASE; break;
		case L's': case L'S': flags|=CRegexpW::SINGLELINE; break;
		case L'm': case L'M': flags|=CRegexpW::MULTILINE; break;
		case L'x': case L'X': flags|=CRegexpW::EXTENDED; break;
		case L' ': case L',': break;
		default:
			logger.Diag(LOG_ERROR, kawari_log::E_KIS_REGEX_OPTION) << L"KIS[" << command << L"] unknown option '"
				<< *it << L"' (i, s, m, x are available).";
			return false;
		}
	}
	resolved.clear();
	re.Compile(pattern.c_str(), flags, Resolve, this);
	return true;
}
//---------------------------------------------------------------------------
bool TRegex::Resolve(const wchar_t *name, CRegexpString &pattern, void *user)
{
	TRegex *self=static_cast<TRegex *>(user);

	// 定義の無い名前は、エンジンが構文エラー(名前入り)にする
	if(self->engine->EntrySize(name)==0) return false;
	if(self->engine->RegexResolveDepthRef()>=RESOLVE_MAX_DEPTH){
		self->logger.Diag(LOG_ERROR, kawari_log::E_KIS_REGEX_NESTED) << L"KIS[" << self->command
			<< L"] the subroutine '" << name << L"' is nested too deeply.";
		return false;
	}

	self->resolved.push_back(name);
	wstring text;
	{
		TResolveDepthGuard guard(self->engine->RegexResolveDepthRef());
		text=self->engine->Parse(wstring(L"${")+name+L"}");
	}
	pattern.append(text.c_str(), static_cast<int>(text.size()));
	return true;
}
//---------------------------------------------------------------------------
int TRegex::GroupCount(void) const
{
	// 取り込まれたグループは、パターン自身のグループの後ろに、解決した順に振られる。
	// 最初に取り込まれたものの番号の手前までが、パターン自身のグループ。
	int own=re.GetGroupCount();
	for(vector<wstring>::const_iterator it=resolved.begin();it!=resolved.end();++it){
		int n=re.GetGroupNumber(it->c_str());
		if((n>0)&&(n-1<own)) own=n-1;
	}
	return own;
}
//---------------------------------------------------------------------------
void TRegex::Report(const CRegexpException &ex)
{
	if(!logger.Check(LOG_ERROR)) return;

	// 出力は ds が破棄されるときにまとめて行われる
	TDiagStream ds=logger.Diag(LOG_ERROR, kawari_log::E_KIS_REGEX);
	ds << L"KIS[" << command << L"] ";
	switch(ex.GetCode()){
	case CRegexpException::ERR_SYNTAX:
		ds << L"bad regular expression (" << AsciiToW(ex.what()) << L")";
		if(ex.GetPosition()>=0)
			ds << L" at " << ex.GetPosition() << L" of the pattern";
		break;
	case CRegexpException::ERR_STEP_LIMIT:
		ds << L"the match took too many steps and was stopped";
		break;
	default:
		ds << L"regular expression error (" << AsciiToW(ex.what()) << L")";
		break;
	}
	ds << L".";
}
//---------------------------------------------------------------------------
void TRegex::CollectGroups(const CRegexpContext &ctx, const wstring &str, vector<wstring> &out) const
{
	int last=GroupCount();
	if(last>ctx.GetGroupCount()) last=ctx.GetGroupCount();
	for(int n=0;n<=last;n++){
		int s=ctx.GetStart(n);
		int e=ctx.GetEnd(n);
		out.push_back(((s>=0)&&(e>=s))?str.substr(s, e-s):wstring());
	}
}
//---------------------------------------------------------------------------
// エントリの中身を置き換える。エントリ名が空なら何もしない
void StoreEntry(TKawariEngine *engine, const wstring &name, const vector<wstring> &values)
{
	if(name.empty()) return;

	TEntry entry=engine->CreateEntry(name);
	if(!entry.IsValid()) return;
	entry.Clear();
	for(vector<wstring>::const_iterator it=values.begin();it!=values.end();++it)
		entry.Push(engine->CreateStrWord(*it));
}
//---------------------------------------------------------------------------
// 置換。times<0なら全部、そうでなければその回数まで。
// 戻り値: 置換後の文字列(一致しなければ元の文字列)。エラーなら空文字列
wstring ReplaceImpl(TKawariEngine *engine, TKawariLogger &logger, const wstring &cmd,
					const wstring &str, const wstring &pattern, const wstring &repl,
					const wstring &options, int times)
{
	TRegex rx(engine, logger, cmd);
	try{
		if(!rx.Compile(pattern, options, false)) return L"";
		CRegexpString out;
		int n=rx.Get().Replace(str.c_str(), static_cast<int>(str.size()), repl.c_str(), out, times);
		if(n==0) return str;
		return wstring(out.c_str(), out.size());
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return L"";
	}
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
// regex_match string pattern [options [entry]]
// stringのどこかにpatternが一致すれば"1"、しなければ"0"。
// entryには、一致した部分と、括弧のグループが順に入る(一致しなければ空になる)。
wstring KIS_regex_match::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	TRegex rx(Engine, GetLogger(), args[0]);
	try{
		if(!rx.Compile(args[2], Arg(args, 3), false)) return (L"");

		const wstring &str=args[1];
		CRegexpContext ctx;
		bool matched=rx.Get().Match(str.c_str(), static_cast<int>(str.size()), ctx);

		vector<wstring> groups;
		if(matched) rx.CollectGroups(ctx, str, groups);
		StoreEntry(Engine, Arg(args, 4), groups);
		return matched?L"1":L"0";
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return (L"");
	}
}
//---------------------------------------------------------------------------
// regex_find string pattern [options [index [entry]]]
// index(省略すると0。負の値は末尾から数える)以降で最初に一致した位置。
// 一致しなければ、またはindexが範囲外ならば-1。entryはregex_matchと同じ。
wstring KIS_regex_find::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	TRegex rx(Engine, GetLogger(), args[0]);
	try{
		if(!rx.Compile(args[2], Arg(args, 3), false)) return (L"");

		const wstring &str=args[1];
		int len=static_cast<int>(str.size());
		int start=0;
		if(!Arg(args, 4).empty()){
			start=StringToInt(args[4]);
			if(start<0) start+=len;
		}

		vector<wstring> groups;
		wstring ret=L"-1";
		if((start>=0)&&(start<=len)){
			CRegexpContext ctx;
			if(rx.Get().Match(str.c_str(), len, ctx, start)){
				rx.CollectGroups(ctx, str, groups);
				ret=IntToString(ctx.GetStart(0));
			}
		}
		StoreEntry(Engine, Arg(args, 5), groups);
		return ret;
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return (L"");
	}
}
//---------------------------------------------------------------------------
// regex_findall entry string pattern [options]
// 一致した部分すべてをentryに入れ、その個数を返す。
wstring KIS_regex_findall::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 4)) return (L"");

	TRegex rx(Engine, GetLogger(), args[0]);
	try{
		if(!rx.Compile(args[3], Arg(args, 4), true)) return (L"");

		const wstring &str=args[2];
		CRegexpContext ctx;
		vector<wstring> found;
		while(rx.Get().Match(str.c_str(), static_cast<int>(str.size()), ctx))
			found.push_back(str.substr(ctx.GetStart(0), ctx.GetEnd(0)-ctx.GetStart(0)));

		StoreEntry(Engine, args[1], found);
		return IntToString(static_cast<int>(found.size()));
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return (L"");
	}
}
//---------------------------------------------------------------------------
// regex_count string pattern [options]
wstring KIS_regex_count::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	TRegex rx(Engine, GetLogger(), args[0]);
	try{
		if(!rx.Compile(args[2], Arg(args, 3), true)) return (L"");

		const wstring &str=args[1];
		CRegexpContext ctx;
		int count=0;
		while(rx.Get().Match(str.c_str(), static_cast<int>(str.size()), ctx)) count++;
		return IntToString(count);
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return (L"");
	}
}
//---------------------------------------------------------------------------
// regex_replace string pattern replace [options]
// replaceでは $1 ${name} $& $` $' $$ が使える。
wstring KIS_regex_replace::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 4)) return (L"");

	return ReplaceImpl(Engine, GetLogger(), args[0], args[1], args[2], args[3], Arg(args, 4), -1);
}
//---------------------------------------------------------------------------
// regex_replace_first string pattern replace [options]
wstring KIS_regex_replace_first::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 4)) return (L"");

	return ReplaceImpl(Engine, GetLogger(), args[0], args[1], args[2], args[3], Arg(args, 4), 1);
}
//---------------------------------------------------------------------------
// regex_erase string pattern [options]
wstring KIS_regex_erase::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	return ReplaceImpl(Engine, GetLogger(), args[0], args[1], args[2], L"", Arg(args, 3), -1);
}
//---------------------------------------------------------------------------
// regex_erase_first string pattern [options]
wstring KIS_regex_erase_first::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 3)) return (L"");

	return ReplaceImpl(Engine, GetLogger(), args[0], args[1], args[2], L"", Arg(args, 3), 1);
}
//---------------------------------------------------------------------------
// regex_split entry string pattern [options [max]]
// patternに一致した部分で区切り、各要素をentryに入れて個数を返す。
// maxが正ならば、最大でmax個の要素にする(残りは最後の要素に入る)。
// 先頭と末尾の空の一致では区切らない。
wstring KIS_regex_split::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 4)) return (L"");

	TRegex rx(Engine, GetLogger(), args[0]);
	try{
		if(!rx.Compile(args[3], Arg(args, 4), true)) return (L"");

		const wstring &str=args[2];
		int len=static_cast<int>(str.size());
		int maxwords=Arg(args, 5).empty()?0:StringToInt(args[5]);

		CRegexpContext ctx;
		vector<wstring> pieces;
		int last=0;
		while(rx.Get().Match(str.c_str(), len, ctx)){
			int s=ctx.GetStart(0);
			int e=ctx.GetEnd(0);
			if((s==e)&&((s==0)||(s==len))) continue;
			if((maxwords>0)&&(static_cast<int>(pieces.size())>=maxwords-1)) break;
			pieces.push_back(str.substr(last, s-last));
			last=e;
		}
		pieces.push_back(str.substr(last));

		StoreEntry(Engine, args[1], pieces);
		return IntToString(static_cast<int>(pieces.size()));
	}catch(const CRegexpException &ex){
		rx.Report(ex);
		return (L"");
	}
}
//---------------------------------------------------------------------------
// regex_escape string
// 正規表現の特殊文字を\でエスケープする(文字列をそのままパターンに埋め込むため)。
wstring KIS_regex_escape::Function(const vector<wstring>& args)
{
	if(!AssertArgument(args, 2)) return (L"");

	static const wchar_t special[]=L"\\^$.|?*+()[]{}# ";
	wstring ret;
	for(wstring::const_iterator it=args[1].begin();it!=args[1].end();++it){
		if(wcschr(special, *it)!=NULL) ret+=L'\\';
		ret+=*it;
	}
	return ret;
}
//---------------------------------------------------------------------------
