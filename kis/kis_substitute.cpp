//---------------------------------------------------------------------------
//
// "‰Ø˜a—œ" for ‚ ‚êˆÈŠO‚Ì‰½‚©ˆÈŠO‚Ì‰½‚©
// KawariInlineScript -- toupper, tolower --
//
//      Programed by Chikara.H (MDR)
//
//  2001.09.04  created(9Š„‚Íkis_escape.cpp‚©‚ç”qØj
//  2002.01.11  Phase 7.3.1   I18N Hack (kpcg.I18Nhack)
//  2003.02.16  Phase 7.5.0
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_substitute.h"
#include "misc/misc.h"
#include "misc/i18n.h"
//---------------------------------------------------------------------------
#include <ctype.h>
#include <cstdlib>
using namespace std;
//---------------------------------------------------------------------------
string KIS_toupper::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	string retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=string(" ");
		for(unsigned int j=0;j<args[i].size();j++) {
#ifdef I18N_HACK
			if(int trail=lang.moreBytes(args[i][j])) {
				for(int k=0; k < trail; k++) {
					retstr +=args[i][j++];
				}
				retstr+=args[i][j];
#else
			if(iskanji1st(args[i][j])) {
				retstr+=args[i][j++];
				retstr+=args[i][j];
#endif
			} else {
				if( 'a' <= args[i][j] && args[i][j] <= 'z') {
					retstr+= toupper(args[i][j]);
				} else {
					retstr+=args[i][j];
				}
			}
		}
	}


	return(retstr);
}
//---------------------------------------------------------------------------
string KIS_tolower::Function(const vector<string>& args)
{
	if(args.size()<2) return("");

	string retstr;
	for(unsigned int i=1;i<args.size();i++) {
		if(i>1) retstr+=string(" ");

		for(unsigned int j=0;j<args[i].size();j++) {
#ifdef I18N_HACK
			if(int trail=lang.moreBytes(args[i][j])) {
				for(int k=0; k < trail; k++) {
					retstr +=args[i][j++];
				}
				retstr+=args[i][j];
#else
			if(iskanji1st(args[i][j])) {
				retstr+=args[i][j++];
				retstr+=args[i][j];
#endif
			} else {
				if( args[i][j] >= 'A'  && args[i][j] <= 'Z') {
					retstr+= tolower(args[i][j]);
				} else {
					retstr+=args[i][j];
				}
			}
		}
	}


	return(retstr);
}
//

//---------------------------------------------------------------------------

