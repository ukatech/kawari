
# -rオプション(デフォルトルール抑制)付で実行してください

include files.mak

.suffixes : .cxx .cpp .hxx .h

KAWAOBJ =  $(KAWASRC:.cpp=.o)

SHIOOBJ =  $(SHIOSRC:.cpp=.o)

CRYPTOBJ = $(CRYPTSRC:.cpp=.o)

CFLAGS = -O2 -I.
LIBS   = 


## TARGETS ##

ALLTARGET = ./shiori_so/shiori.so ./tool/kosui ./tool/kawari_encode

all : $(ALLTARGET)


./shiori_so/shiori.so : ./shiori_so/shiori.o $(SHIOOBJ) $(KAWAOBJ)
	g++ -o$@ $? $(LIBS) -shared


./tool/kosui : ./tool/kosui.o $(KAWAOBJ)
	g++ -o$@ $? $(LIBS)


./tool/kawari_encode : ./tool/kawari_encode.o $(CRYPTOBJ)
	g++ -o$@ $? $(LIBS)


%.o : %.cxx
	g++ $(CFLAGS) $(SHIORIVER) -o$@ -c $<

%.cxx : %.cpp
	ruby sjis2ascii.rb < $< > $@

%.hxx : %.h
	ruby sjis2ascii.rb < $< > $@



cleanxx : 
	-rm ./kis/*.cxx ./libkawari/*.cxx ./shiori_so/*.cxx ./tool/*.cxx
	-rm ./kis/*.hxx ./libkawari/*.hxx ./shiori_so/*.hxx ./tool/*.hxx
	-rm config.hxx

clean : cleanxx
	-rm $(KAWAOBJ) $(SHIOOBJ) $(CRYPTOBJ) ./shiori_so/shiori.o ./tool/kosui.o ./tool/kawari_encode.o 

cleanall : clean
	-rm $(ALLTARGET)

depend : clean
	ruby ./makedepend.rb --with-sjis2ascii > depend.mak


include depend.mak

