./kis/kis_base.hxx : ./config.hxx

./kis/kis_config.hxx : ./kis/kis_echo.hxx ./kis/kis_dict.hxx ./kis/kis_test.hxx ./kis/kis_date.hxx ./kis/kis_counter.hxx ./kis/kis_cpiro.hxx ./kis/kis_expr.hxx ./kis/kis_math.hxx ./kis/kis_file.hxx ./kis/kis_escape.hxx ./kis/kis_help.hxx ./kis/kis_urllist.hxx ./kis/kis_substitute.hxx ./kis/kis_saori.hxx

./kis/kis_counter.cxx : ./config.hxx ./kis/kis_counter.hxx ./libkawari/kawari_engine.hxx ./misc/misc.hxx

./kis/kis_counter.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_cpiro.cxx : ./config.hxx ./kis/kis_cpiro.hxx ./libkawari/kawari_engine.hxx

./kis/kis_cpiro.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_date.cxx : ./config.hxx ./kis/kis_date.hxx

./kis/kis_date.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_dict.cxx : ./config.hxx ./kis/kis_dict.hxx ./libkawari/kawari_engine.hxx ./libkawari/kawari_dict.hxx

./kis/kis_dict.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_echo.cxx : ./config.hxx ./kis/kis_echo.hxx

./kis/kis_echo.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_escape.cxx : ./config.hxx ./kis/kis_escape.hxx ./misc/misc.hxx ./misc/i18n.hxx

./kis/kis_escape.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_expr.cxx : ./config.hxx ./libkawari/kawari_engine.hxx ./misc/misc.hxx ./misc/i18n.hxx ./kis/kis_expr.hxx

./kis/kis_expr.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_file.cxx : ./config.hxx ./misc/_dirent.hxx ./kis/kis_file.hxx ./libkawari/kawari_engine.hxx

./kis/kis_file.hxx : ./kis/kis_base.hxx

./kis/kis_help.cxx : ./config.hxx ./kis/kis_help.hxx ./libkawari/kawari_iscript.hxx ./libkawari/kawari.hxx

./kis/kis_help.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_list.cxx : ./libkawari/kawari_dict.hxx ./kis/kis_list.hxx

./kis/kis_list.hxx : ./kis/kis_base.hxx

./kis/kis_math.hxx : ./config.hxx ./misc/misc.hxx ./kis/kis_base.hxx

./kis/kis_saori.cxx : ./config.hxx ./kis/kis_saori.hxx ./libkawari/kawari_engine.hxx ./misc/misc.hxx ./misc/phttp.hxx ./misc/i18n.hxx

./kis/kis_saori.hxx : ./config.hxx ./kis/kis_base.hxx ./misc/phttp.hxx

./kis/kis_substitute.cxx : ./config.hxx ./kis/kis_substitute.hxx ./misc/misc.hxx ./misc/i18n.hxx

./kis/kis_substitute.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_test.cxx : ./config.hxx ./kis/kis_test.hxx

./kis/kis_test.hxx : ./config.hxx ./kis/kis_base.hxx

./kis/kis_urllist.cxx : ./config.hxx ./kis/kis_urllist.hxx

./kis/kis_urllist.hxx : ./config.hxx ./kis/kis_base.hxx

./libkawari/kawari.cxx : ./config.hxx ./libkawari/kawari.hxx ./libkawari/sequence_gen.hxx ./misc/misc.hxx ./misc/i18n.hxx ./misc/mmap.hxx ./saori/saori.hxx

./libkawari/kawari.hxx : ./config.hxx ./libkawari/kawari_engine.hxx ./misc/phttp.hxx ./misc/mmap.hxx

./libkawari/kawari_code.cxx : ./config.hxx ./libkawari/kawari_code.hxx ./libkawari/kawari_dict.hxx ./misc/misc.hxx

./libkawari/kawari_code.hxx : ./config.hxx ./libkawari/kawari_lexer.hxx ./libkawari/kawari_iscript.hxx ./misc/misc.hxx

./libkawari/kawari_crypt.cxx : ./config.hxx ./libkawari/kawari_crypt.hxx ./misc/base64.hxx

./libkawari/kawari_crypt.hxx : ./config.hxx

./libkawari/kawari_dict.cxx : ./config.hxx ./libkawari/kawari_dict.hxx ./misc/misc.hxx

