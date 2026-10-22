//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- expr --
//
//      Programed by Kouji.U (sky)
//
//  2001.06.02  created
//  2001.06.06  引数数制限仕様bug、findのbugをfix.
//  2001.06.09  BCC5.5.1でコンパイルできるように修正 (NAKAUE.T)
//  2001.07.10  Cygwinでコンパイルできるように修正 (NAKAUE.T)
//  2001.07.19  Phase 6.2     Mingw対応 (NAKAUE.T)
//  2001.08.06  Phase 6.2     libg++-v3対応 (NAKAUE.T)
//  2001.08.08  Phase 6.2     ostrstreamバグ修正
//  2001.10.29  Phase 7.0.2   change macro name(Thanks: Sanori)
//  2002.01.07  Phase 7.3     findposサブコマンド追加 (さとー)
//                            splitコマンド追加(さとー)
//	2002.01.11  Phase 7.3.1   I18N Hack (kpcg.I18Nhack)
//  2003.02.16  Phase 7.5.0
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include <strstream>
#ifdef __GNUC__
	#if (__GNUC__ < 3)
		#include <cctype>
	#else
		#include <cctype>
		#include <cwchar>
	#endif
	namespace std {
		typedef basic_string<wchar_t> wstring;
	}
#else
	#include <cwctype>
	#include <cwchar>
#endif

//#define EXPR_LOG
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
#include "misc/misc.h"
#include "misc/i18n.h"
#include "kis/kis_expr.h"
using namespace std;
//---------------------------------------------------------------------------
// 文字列操作時以外は全てstringで扱う。
// 数値もASCII数字のみ対応。
//
#define ISFALSE(x) ((x=="")||(x=="0")||(x=="false"))

#ifndef KIS_EXPR_REGEX
#	define Eval5 Eval6
#endif

// Wide Char String -> Char String
static std::string wtoc(const std::wstring& ws){
    unsigned int max = ws.length();
    std::string ret;
    for (unsigned int i=0; i<max; i++){

#ifdef I18N_HACK
		ret += lang.WcharToStr(ws[i]);
#else
        if (ws[i] & (unsigned short)0xff00){
            ret += static_cast<unsigned char>(
                (ws[i] & (unsigned short)0xff00) >> 8);
            ret += static_cast<unsigned char>(
                (ws[i] & (unsigned short)0x00ff));
        }else{
            ret += static_cast<unsigned char>(
                (ws[i] & (unsigned short)0x00ff));
        }
#endif
    }
    return ret;
}

// Char String -> Wide Char String
static std::wstring ctow(const std::string& s){
    unsigned int max = s.length();
    std::wstring ret;
    for (unsigned int i=0; i<max; i++){
    	
    	
#ifdef I18N_HACK
	int trail = lang.moreBytes(s[i]);
	if ((i+trail) < max)
	{
		ret+=lang.StrToWchar(s.substr(i), trail);
		i+=trail; // skip used data
	}
	else
		ret+=static_cast<unsigned char>(s[i]);
#else
        if (iskanji1st(s[i]) && (i<max-1)){
            ret +=
                (static_cast<unsigned char>(s[i++]) << 8) |
                    static_cast<unsigned char>(s[i]);
        }else{
            ret += static_cast<unsigned char>(s[i]);
        }
#endif
    }
    return ret;
}


static bool is_integer(string x) {
    int max = x.length();
    for (int i=0; i<max; i++)
        if (!isdigit(x[i])&&((x[i]!='-')&&(i==0)))
            return false;
    return true;
}

#define COMP_FUNC(name, rel)\
    static inline bool name(const std::string& l, const std::string& r){\
        if (is_integer(l) && is_integer(r)){\
            int il = atoi(l.c_str());\
            int ir = atoi(r.c_str());\
            return il rel ir;\
        }else{\
            return ctow(l) rel ctow(r);\
        }\
    }\

COMP_FUNC(kis_less_than, <)
COMP_FUNC(kis_less_equal, <=)
COMP_FUNC(kis_equal, ==)
COMP_FUNC(kis_not_equal, !=)
COMP_FUNC(kis_greater_equal, >=)
COMP_FUNC(kis_greater_than, >)

#define ARITH_FUNC(name, op) \
    static string name (const std::string& l, const std::string& r){\
        if (!is_integer(l) || !is_integer(r)) \
            throw string("non-numeric argument");\
        ostrstream ostr;\
        ostr << atoi(l.c_str()) op atoi(r.c_str()) << ends;\
		string retstr(ostr.str());\
		ostr.freeze(false);\
        return retstr;\
    }\

ARITH_FUNC (kis_plus, +)
ARITH_FUNC (kis_minus, -)
ARITH_FUNC (kis_multiply, *)

