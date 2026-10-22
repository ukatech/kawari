!ifdef CC
!error -rオプション(デフォルトルール抑制)付で実行してください
!endif

!include files.mak

.suffixes : .cxx .cpp .hxx .h

MACHSRC = ./saori/saori_win32.cpp

KAWAOBJ =  $(KAWASRC:.cpp=.obj) $(MACHSRC:.cpp=.obj)

SHIOOBJ =  $(SHIOSRC:.cpp=.obj)

CRYPTOBJ = $(CRYPTSRC:.cpp=.obj)



CFLAGS = -O2 -I. -w-8027 -DUSEKDB
LIBS   = 

UPX = upx
#UPX = rem
# upxは実行ファイル圧縮ツール
# http://wildsau.idv.uni-linz.ac.at/mfx/upx.html
# を参照してください

!ifdef SHIORIVER
SHIORIVER=-DSHIORIVER="\"$(SHIORIVER)\""
!endif



## TARGETS ##

ALLTARGET = ./shiori_dll/shiori.dll ./tool/kosui.exe ./tool/kawari_encode.exe

all : $(ALLTARGET)


./shiori_dll/shiori.dll : ./shiori_dll/shiori.obj $(SHIOOBJ) $(KAWAOBJ)
	bcc32 -tWD $** -l./shiori_dll/bcc_shiori.def
	$(UPX) $@


./tool/kosui.exe : ./tool/kosui.obj ./tool/kdb.obj $(KAWAOBJ)
	bcc32 -e$@ $** $(LIBS)
	$(UPX) $@


./tool/kawari_encode.exe : ./tool/kawari_encode.obj $(CRYPTOBJ)
	bcc32 -e$@ $** $(LIBS)
	$(UPX) $@


.cxx.obj:
	bcc32 -P $(CFLAGS) $(SHIORIVER) -n$: -c $<

.cpp.cxx:
	ruby sjis2ascii.rb < $< > $@

.h.hxx:
	ruby sjis2ascii.rb < $< > $@


cleanxx : 
	-for %d in ( .\kis .\libkawari .\shiori_dll .\tool ) do del %d\*.cxx
	-for %d in ( .\kis .\libkawari .\shiori_dll .\tool ) do del %d\*.hxx
	-del config.hxx

clean : cleanxx
	-for %f in ( $(KAWAOBJ:/=\) $(SHIOOBJ:/=\) ) do del %f
	-for %d in ( .\shiori_dll .\tool ) do del %d\*.tds
	-for %d in ( .\shiori_dll .\tool ) do del %d\*.obj

cleanall : clean
	-for %f in ( $(ALLTARGET:/=\) ) do del %f

depend : clean
	ruby ./makedepend.rb --with-sjis2ascii > depend.mak


!include depend.mak

