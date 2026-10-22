//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- オンラインヘルプ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.07.21  Phase 6.2     First version
//  2001.08.06  Phase 6.2     ver追加
//  2001.08.12  Phase 6.2.1   mingw対策
//  2003.02.16  Phase 7.5.0   ライセンス表記改修
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_help.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_iscript.h"
#include "libkawari/kawari.h"
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
using namespace std;
//---------------------------------------------------------------------------
// 華和梨ライセンス (modified BSD(FreeBSD) style license)

#define KAWARI_CORE_LICENSE \
"Copyright (C) 2001-2003 KAWARI Development Team\n" \
"(Meister(original works)/Nie/Nise-Meister/Sato/Shino/Suikyo)\n" \
"Contributers (Ebisawa/MDR/Sanori/Whoami)\n" \
"All rights reserved.\n" \
"\n" \
"Redistribution and use in source and binary forms, with or without \n" \
"modification, are permitted provided that the following conditions are \n" \
"met: \n" \
"\n" \
"1. Redistributions of source code must retain the above copyright \n" \
"   notice, this list of conditions and the following disclaimer \n" \
"   as the first lines of this file unmodified." \
"\n" \
"2. Redistributions in  binary form must reproduce the above copyright \n" \
"   notice, this list of conditions and the following disclaimer in the \n" \
"   documentation and/or other materials provided with the distribution. \n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR \n" \
"IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED \n" \
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE \n" \
"DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, \n" \
"INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES \n" \
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR \n" \
"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) \n" \
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, \n" \
"STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN \n" \
"ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE \n" \
"POSSIBILITY OF SUCH DAMAGE.\n" \
"\n" \
"License of MT19937 library is following.\n" \
"Redistribution and use in source and binary forms, with or without\n" \
"modification, are permitted provided that the following conditions\n" \
"are met:\n" \
"\n" \
"  1. Redistributions of source code must retain the above copyright\n" \
"     notice, this list of conditions and the following disclaimer.\n" \
"\n" \
"  2. Redistributions in binary form must reproduce the above copyright\n" \
"     notice, this list of conditions and the following disclaimer in the\n" \
"     documentation and/or other materials provided with the distribution.\n" \
"\n" \
"  3. The names of its contributors may not be used to endorse or promote \n" \
"     products derived from this software without specific prior written \n" \
"     permission.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n" \
"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n" \
"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n" \
"A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR\n" \
"CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n" \
"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n" \
"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n" \
"PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n" \
"LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n" \
"NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n" \
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"

//---------------------------------------------------------------------------
string KIS_help::Function(const vector<string>& args)
{
	if(args.size()>=2) {
		const TKisFunction_base *func=KisEngine->Function(args[1]);
		if(!func) return(string("Command \"")+args[1]+string("\" is NOT exist."));

		return(string(func->Name())+"\n"
		 +"syntax  : "+string(func->Format())+"\n"
		 +"return  : "+string(func->Returnval())+"\n"
		 +"comment : "+string(func->Information()));
	} else {
		string retstr="Command list :";
		const vector<TKisFunction_base*> &functable=KisEngine->GetFunctionTable();

		vector<TKisFunction_base*>::const_iterator it;
		for(it=functable.begin();it!=functable.end();it++) {
			retstr+="\n\t";
			retstr+=(*it)->Name();
		}

		return(retstr);
	}

}
//---------------------------------------------------------------------------
string KIS_ver::Function(const vector<string>& args)
{
	if ((args.size()>=2)&&(args[1]=="license")){
		// ライセンス表示
		ostream *logfs=KisEngine->Engine()->GetLogStream();
		if (logfs)
			(*logfs) << KAWARI_CORE_LICENSE;
		else
			cout << KAWARI_CORE_LICENSE;
		return ("");
	}else{
		return(KAWARI_NAME "/" KAWARI_MAJOR "." KAWARI_MINOR "." KAWARI_SUBMINOR);
	}
}
//---------------------------------------------------------------------------

