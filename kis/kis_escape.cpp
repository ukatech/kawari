//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- エスケープ --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.07.14  Phase 6.1     First version
//  2002.01.11  Phase 7.3.1   I18N Hack (kpcg.I18Nhack)
//  2003.02.16  Phase 7.5.0
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_escape.h"
//---------------------------------------------------------------------------
#include "misc/misc.h"
#include "misc/i18n.h"
//---------------------------------------------------------------------------
string KIS_escape::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	string retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=string(" ");

		for(unsigned int j=0;j<args[i].size();j++) {
#ifdef I18N_HACK
			if(int trail=lang.moreBytes(args[i][j])) {
				for(int k = 0; k < trail; k++) {
					retstr += args[i][j++];
				}
				retstr+=args[i][j];
#else
			if(iskanji1st(args[i][j])) {
				retstr+=args[i][j++];
				retstr+=args[i][j];
#endif
			} else {
				if((args[i][j]=='\\')||(args[i][j]=='%')) retstr+='\\';
				retstr+=args[i][j];
			}
		}
	}


	return(retstr);
}
//---------------------------------------------------------------------------

