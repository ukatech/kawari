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
//  2002.03.17  Phase 7.9.0   helpの出力先を標準出力に変更
//  2002.04.22  Phase 8.0.0   ver license追加
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_help.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_vm.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_version.h"
#include "libkawari/kawari_log.h"
//---------------------------------------------------------------------------
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------
wstring KIS_help::Function(const vector<wstring>& args)
{
	if(args.size()>=2) {
		TKisFunctionInfo info;
		if (Engine->GetFunctionInfo(args[1], info)){
			GetLogger().GetStream() << info.name << endl
			 << L"syntax  : " << info.format << endl
			 << L"return  : " << info.returnval << endl
			 << L"comment : " << info.information << endl;
		} else {
			GetLogger().GetStream() << L"Command \"" << args[1] << L"\" is NOT exist." << endl;
		}
	} else {
		GetLogger().GetStream() << L"Command list :" << endl;
		vector<wstring> list;
		Engine->GetFunctionList(list);

		vector<wstring>::const_iterator it;
		for(it=list.begin();it!=list.end();it++) {
			GetLogger().GetStream() << (*it) << endl;
		}
	}

	return(L"");
}
//---------------------------------------------------------------------------
wstring KIS_ver::Function(const vector<wstring>& args)
{
	if ((args.size()>=2)&&(args[1]==L"license")){
		// 狭い文字列リテラルとワイド文字列リテラルの連結は VC6 が受け付けない
		GetLogger().GetStream() << AtoW(std::string(KAWARI_CORE_LICENSE));
		return (L"");
	}else if((args.size()>=2)&&(args[1]==L"author")) {
		return(AtoW(std::string(KAWARI_AUTHOR)));
	}else{
		return(AtoW(std::string(KAWARI_NAME "/" KAWARI_MAJOR "." KAWARI_MINOR "." KAWARI_SUBMINOR)));
	}
}
//---------------------------------------------------------------------------

