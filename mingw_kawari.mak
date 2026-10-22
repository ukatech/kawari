
# -rオプション(デフォルトルール抑制)付で実行してください

include files.mak

.suffixes : .cxx .cpp .hxx .h

MACHSRC = ./saori/saori_win32.cpp

KAWAOBJ =  $(KAWASRC:.cpp=.o) $(MACHSRC:.cpp=.o)

SHIOOBJ =  $(SHIOSRC:.cpp=.o)

CRYPTOBJ = $(CRYPTSRC:.cpp=.o)

CFLAGS = -O2 -I. -DUSEKDB -DI18N_HACK
LIBS   = 

UPX = upx
#UPX = rem
# upxは実行ファイル圧縮ツール
# http://wildsau.idv.uni-linz.ac.at/mfx/upx.html
# を参照してください


## TARGETS ##

ALLTARGET = ./shiori_dll/shiori.dll ./tool/kosui.exe ./tool/kawari_encode.exe

all : $(ALLTARGET)


./shiori_dll/shiori.dll : ./shiori_dll/shiori.o $(SHIOOBJ) $(KAWAOBJ)
	dllwrap --driver-name g++ --def ./shiori_dll/mingw_shiori.def -o$@ ./shiori_dll/shiori.o $(SHIOOBJ) $(KAWAOBJ) $(LIBS)
	strip $@
	$(UPX) $@


./tool/kosui.exe : ./tool/kosui.o ./tool/kdb.o $(KAWAOBJ)
	g++ -o$@ ./tool/kosui.o ./tool/kdb.o $(KAWAOBJ) $(LIBS)
	strip $@
	$(UPX) $@


./tool/kawari_encode.exe : ./tool/kawari_encode.o $(CRYPTOBJ)
	g++ -o$@ ./tool/kawari_encode.o $(CRYPTOBJ) $(LIBS)
	strip $@
	$(UPX) $@


%.o : %.cxx
	g++ $(CFLAGS) $(SHIORIVER) -o$@ -c $<

%.cxx : %.cpp
	ruby sjis2ascii.rb < $< > $@

%.hxx : %.h
	ruby sjis2ascii.rb < $< > $@



cleanxx : 
	-rm ./kis/*.cxx ./libkawari/*.cxx ./shiori_dll/*.cxx ./tool/*.cxx
	-rm ./kis/*.hxx ./libkawari/*.hxx ./shiori_dll/*.hxx ./tool/*.hxx
	-rm config.hxx

clean : cleanxx
	-rm $(KAWAOBJ) $(SHIOOBJ) $(CRYPTOBJ) ./shiori_dll/shiori.o ./tool/kosui.o ./tool/kdb.o ./tool/kawari_encode.o 

cleanall : clean
	-rm $(ALLTARGET)

depend : clean
	ruby ./makedepend.rb --with-sjis2ascii > depend.mak


include depend.mak

