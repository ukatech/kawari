
# -rオプション(デフォルトルール抑制)付で実行してください

include files.mak

.suffixes : .cxx .cpp .hxx .h

KAWAOBJ =  $(KAWASRC:.cpp=.o)

SHIOOBJ =  $(SHIOSRC:.cpp=.o)

CRYPTOBJ = $(CRYPTSRC:.cpp=.o)

CFLAGS = -O2 -I.
LIBS   = 


## TARGETS ##

#ALLTARGET = ./shiori_so/shiori.so ./tool/kosui.exe ./tool/kawari_encode.exe
ALLTARGET = ./tool/kosui.exe ./tool/kawari_encode.exe

all : $(ALLTARGET)


#./shiori_so/shiori.so : ./shiori_so/shiori.o $(SHIOOBJ) $(KAWAOBJ)
#	gpp -o$@ $? $(LIBS) -shared


./tool/kosui.exe : ./tool/kosui.o $(KAWAOBJ)
	gpp -o$@ $? $(LIBS)


./tool/kawari_encode.exe : ./tool/kawari_encode.o $(CRYPTOBJ)
	gpp -o$@ $? $(LIBS)


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

