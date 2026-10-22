KISSRC =   ./kis/kis_echo.cpp \
           ./kis/kis_dict.cpp \
           ./kis/kis_test.cpp \
           ./kis/kis_date.cpp \
           ./kis/kis_counter.cpp \
           ./kis/kis_cpiro.cpp \
           ./kis/kis_expr.cpp \
           ./kis/kis_file.cpp \
           ./kis/kis_escape.cpp \
           ./kis/kis_help.cpp \
           ./kis/kis_urllist.cpp \
           ./kis/kis_saori.cpp \
           ./kis/kis_substitute.cpp




CRYPTSRC = ./libkawari/kawari_crypt.cpp \
           ./misc/base64.cpp

KAWASRC =  ./libkawari/kawari.cpp \
           ./libkawari/kawari_engine.cpp \
           ./libkawari/kawari_dict.cpp \
           ./libkawari/kawari_code.cpp \
           ./libkawari/kawari_iscript.cpp \
           ./libkawari/kawari_lexer.cpp \
           ./libkawari/kawari_matchdict.cpp \
           ./libkawari/kawari_matchcode.cpp \
           ./libkawari/sequence_gen.cpp \
           ./misc/misc.cpp \
           ./misc/mt19937ar.cpp \
           ./misc/phttp.cpp \
           ./misc/i18n.cpp \
           ./saori/saori.cpp \
           $(KISSRC) \
           $(CRYPTSRC)

SHIOSRC =  ./shiori/shiori_interface.cpp