#define ARITHDIV_FUNC(name, op) \
    static string name (const std::string& l, const std::string& r){\
        if (!is_integer(l) || !is_integer(r)) \
            throw string("non-numeric argument");\
        int ir = atoi(r.c_str());\
        if (ir==0) \
            throw string("division by zero");\
        ostrstream ostr;\
        ostr << (atoi(l.c_str()) op ir) << ends;\
		string retstr(ostr.str());\
		ostr.freeze(false);\
        return retstr;\
    }\

ARITHDIV_FUNC(kis_divide, /)
ARITHDIV_FUNC(kis_mod, %)


//---------------------------------------------------------------------------
string KIS_expr::Function(const vector<string>& args){
    // 最短で expr hoge
    if (args.size()<2) return "";
#ifdef EXPR_LOG
    LogFS = new ofstream("c:\\tmp\\expr.log", ios_base::app|ios_base::out);
    (*LogFS) << "expr " ;
#endif
    if (Args.size()!=0)
        Args.erase(Args.begin(), Args.end());
    for (unsigned int i=1; i<args.size(); i++) {
#ifdef EXPR_LOG
        (*LogFS) << args[i] << ",";
#endif
        Args.push_back(args[i]);
    }
#ifdef EXPR_LOG
    (*LogFS) << " = " << endl << flush;
#endif
    Index = 0;
    try{
        string ret = Eval();
        if (Index<Args.size()){		// Error
#ifdef EXPR_LOG
            (*LogFS) << "syntax error" << endl << flush;
            LogFS->close();
            delete LogFS;
#endif
            return "";
        }else{
#ifdef EXPR_LOG
            (*LogFS) << ret << endl << flush;
            LogFS->close();
            delete LogFS;
#endif
            return ret;
        }
    }catch(string err){
#ifdef EXPR_LOG
        (*LogFS) << " failed " << err << endl << flush;
        LogFS->close();
        delete LogFS;
#endif
        return "";
    }
}

// 正規表現なんてやらない、やれない、やれるかっ。
string KIS_expr::EvalRegex(const std::string& l, const std::string& r){
//    return (l.find(r))? r : string("");
    return (ctow(l).find(ctow(r))!=string::npos)? r : string("");
}

// ( Expression ), bare-operand
string KIS_expr::Eval7(){
    if (Index>=Args.size())
        throw string("syntax error");
    if (testNext("(")){
        Index++;
        string val = Eval();
        if (!testNext(")"))
            throw string("syntax error");
        Index++;
        return val;
    }else if (testNext(")")){
        throw string("syntax error");
    }
    return Args[Index++];
}

// match, substr, index, lenth, quote
string KIS_expr::Eval6(){
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval6 " << endl << flush;
#endif
    if (testNext("quote")){
        Index++;
        if (Index>=Args.size())	// Error
            throw string("syntax error");
        else
            return Args[Index++];
    }else if (testNext("length")){
        Index++;
        ostrstream r;
        r << ctow(Eval6()).length() << ends;
		string retstr(r.str());
		r.freeze(false);
        return retstr;
    }else if (testNext("find")){
        Index++;
        string l = Eval6();
        string r = Eval6();
        l = EvalRegex(l, r);
        return l;
	}else if(testNext("findpos")){	// 現実に擦り寄る実装(2002/1/7 さとー)
		Index++;
		string l = Eval6();
		string r = Eval6();
		unsigned int idx;
		if((idx=ctow(l).find(ctow(r)))!=string::npos){
			ostrstream r;
			r << (idx + 1) << ends;
			string retstr(r.str());
			r.freeze(false);
			return retstr;
		}else{
			return "";
		}
    }else if (testNext("index")){
        Index++;
        wstring str = ctow(Eval6());
        wstring cs = ctow(Eval7());
        unsigned int idx = string::npos;
        unsigned int max=cs.length();
        for (unsigned int i=0; i<max; i++){
            string::size_type pos = str.find(cs[i]);
            if (pos != string::npos){
                idx = (pos<idx)? pos : idx;
            }
        }
        if (idx != string::npos){
            ostrstream r;
            r << (idx + 1) << ends;
			string retstr(r.str());
			r.freeze(false);
        	return retstr;
        }else{
            return "";
        }
    }else if (testNext("substr")){
        Index++;
        wstring l = ctow(Eval6());
        string si1 = Eval6();
        string si2 = Eval6();
        if (!is_integer(si1) || !is_integer(si2)) return "";
        int i1 = atoi(si1.c_str());
        int i2 = atoi(si2.c_str());
        // exprって1から数えるんだ・・・
        if (i1>(int)l.length() || i1<=0 || i2<=0) return "";
        return wtoc(l.substr(i1-1, i2));
    }else{
        return Eval7();
    }
}

// 正規表現
#ifdef KIS_EXPR_REGEX
string KIS_expr::Eval5(){
    string l = Eval6();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval5 " << l << endl << flush;
#endif
    while (true) {
        if (testNext(":")){
            Index++; string r=Eval6();
            l = EvalRegex(l, r);
        }else{
            return l;
        }
    }
    return l;
}
#endif

