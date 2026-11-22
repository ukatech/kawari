//---------------------------------------------------------------------------
//
// TPHMessage - 偽HTTPメッセージ -
//
//      Programed by Suikyo.
//
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "misc/phttp.h"
//---------------------------------------------------------------------------
#include <sstream>
#include <iostream>
#include <string>
using namespace std;
//---------------------------------------------------------------------------
// シリアライズ (wide文字列)
wstring TPHMessage::SerializeW(void) const{
	wstring ret=startline;
	ret+=L"\x0d\x0a";

	for(TPHMessage::const_iterator it=begin();it!=end();it++) {
		if(it->second.size())
			ret+=it->first+L": "+it->second+L"\x0d\x0a";
	}
	ret+=L"\x0d\x0a";
	return ret;
}
//---------------------------------------------------------------------------
// シリアライズ (指定文字コードのバイト列)
string TPHMessage::Serialize(TKawariCharset cs) const{
	return WtoMB(SerializeW(), cs);
}
//---------------------------------------------------------------------------
// デシリアライズ (wide文字列)
void TPHMessage::DeserializeW(const wstring &mes){
	wistringstream ifs(mes);
	wstring buff;

	getline(ifs,buff,L'\x0a');
	if(buff.size()&&(buff[buff.size()-1]==L'\x0d')) buff.erase(buff.size()-1);	// 0x0dを取り除く
	startline=buff;

	// ヘッダを分解してmapに積む
	while(getline(ifs,buff,L'\x0a')) {
		if(buff.size()==0) break;
		if(buff[buff.size()-1]==L'\x0d') buff.erase(buff.size()-1);	// 0x0dを取り除く
		if(buff.size()==0) break;

		wstring::size_type pos=buff.find(L':');
		wstring key=buff.substr(0,pos);

		if(pos==wstring::npos){
			Add(key,L"");
			continue;
		}
		pos++;
		while((pos<buff.size())&&(buff[pos]==L' ')) pos++;

		Add(key,buff.substr(pos));
	}
}
//---------------------------------------------------------------------------
// デシリアライズ (指定文字コードのバイト列)
void TPHMessage::Deserialize(const string &mes, TKawariCharset cs){
	DeserializeW(MBtoW(mes, cs));
}
//---------------------------------------------------------------------------
// 生バイト列のヘッダから"Charset:"を探す
TKawariCharset TPHMessage::ScanCharset(const string &mes){
	string::size_type pos=0;
	bool first=true;
	while(pos<=mes.size()){
		string::size_type eol=mes.find('\x0a', pos);
		string line=mes.substr(pos, (eol==string::npos)?string::npos:eol-pos);
		if(line.size()&&(line[line.size()-1]=='\x0d')) line.erase(line.size()-1);
		if(!first){
			if(line.size()==0) break;	// ヘッダ終了
			if(line.size()>8){
				static const char key[]="charset:";
				bool match=true;
				for(int i=0; i<8; i++){
					char c=line[i];
					if((c>='A')&&(c<='Z')) c=(char)(c-'A'+'a');
					if(c!=key[i]){ match=false; break; }
				}
				if(match){
					string val=line.substr(8);
					string::size_type st=val.find_first_not_of(" \t");
					string::size_type en=val.find_last_not_of(" \t");
					if(st==string::npos) return KC_NULL;
					return CharsetFromNameExt(AtoW(val.substr(st, en-st+1)));
				}
			}
		}
		first=false;
		if(eol==string::npos) break;
		pos=eol+1;
	}
	return KC_NULL;
}
//---------------------------------------------------------------------------
// ダンプ
void TPHMessage::Dump(wostream &os) const{
	os << startline << endl;
	for (TPHMessage::const_iterator it=begin(); it!=end(); it++){
		os << it->first << L": " << it->second << endl;
	}
	os << endl;
}
//---------------------------------------------------------------------------