./libkawari/kawari_dict.hxx : ./config.hxx ./libkawari/kawari_code.hxx ./libkawari/wordcollection.hxx

./libkawari/kawari_engine.cxx : ./config.hxx ./libkawari/kawari_engine.hxx ./libkawari/kawari_crypt.hxx ./libkawari/sequence_gen.hxx ./misc/misc.hxx

./libkawari/kawari_engine.hxx : ./config.hxx ./libkawari/kawari_dict.hxx ./libkawari/kawari_iscript.hxx ./libkawari/kawari_matchdict.hxx ./libkawari/kawari.hxx ./saori/saori.hxx

./libkawari/kawari_engine_fwd.hxx :

./libkawari/kawari_iscript.cxx : ./config.hxx ./libkawari/kawari_iscript.hxx ./libkawari/kawari_code.hxx ./libkawari/kawari_engine.hxx ./kis/kis_config.hxx ./kis/kis_config.hxx

./libkawari/kawari_iscript.hxx : ./config.hxx

./libkawari/kawari_lexer.cxx : ./config.hxx ./libkawari/kawari_lexer.hxx ./misc/misc.hxx ./misc/i18n.hxx

./libkawari/kawari_lexer.hxx : ./config.hxx

./libkawari/kawari_matchcode.cxx : ./config.hxx ./libkawari/kawari_matchcode.hxx ./libkawari/kawari_engine.hxx

./libkawari/kawari_matchcode.hxx : ./config.hxx ./libkawari/kawari_engine_fwd.hxx

./libkawari/kawari_matchdict.cxx : ./config.hxx ./libkawari/kawari_matchdict.hxx ./libkawari/kawari_engine.hxx ./misc/misc.hxx

./libkawari/kawari_matchdict.hxx : ./config.hxx ./libkawari/kawari_engine_fwd.hxx ./libkawari/kawari_matchcode.hxx ./libkawari/wordcollection.hxx ./libkawari/kawari_dict.hxx

./libkawari/sequence_gen.cxx : ./config.hxx ./libkawari/sequence_gen.hxx

./libkawari/sequence_gen.hxx : ./config.hxx

./libkawari/wordcollection.hxx : ./config.hxx

./misc/base64.cxx : ./config.hxx ./misc/base64.hxx

./misc/base64.hxx : ./config.hxx

./misc/i18n.cxx : ./misc/i18n.hxx

./misc/i18n.hxx :

./misc/misc.cxx : ./config.hxx ./misc/misc.hxx

./misc/misc.hxx : ./config.hxx ./misc/mt19937ar.hxx

./misc/mmap.hxx :

./misc/mt19937ar.cxx : ./misc/mt19937ar.hxx

./misc/mt19937ar.hxx : ./config.hxx

./misc/phttp.cxx : ./config.hxx ./misc/phttp.hxx

./misc/phttp.hxx : ./config.hxx ./misc/mmap.hxx

./misc/_dirent.cxx : ./kis/_dirent.hxx

./misc/_dirent.hxx :

./saori/saori.cxx : ./config.hxx ./saori/saori.hxx ./misc/i18n.hxx

./saori/saori.hxx : ./config.hxx ./misc/phttp.hxx

./saori/saori_win32.cxx : ./config.hxx ./saori/saori_win32.hxx

./saori/saori_win32.hxx : ./config.hxx

./shiori/shiori_impl.hxx : ./config.hxx ./libkawari/kawari.hxx

./shiori/shiori_interface.cxx : ./config.hxx ./shiori/shiori_interface.hxx ./shiori/shiori_impl.hxx

./shiori/shiori_interface.hxx : ./config.hxx

./shiori_dll/shiori.cxx : ./config.hxx ./shiori/shiori_interface.hxx

./shiori_so/shiori.cxx : ./config.hxx ./shiori/shiori_interface.hxx

./shiori_so/shiori.hxx :

./tool/kawari_encode.cxx : ./libkawari/kawari_crypt.hxx

./tool/kosui_base.hxx : ./config.hxx

./tool/kdb.cxx : ./config.hxx ./tool/kdb.hxx

./tool/kdb.hxx : ./config.hxx ./tool/kosui_base.hxx

./tool/kosui.cxx : ./config.hxx ./tool/kosui_base.hxx ./libkawari/kawari.hxx ./libkawari/kawari_engine.hxx ./tool/kdb.hxx

./tool/kosui_base.hxx : ./config.hxx

./config.hxx :

