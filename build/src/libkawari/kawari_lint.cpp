//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 静的検査 (kosui --check の Lint)
//
//  2026.10.23  Phase 9.2.0   作成
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_lint.h"
#include "libkawari/kawari_dict.h"
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_code.h"
#include "libkawari/kawari_codeset.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_rc.h"
#include "kis/kis_base.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
#include <algorithm>
using namespace std;
using namespace kawari::resource;
//---------------------------------------------------------------------------
TKawariLint::TKawariLint(TNS_KawariDictionary &d, TKawariVM &v, TKawariLogger &l)
	: dict(d), vm(v), logger(l), collecting(true)
{
}
//---------------------------------------------------------------------------
unsigned int TKawariLint::Check(TNS_KawariDictionary &d, TKawariVM &v, TKawariLogger &l,
								const vector<Zone> &zones, const TLintNotes *notes)
{
	TKawariLint lint(d, v, l);
	return lint.Run(zones, notes);
}
//---------------------------------------------------------------------------
// 位置の分かるものをファイル・行の順に、分からないものは最後に
bool TKawariLint::FindingLess(const Finding &a, const Finding &b)
{
	if ((a.file==0)!=(b.file==0)) return (a.file!=0);
	if (a.file!=b.file) return (a.file<b.file);
	if (a.line!=b.line) return (a.line<b.line);
	return (a.seq<b.seq);
}
//---------------------------------------------------------------------------
// 1 周目で定義を集め、2 周目で検査する。
// 単語は内容が同じなら共有されるので、同じ単語 ID の木は 1 回しか歩かない。
unsigned int TKawariLint::Run(const vector<Zone> &zones, const TLintNotes *notes)
{
	// 歩く場所の一覧 (辞書の単語 + ゾーン)
	vector<Root> roots;

	TSourceMap &srcmap=dict.SourceMap();

	vector<TEntry> entries;
	dict.FindAllEntry(entries);
	set<TWordID> seen;
	for (vector<TEntry>::const_iterator eit=entries.begin(); eit!=entries.end(); ++eit) {
		vector<TWordID> words;
		eit->FindAll(words);
		for (vector<TWordID>::const_iterator wit=words.begin(); wit!=words.end(); ++wit) {
			if (!seen.insert(*wit).second) continue;
			const TKVMCode_base *code=dict.GetWordFromID(*wit);
			if (!code) continue;
			Root r;
			r.code=code;
			r.entryname=eit->GetName();
			TSourceMap::Loc loc;
			if (srcmap.FindWord(eit->GetID(), *wit, loc)) {
				r.ctx.file=loc.file;
				r.ctx.line=(int)loc.line;
			}
			roots.push_back(r);
		}
	}
	for (vector<Zone>::const_iterator zit=zones.begin(); zit!=zones.end(); ++zit) {
		if (!zit->code) continue;
		Root r;
		r.code=zit->code;
		r.ctx.file=zit->file;
		roots.push_back(r);
	}
	// Ctx は entryname への参照を持つので、並びが確定してから結び付ける
	for (vector<Root>::iterator it=roots.begin(); it!=roots.end(); ++it)
		if (it->entryname.size()) it->ctx.entry=&it->entryname;

	collecting=true;
	for (vector<Root>::const_iterator it=roots.begin(); it!=roots.end(); ++it)
		Walk(it->code, it->ctx);

	collecting=false;
	findings.clear();
	reported.clear();
	for (vector<Root>::const_iterator it=roots.begin(); it!=roots.end(); ++it)
		Walk(it->code, it->ctx);

	// コンパイラが集めた所見 (木には残らない記述の癖)
	if (notes) {
		for (vector<TLintNotes::Note>::const_iterator it=notes->notes.begin(); it!=notes->notes.end(); ++it) {
			Finding f;
			f.file=srcmap.FileIndex(it->file);
			f.line=(int)it->line;
			f.seq=static_cast<unsigned int>(findings.size());
			f.code=it->code;
			f.name=it->name;
			findings.push_back(f);
		}
	}

	sort(findings.begin(), findings.end(), FindingLess);
	unsigned int count=0;
	for (vector<Finding>::const_iterator it=findings.begin(); it!=findings.end(); ++it) {
		// 「# lint:disable-next-line=K-W...」で抑制された行
		if (notes && it->file && notes->IsSuppressed(srcmap.FileName(it->file), (unsigned int)it->line, it->code))
			continue;
		count++;

		TDiagStream d=logger.Diag(kawari_log::LOG_WARNING, it->code, 0);
		if (it->file) d.At(srcmap.FileName(it->file), it->line);
		switch (it->code) {
		case kawari_log::W_LINT_UNDEFINED_ENTRY:
			d << RC.S(WARN_LINT_UNDEFINED_ENTRY1) << it->name << RC.S(WARN_LINT_UNDEFINED_ENTRY2);
			break;
		case kawari_log::W_LINT_UNKNOWN_COMMAND:
			d << RC.S(WARN_LINT_UNKNOWN_COMMAND1) << it->name << RC.S(WARN_LINT_UNKNOWN_COMMAND2);
			break;
		case kawari_log::W_LINT_REGEX_PAREN:
			d << it->name << RC.S(WARN_LINT_REGEX_PAREN);
			break;
		case kawari_log::W_LINT_REGEX_HISTORY:
			d << it->name << RC.S(WARN_LINT_REGEX_HISTORY);
			break;
		}
		// 位置が分からないときは、せめてエントリ名を添える
		if ((!it->file)&&it->entry.size()) d << L" (in ${" << it->entry << L"})";
	}
	return count;
}
//---------------------------------------------------------------------------
// 中間コードの木を歩く
void TKawariLint::Walk(const TKVMCode_base *code, const Ctx &ctx)
{
	if (!code) return;

	vector<const TKVMCode_base *> children;
	code->GetChildren(children);

	// =kis ゾーンの最上位は、文ごとの行を持っている
	const TKVMCodeInlineScript *inl=dynamic_cast<const TKVMCodeInlineScript *>(code);
	if (inl) {
		for (unsigned int i=0; i<children.size(); i++) {
			Ctx c=ctx;
			if ((i<inl->lines.size())&&inl->lines[i]) c.line=(int)inl->lines[i];
			Walk(children[i], c);
		}
		return;
	}

	const TKVMCodeScriptStatement *st=dynamic_cast<const TKVMCodeScriptStatement *>(code);
	if (st) {
		Statement(st, ctx);
	} else if (collecting) {
		// "$(setstr name ...)" のように、文字列の中に書かれた (後で評価される) スクリプトの名前
		const TKVMCodeString *lit=dynamic_cast<const TKVMCodeString *>(code);
		if (lit && (lit->s.find(L"$(")!=wstring::npos)) CollectEmbedded(lit->s);
	} else {
		// ${名前} / ${名前 + 名前} / $名前[添字] で、名前がそのまま書かれているもの
		const TKVMCodePVW *pvw=dynamic_cast<const TKVMCodePVW *>(code);
		if (pvw) {
			Reference(pvw->EntryName(), ctx);
		} else {
			const TKVMSetCodeWord *sw=dynamic_cast<const TKVMSetCodeWord *>(code);
			if (sw) {
				const TKVMCodeIDString *id=sw->GetIfPVW();
				if (id) Reference(id->s, ctx);
			}
		}
	}

	if (!collecting) {
		const TKVMCodeEntryIndex *ei=dynamic_cast<const TKVMCodeEntryIndex *>(code);
		if (ei && (children.size()>0)) {
			const TKVMCodeString *name=dynamic_cast<const TKVMCodeString *>(children[0]);
			if (name) Reference(name->s, ctx);
		}
	}

	for (unsigned int i=0; i<children.size(); i++)
		Walk(children[i], ctx);
}
//---------------------------------------------------------------------------
// スクリプト文 : 1 周目は引数のリテラルと function の定義名を集め、2 周目はコマンド名を調べる
void TKawariLint::Statement(const TKVMCodeScriptStatement *st, const Ctx &ctx)
{
	const vector<TKVMCode_base *> &list=st->list;
	if (list.empty()) return;

	const TKVMCodeString *arg0=dynamic_cast<const TKVMCodeString *>(list[0]);

	if (collecting) {
		if (arg0 && (arg0->s==L"function") && (list.size()>=2)) {
			const TKVMCodeString *fname=dynamic_cast<const TKVMCodeString *>(list[1]);
			if (fname) funcs.insert(fname->s);
		}
		for (unsigned int i=1; i<list.size(); i++) {
			const TKVMCodeString *lit=dynamic_cast<const TKVMCodeString *>(list[i]);
			if (!lit) {
				// 名前の一部が組み立てられるもの (launch_a_"${n}") : 先頭のリテラルを接頭辞として覚える
				const TKVMCodeList *cat=dynamic_cast<const TKVMCodeList *>(list[i]);
				if (cat && !cat->list.empty()) {
					const TKVMCodeString *head=dynamic_cast<const TKVMCodeString *>(cat->list[0]);
					if (head && LooksLikeEntryName(head->s)) dynprefix.insert(head->s);
				}
				continue;
			}
			wstring name=lit->s;
			wstring::size_type br=name.find(L'[');
			if (br!=wstring::npos) name.erase(br);
			if (name.size()) mentioned.insert(name);
		}
		return;
	}

	if (arg0 && arg0->s.size() && !IsKnownCommand(arg0->s))
		Report(kawari_log::W_LINT_UNKNOWN_COMMAND, arg0->s, ctx);

	// regex_replace(_first) string pattern replace : 置換文字列の ${N} は引用符で囲まないと、
	// 正規表現に届く前に履歴参照として展開されてしまう
	if (arg0 && ((arg0->s==L"regex_replace")||(arg0->s==L"regex_replace_first")) && (list.size()>3)) {
		bool history=(dynamic_cast<const TKVMCodeHistoryCall *>(list[3])!=NULL);
		if (!history) {
			const TKVMCodeList *cat=dynamic_cast<const TKVMCodeList *>(list[3]);
			if (cat)
				for (vector<TKVMCode_base *>::const_iterator it=cat->list.begin(); it!=cat->list.end(); ++it)
					if (dynamic_cast<const TKVMCodeHistoryCall *>(*it)) { history=true; break; }
		}
		if (history) Report(kawari_log::W_LINT_REGEX_HISTORY, arg0->s, ctx);
	}
}
//---------------------------------------------------------------------------
// 文字列に埋め込まれたスクリプトの単語を、定義されうる名前として覚える
// (どれがコマンドでどれが引数かは分からないので、区切った単語を全部入れる)
void TKawariLint::CollectEmbedded(const wstring &s)
{
	static const wchar_t sep[]=L" \t\r\n\"\\()[]{};,$";
	wstring::size_type pos=0;
	while (pos<s.size()) {
		wstring::size_type b=s.find_first_not_of(sep, pos);
		if (b==wstring::npos) break;
		wstring::size_type e=s.find_first_of(sep, b);
		if (e==wstring::npos) e=s.size();
		mentioned.insert(s.substr(b, e-b));
		// 文字列が単語の途中で終わるときは、続きが組み立てられる名前の接頭辞かもしれない
		// ("$(copy launch_a_"${n}"_7 ..." の launch_a_)
		if ((e==s.size())&&(e-b>=3)&&LooksLikeEntryName(s.substr(b, e-b)))
			dynprefix.insert(s.substr(b, e-b));
		pos=e;
	}
}
//---------------------------------------------------------------------------
// エントリ名の先頭部分として使えそうか (空白や引用符を含む文章の書き出しは除く)
bool TKawariLint::LooksLikeEntryName(const wstring &s)
{
	if (s.empty()) return false;
	for (wstring::size_type i=0; i<s.size(); i++) {
		wchar_t c=s[i];
		if ((c<=L' ')||(c==L'"')||(c==L',')||(c==L'(')||(c==L')')||(c==L'$')||(c==L'\x3000'))
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------
// ステートメント自身が解釈する語、組み込み関数、定義された関数のどれか
bool TKawariLint::IsKnownCommand(const wstring &name)
{
	static const wchar_t *const special[]={
		L"NULL", L"break", L"continue", L"return", L"function", L"lambda", L"rmfunc",
		L"while", L"until", L"loop", L"foreach", L"?", L"silent", L"if", NULL
	};
	for (const wchar_t *const *p=special; *p; ++p)
		if (name==*p) return true;

	TKisFunctionInfo info;
	if (name[0]==L'.') {
		// 先頭の '.' は強制ビルトイン呼び出し
		return vm.GetFunctionInfo(name.substr(1), info);
	}
	if (vm.GetFunctionInfo(name, info)) return true;
	if (funcs.count(name)) return true;
	return dict.GetEntry(TKawariVM::SYSTEM_FUNCTION_PREFIX+name).IsValid();
}
//---------------------------------------------------------------------------
// エントリの参照 : どこにも定義されていなければ警告する
void TKawariLint::Reference(const wstring &name, const Ctx &ctx)
{
	if (name.empty()) return;
	if (name[0]==L'@') return;
	if (name.compare(0, 7, L"System.")==0) return;
	if (mentioned.count(name)) return;
	if (dict.GetEntry(name).IsValid()) return;
	// 組み立てた名前で作られうるもの
	for (set<wstring>::const_iterator it=dynprefix.begin(); it!=dynprefix.end(); ++it)
		if (name.compare(0, it->size(), *it)==0) return;
	Report(kawari_log::W_LINT_UNDEFINED_ENTRY, name, ctx);
}
//---------------------------------------------------------------------------
void TKawariLint::Report(unsigned int code, const wstring &name, const Ctx &ctx)
{
	// 同じ場所の同じ名前は 1 回だけ
	wstring key=IntToString(code)+L":";
	key+=IntToString(ctx.file)+L":"+IntToString(ctx.line)+L":";
	if ((!ctx.file)&&ctx.entry) key+=*ctx.entry+L":";
	key+=name;
	if (!reported.insert(key).second) return;

	Finding f;
	f.file=ctx.file;
	f.line=ctx.line;
	f.seq=static_cast<unsigned int>(findings.size());
	f.code=code;
	f.name=name;
	if (ctx.entry) f.entry=*ctx.entry;
	findings.push_back(f);
}
//---------------------------------------------------------------------------
