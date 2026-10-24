//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// バージョン表記
//
//      Programed by Suikyo.
//
//  2002.04.13  Phase 8.0.0   移植
//  2026.10.18  Phase 9.0.0   Phase 9.0.0 / Core 4.0.0 (マクロはnarrowのまま。wideで使うときは L"" KAWARI_NAME "/" ... のように連結する)
//  2026.10.19  Phase 9.1.0   Phase 9.1.0 / Core 4.1.0
//  2026.10.22  Phase 9.1.1   Phase 9.1.1 / Core 4.1.1 (textload_charset等の追加)
//  2026.10.23  Phase 9.2.0   Phase 9.2.0 / Core 4.2.0 (診断・kosui --check・静的検査・時刻の64bit化)
//
//---------------------------------------------------------------------------
#ifndef KAWARI_VERSION_H
#define KAWARI_VERSION_H
//---------------------------------------------------------------------------
// SHIORIラッパー

#define KAWARI_NAME		"KAWARI.kdt"
#define KAWARI_AUTHOR	"KawariDeveloperTeam"	// 著作者(英数字だけ)
#define KAWARI_MAJOR	"9"		// メジャー番号(ピリオド無し、数字だけ)
#define KAWARI_MINOR	"2"		// マイナー番号(ピリオド無し、数字だけ)
#define KAWARI_SUBMINOR	"0"		// バグフィックス(ピリオド無し、数字だけ)

// Win32のバージョン情報リソース(kawari_version.rc)用。上の番号と揃えること
#define KAWARI_VERSION_RC		9,2,0,0
#define KAWARI_VERSION_RC_STR	"9.2.0.0"

//---------------------------------------------------------------------------
// コア

// 華和梨コア・コードネーム
#define KAWARI_CORE_CODENAME	"K.I.U."

// 華和梨コア・バージョン (過去に遡って振り直し)
//   1.0.0 = Phase 0.4
//   1.1.0 = Phase 0.42
//   2.0.0 = Phase 5.1
//   2.1.0 = Phase 5.4
//   2.2.0 = Phase 6.1
//   2.3.0 = Phase 7.0
//   3.0.0 = Phase 8.0.0
//   3.1.0 = Phase 8.1.0
//   3.2.0 = Phase 8.2.0
//   4.0.0 = Phase 9.0.0 (内部文字列をwchar_tに変更)
//   4.1.0 = Phase 9.1.0 (lambda追加)
//   4.1.1 = Phase 9.1.1 (textload_charset等追加)
//   4.2.0 = Phase 9.2.0 (診断コードと位置、kosui --check、静的検査、時刻の64bit化)
#define KAWARI_CORE_VERSION	"4.2.0"

// 華和梨コア・クレジット(適当に改行で区切って連名)
#define KAWARI_CORE_CREDITS \
	" Original KAWARI Developer Team :\n" \
"    Meister(original works)/Nise-Meister/Sato/Shino/Suikyo\n" \
" Contributers :\n" \
"    ebisawa, MDR, sanori, whoami, ABE, phonohawk, Shiba-yan, PaulLiu\n" \
" Libraries :\n" \
"    Mersenne Twister Library\n" \
"       <http://www.math.keio.ac.jp/~matumoto/mt.html>\n" \

// 華和梨ライセンス (modified BSD(FreeBSD) style license)
#define KAWARI_CORE_LICENSE \
"Copyright (C) 2001-2008 KAWARI Development Team\n" \
"(Meister(original works)/Nise-Meister/Sato/Shino/Suikyo)\n" \
"Contributers (Ebisawa/MDR/Sanori/Whoami/ABE/phonohawk/Shiba-yan/PaulLiu)\n" \
"All rights reserved.\n" \
"\n" \
"Redistribution and use in source and binary forms, with or without\n" \
"modification, are permitted provided that the following conditions are met:\n" \
"\n" \
"1. Redistributions of source code must retain the above copyright notice, this\n" \
"   list of conditions and the following disclaimer.\n" \
"\n" \
"2. Redistributions in binary form must reproduce the above copyright notice,\n" \
"   this list of conditions and the following disclaimer in the documentation\n" \
"   and/or other materials provided with the distribution.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n" \
"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n" \
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n" \
"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n" \
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n" \
"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n" \
"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n" \
"CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n" \
"OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n" \
"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n" \
"\n" \
"License of MT19937 library is following.\n" \
"Redistribution and use in source and binary forms, with or without\n" \
"modification, are permitted provided that the following conditions\n" \
"are met:\n" \
"\n" \
"  1. Redistributions of source code must retain the above copyright\n" \
"     notice, this list of conditions and the following disclaimer.\n" \
"\n" \
"  2. Redistributions in binary form must reproduce the above copyright\n" \
"     notice, this list of conditions and the following disclaimer in the\n" \
"     documentation and/or other materials provided with the distribution.\n" \
"\n" \
"  3. The names of its contributors may not be used to endorse or promote \n" \
"     products derived from this software without specific prior written \n" \
"     permission.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n" \
"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n" \
"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n" \
"A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR\n" \
"CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n" \
"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n" \
"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n" \
"PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n" \
"LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n" \
"NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n" \
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"

#endif // KAWARI_VERSION_H