// *, /, %
string KIS_expr::Eval4(){
    string l = Eval5();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval4 " << l << endl << flush;
#endif
    while (true){
        ostrstream ret;
        if (testNext("*")){
            Index++; string r=Eval4();
            l = kis_multiply(l, r);
        }else if (testNext("/")){
            Index++; string r=Eval4();
            l = kis_divide(l, r);
        }else if (testNext("%")){
            Index++; string r=Eval4();
            l = kis_mod(l, r);
        }else{
            return l;
        }
    }
}

// +, -
string KIS_expr::Eval3(){
    string l = Eval4();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval3 " << l << endl << flush;
#endif
    while (true){
        if (testNext("+")){
            Index++; string r=Eval4();
            string val = kis_plus(l, r);
            l = val;
        }else if (testNext("-")){
            Index++; string r=Eval4();
            string val = kis_minus(l, r);
            l = val;
        }else{
            return l;
        }
    }
}

// 比較演算子
string KIS_expr::Eval2(){
    string l=Eval3();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval2 " << l << endl << flush;
#endif
    while (true) {
        if (testNext("<")){
            Index++; string r=Eval3(); l=(kis_less_than(l, r))? "1" : "";
        }else if (testNext("<=")){
            Index++; string r=Eval3(); l=(kis_less_equal(l, r))? "1" : "";
        }else if (testNext("=") || testNext("==")){
            Index++; string r=Eval3(); l=(kis_equal(l, r))? "1" : "";
        }else if (testNext("!=")){
            Index++; string r=Eval3(); l=(kis_not_equal(l, r))? "1" : "";
        }else if (testNext(">=")){
            Index++; string r=Eval3(); l=(kis_greater_equal(l, r))? "1" : "";
        }else if (testNext(">")){
            Index++; string r=Eval3(); l=(kis_greater_than(l, r))? "1" : "";
        }else{
            return l;
        }
    }
}

// '&'
string KIS_expr::Eval1(){
    string l = Eval2();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval1 " << l << endl << flush;
#endif
    while (true){
        if (testNext("&")) {
            Index++;
            string r = Eval2();
            if (ISFALSE(l) || ISFALSE(r)){
                l = "";
            }
        }else{
            return l;
        }
    }
}

// '|'
string KIS_expr::Eval(){
    string l = Eval1();
#ifdef EXPR_LOG
    if (*LogFS) (*LogFS) << "Eval " << l << endl << flush;
#endif
    while (true){
        if (testNext("|")){
            Index++;
            string r = Eval1();
            if (ISFALSE(l)) {
                l=r;
            }
        }else{
            return l;
        }
    }
}
//---------------------------------------------------------------------------
// splitコマンドパート

tokenizer::tokenizer(const string s, const string c)
{
	str=s;
	cs=c;
	pos=0;
	size=ctow(s).length();
}

tokenizer::Parse tokenizer::token(void)
{
	Parse ret;
	
	// ポインタが文字列最後尾に来た
	if(pos>=size) {
		ret.ParseState=tokenizer::Parse::parseEND;
		ret.str="";
		return ret;
	}
	
	unsigned int idx;	// 注目中のポインタ
	if((idx=ctow(str).substr(pos,size-pos).find(ctow(cs)))!=string::npos) {
		// 区切り文字列がポインタ以降の文字列中にあった
		ret.ParseState=tokenizer::Parse::parseOK;
		ret.str=wtoc(ctow(str).substr(pos,idx));	// 空文字列が返ることもある
		pos+=idx+ctow(cs).length();				// 区切り文字列分だけポインタ前進
	} else {
		// 区切り文字列がポインタ以降の文字列中にない
		ret.ParseState=tokenizer::Parse::parseOK;
		ret.str=wtoc(ctow(str).substr(pos,size-pos));	// 空文字が返ることもある
		pos=size;									// ポインタを文字列最後尾に
	}
	
	return ret;
}

string KIS_split::Function(const vector<string>& args){
	
	if(args.size()!=4) {
		return("");
	}
	
	tokenizer sentence(args[2],args[3]);
	int line=0;
	int nline=0;
	tokenizer::Parse result=sentence.token();
	
	while(result.ParseState==tokenizer::Parse::parseOK) {
		line++;
		string entry=args[1]+string(".")+IntToString(line);
		if(result.str.size()>0) { // 空ではなかったらエントリに追加
			KisEngine->Engine()->InsertAfterClear(entry,result.str);
		} else { // 空トークンをカウント
			nline++;
			KisEngine->Engine()->ClearEntry(entry);
		}
		result=sentence.token();
	}

	if((line>0)&&(nline!=line)) { // 空ではなかったら行数をセット
		KisEngine->Engine()->InsertAfterClear(args[1]+string(".size"),IntToString(line));
	}

	return("");
}

//---------------------------------------------------------------------------
