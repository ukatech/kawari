.autodepend

!include files.mak

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
	bcc32 -tWD -e$@ $** $(LIBS)
	$(UPX) $@


./tool/kosui.exe : ./tool/kosui.obj ./tool/kdb.obj $(KAWAOBJ)
	bcc32 -e$@ $** $(LIBS)
	$(UPX) $@


./tool/kawari_encode.exe : ./tool/kawari_encode.obj $(CRYPTOBJ)
	bcc32 -e$@ $** $(LIBS)
	$(UPX) $@


.cpp.obj:
	bcc32 $(CFLAGS) $(SHIORIVER) -n$: -c $<


clean :
	-for %f in ( $(KAWAOBJ:/=\) $(SHIOOBJ:/=\) ) do del %f
	-for %d in ( .\shiori_dll .\tool ) do del %d\*.tds
	-for %d in ( .\shiori_dll .\tool ) do del %d\*.obj

cleanall : clean
	-for %f in ( $(ALLTARGET:/=\) ) do del %f



