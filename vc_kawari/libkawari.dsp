# Microsoft Developer Studio Project File - Name="libkawari" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libkawari - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "libkawari.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "libkawari.mak" CFG="libkawari - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "libkawari - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "libkawari - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libkawari - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /I "../" /I "../libkawari/" /I "../kis/" /I "../shiori/" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libkawari.lib"

!ELSEIF  "$(CFG)" == "libkawari - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "../" /D "_LIB" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "USEKDB" /YX /FD /GZ /c
# SUBTRACT CPP /u
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libkawari.lib"

!ENDIF 

# Begin Target

# Name "libkawari - Win32 Release"
# Name "libkawari - Win32 Debug"
# Begin Source File

SOURCE=..\libkawari\base64.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\base64.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_code.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_code.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_crypt.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_crypt.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_dict.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_dict.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_engine.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_engine.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_engine_base.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_iscript.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_iscript.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_lexer.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_lexer.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_matchcode.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_matchcode.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_matchdict.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\kawari_matchdict.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_base.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_config.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_counter.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_counter.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_cpiro.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_cpiro.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_date.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_date.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_dict.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_dict.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_echo.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_echo.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_escape.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_escape.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_expr.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_expr.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_file.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_file.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_help.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_help.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_math.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_test.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_test.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_urllist.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_urllist.h
# End Source File
# Begin Source File

SOURCE=..\kis\kis_substitute.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\kis_substitute.h
# End Source File
# Begin Source File

SOURCE=..\kis\_dirent.cpp
# End Source File
# Begin Source File

SOURCE=..\kis\_dirent.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\misc.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\misc.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\mt19937.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\mt19937.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\sequence_gen.cpp
# End Source File
# Begin Source File

SOURCE=..\libkawari\sequence_gen.h
# End Source File
# Begin Source File

SOURCE=..\libkawari\wordcollection.h
# End Source File
# End Target
# End Project
