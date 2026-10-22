//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 暗号化/復号化
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.03.15  Phase 0.42    暗号化実装
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_crypt.h"
#include "misc/base64.h"
//---------------------------------------------------------------------------
// 暗号化形式を確認する
bool CheckCrypt(const string& encodedstr)
{
	string id=encodedstr.substr(0,9);
	return(id=="!KAWA0000");
}
//---------------------------------------------------------------------------
// 標準暗号化
string EncryptString(const string& str)
{
	string encodedstr;

	for(unsigned int i=0;i<str.size();i++) {
		encodedstr+=str[i]^0xcc;
	}

	return(string("!KAWA0000")+EncodeBase64(encodedstr));
}
//---------------------------------------------------------------------------
// 標準復号化
#if defined(STRING_APPEND_INATOMIC)
string DecryptString(const string& encodedstr)
{
	// long int = 32bit であることに依存したコーディングになってます 
	string str=DecodeBase64(encodedstr.substr(9));

	unsigned cnt = str.size();
	string decodedstr;
	
	char *s = new char[cnt + 4];
	
	str.copy(s, cnt);
	
	for(unsigned int i=0;i<cnt ;i += 4) {
		*(unsigned long *)(s + i) ^= 0xccccccccUL;
	}

	decodedstr.assign(s, cnt);
	delete[] s;
	return(decodedstr);
}
#else
string DecryptString(const string& encodedstr)
{
	string str=DecodeBase64(encodedstr.substr(9));

	string decodedstr;
	for(unsigned int i=0;i<str.size();i++) {
		decodedstr+=str[i]^0xcc;
	}

	return(decodedstr);
}
#endif
//---------------------------------------------------------------------------
