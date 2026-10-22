//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// KawariInlineScript -- 保守的piro --
//
//      Programed by NAKAUE.T (Meister)
//
//  2001.03.19  Phase 0.5     Piroject-X
//  2001.05.31  Phase 5.2     KISインターフェース
//  2001.06.10  Phase 5.3.1   「-」を「.」に
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#include "kis/kis_cpiro.h"
//---------------------------------------------------------------------------
#include "libkawari/kawari_engine.h"
//---------------------------------------------------------------------------
string KIS_pirocall::Function(const vector<string>& args)
{
	if((args.size()!=2)&&(args.size()!=3)) return("");

	string entry="piro."+KisEngine->Engine()->EncodeEntryName(args[1]);

	string retstr=KisEngine->Engine()->RandomSelect(entry);
	if(retstr.size()) return(retstr);
	 else if(args.size()==3) return(args[2]);

	return("");

}
//---------------------------------------------------------------------------

