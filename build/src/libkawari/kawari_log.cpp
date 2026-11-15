//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// ロガー
//
//      Programed by Suikyo
//
//  2002.04.12  Phase 8.0.0   作成
//  2008.01.23  Phase 8.2.5   include <cstdio> to define [EOF] (PaulLiu)
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "libkawari/kawari_log.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
//---------------------------------------------------------------------------
class nullstreambuf : public wstreambuf {
public:
	nullstreambuf (): wstreambuf() {}
	virtual int_type overflow(int_type p_iChar=traits_type::eof()){
		return traits_type::not_eof(p_iChar);
	}
	virtual std::streamsize xsputn(const wchar_t *, std::streamsize n){
		return n;
	}
	virtual int_type uflow(void){
		return traits_type::eof();
	}
	virtual int_type pbackfail(int_type p_iChar=traits_type::eof()){
		return p_iChar;
	}
	virtual int_type underflow(void){
		return traits_type::eof();
	}
	virtual int sync(){
		return 0;
	}
};
//---------------------------------------------------------------------------
TWideToByteStreamBuf::TWideToByteStreamBuf(std::ostream *os, TKawariCharset cs, bool crlf_)
	: dest(os), fdest(NULL), charset(cs), crlf(crlf_)
{
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::TWideToByteStreamBuf(FILE *fp, TKawariCharset cs, bool crlf_)
	: dest(NULL), fdest(fp), charset(cs), crlf(crlf_)
{
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::~TWideToByteStreamBuf()
{
	Flush(true);
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::SetDestination(std::ostream *os, TKawariCharset cs, bool crlf_)
{
	Flush(true);
	dest=os;
	fdest=NULL;
	charset=cs;
	crlf=crlf_;
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::SetDestination(FILE *fp, TKawariCharset cs, bool crlf_)
{
	Flush(true);
	dest=NULL;
	fdest=fp;
	charset=cs;
	crlf=crlf_;
}
//---------------------------------------------------------------------------
void TWideToByteStreamBuf::Flush(bool final)
{
	if(pending.empty()) return;

	wstring::size_type len=pending.size();
	// 16bit wchar_tでは、末尾の上位サロゲートは次回に回す
	if((!final)&&(sizeof(wchar_t)<4)) {
		unsigned long last=(unsigned long)pending[len-1]&0xffff;
		if((last>=0xd800)&&(last<=0xdbff)) len--;
	}
	if(len==0) return;

	if(dest||fdest) {
		wstring w=pending.substr(0, len);
		if(crlf) {
			wstring c;
			c.reserve(w.size()+16);
			for(wstring::size_type i=0; i<w.size(); i++) {
				if(w[i]==0x0a) c+=(wchar_t)0x0d;
				c+=w[i];
			}
			w.swap(c);
		}
		string bytes=WtoMB(w, charset);
		if(fdest) {
			if(!bytes.empty()) fwrite(bytes.data(), 1, bytes.size(), fdest);
			if(final) fflush(fdest);
		} else {
			if(!bytes.empty()) dest->write(bytes.data(), (std::streamsize)bytes.size());
			if(final) dest->flush();
		}
	}
	pending.erase(0, len);
}
//---------------------------------------------------------------------------
TWideToByteStreamBuf::int_type TWideToByteStreamBuf::overflow(int_type c)
{
	if(!traits_type::eq_int_type(c, traits_type::eof())) {
		wchar_t wc=traits_type::to_char_type(c);
		pending+=wc;
		if((wc==L'\n')||(pending.size()>=4096)) Flush(false);
	}
	return traits_type::not_eof(c);
}
//---------------------------------------------------------------------------
std::streamsize TWideToByteStreamBuf::xsputn(const wchar_t *s, std::streamsize n)
{
	if(n<=0) return 0;
	pending.append(s, (wstring::size_type)n);
	if((pending.size()>=4096)||(wmemchr(s, L'\n', (size_t)n)!=NULL)) Flush(false);
	return n;
}
//---------------------------------------------------------------------------
int TWideToByteStreamBuf::sync(void)
{
	Flush(true);
	return 0;
}
//---------------------------------------------------------------------------
TWideOStream::TWideOStream(std::ostream *os, TKawariCharset cs, bool crlf)
	: wostream(NULL), buf(os, cs, crlf)
{
	rdbuf(&buf);
}
//---------------------------------------------------------------------------
TWideOStream::TWideOStream(FILE *fp, TKawariCharset cs, bool crlf)
	: wostream(NULL), buf(fp, cs, crlf)
{
	rdbuf(&buf);
}
//---------------------------------------------------------------------------
TWideOStream::~TWideOStream()
{
	flush();
}
//---------------------------------------------------------------------------
TKawariLogger::TKawariLogger(void){
	errlevel=0;
	diagctx=NULL;
	diagsink=NULL;
	collector=NULL;
	levellocked=false;
	StdOutStream=NULL;
	LogStream=NullStream=new wostream(new nullstreambuf);
}
//---------------------------------------------------------------------------
TKawariLogger::~TKawariLogger(void){
	if(StdOutStream) {
		StdOutStream->flush();
		delete StdOutStream;
	}
	delete NullStream->rdbuf();
	delete NullStream;
}
//---------------------------------------------------------------------------
// 出力ストリームを標準出力に設定
void TKawariLogger::SetStreamStdOut(void){
	if(!StdOutStream) StdOutStream=new TWideOStream(&cout, KC_ACP);
	LogStream=StdOutStream;
}
//---------------------------------------------------------------------------
// 診断メッセージの出力
// 形式 : [位置: ]error K-E0001: 本文
//        (続けて呼び出し連鎖が "    in ..." の形で 1 フレーム 1 行)
void TKawariLogger::EmitDiag(kawari_log::LOGLEVEL lvl, unsigned int code, bool explicitloc,
							 const wstring &file, int line, unsigned int depth, const wstring &msg) const
{
	wstring locfile=file;
	int locline=line;
	bool hasloc=explicitloc;
	if ((!hasloc)&&diagctx)
		hasloc=diagctx->GetLocation(locfile, locline);

	wostringstream o;
	if (hasloc) {
		o << locfile;
		if (locline>0) o << L"(" << locline << L")";
		o << L": ";
	}
	bool iserror=(lvl==kawari_log::LOG_ERROR);
	o << (iserror ? L"error K-E" : L"warning K-W");
	o << setw(4) << setfill(L'0') << code << setfill(L' ') << L": ";

	// 本文末尾の改行は、ここで付けるので取り除く
	wstring body=msg;
	while (body.size()&&((body[body.size()-1]==L'\n')||(body[body.size()-1]==L'\r')))
		body.erase(body.size()-1);
	o << body << L"\n";

	vector<wstring> chain;
	if (diagctx&&depth)
		diagctx->GetCallChain(chain, depth);
	for (vector<wstring>::const_iterator it=chain.begin(); it!=chain.end(); ++it)
		o << L"    " << *it << L"\n";

	if (collector) {
		if (iserror) collector->errors++; else collector->warnings++;
		if (collector->keep) {
			TDiagRecord r;
			r.iserror=iserror;
			r.code=code;
			r.hasloc=hasloc;
			if (hasloc) { r.file=locfile; r.line=locline; }
			r.message=body;
			r.chain=chain;
			collector->records.push_back(r);
		}
	}

	if (diagsink)
		*diagsink << o.str() << flush;
	else
		GetStream(lvl) << o.str() << flush;
}
//---------------------------------------------------------------------------
TDiagStream::~TDiagStream()
{
	if (active&&logger)
		logger->EmitDiag(lvl, code, explicitloc, file, line, depth, buf);
}
//---------------------------------------------------------------------------
//TKawariLogger Logger;
//---------------------------------------------------------------------------
