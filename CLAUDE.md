# CLAUDE.md

華和梨 (KAWARI) — 伺か用の SHIORI (擬似AI) モジュール。C++ 製、Phase 9.3.2 (Core 4.3.2)。
CI は無い。最低限のテストは `build/src/test/` (下記「テスト」)。
旧 ACP (SJIS の std::string) 版のコードは `v8acp` ブランチに保存してある。

## 内部文字列 (Phase 9.0.0 以降)

- 内部のテキストはすべて `std::wstring` / `wchar_t`。文字数は wchar_t 1 要素 = 1 文字 (Windows のサロゲートペアは 2 文字)。`std::string` は外部とのバイト列 (ファイルの生バイト、SHIORI/SAORI の生リクエスト、暗号化の中間データ) にだけ使う。
- 変換は `misc/charset.h` (`SJIStoW` / `UTF8toW` / `ACPtoW` / `MBtoW(str, cs)` / `DetectCharset` / `CharsetFromName` 等。Windows は Win32 API、POSIX は iconv)。ファイル I/O とパスは `misc/wfile.h` (`kawari_wfopen` / `ReadTextFile` / `WriteTextFile` / `ListDirectory` / `PathStat` / `GetEnvironmentW`)。旧 `ctow` / `wtoc` / `iskanji1st` / `misc/_dirent.*` は廃止 (`misc/l10n.h` は互換スタブ)。
- ロガーは `std::wostream`。バイト出力先へは `TWideOStream` (`ostream*` か `FILE*` + charset、CRLF 変換可) を挟む。
- 文字コードの規則 (変更時は `../kawari-docs` の `other/changelog.md` と整合させる): 辞書は BOM か UTF-8 として正当なら UTF-8、でなければ SJIS。暗号化行はファイルに BOM があれば UTF-8、なければ SJIS。save/savecrypt は UTF-8 BOM あり、textsave/textappend とログファイルは UTF-8 BOM なし、コンソールは ACP。SHIORI はリクエストの Charset (無ければ SJIS) で応答も返す。`loadu` (UTF-8 パス) もエクスポート。`textload_charset` / `textsave_charset` / `textappend_charset` は文字コードを引数で取り (`CharsetFromNameExt`)、Shift_JIS / UTF-8 / EUC-JP / ISO-2022-JP / EUC-KR / GB2312 を扱う (Windows のコードページは 20932 / 50220 / 949 / 936。51932 は `MultiByteToWideChar` が非対応)。SAORI は satoriya 互換 (UTF-8 で開始、GET Version に Charset が無ければ以後 SJIS、応答の Charset を次回に引き継ぐ。`loadu` があれば UTF-8 パス)。
- Win32 API は W 版 (`UNICODE` / `_UNICODE`、`WINVER` / `_WIN32_WINNT` = 0x0500 を `config.h` と `gcc-mach.mak` で定義)。ただし i686-posix の MinGW 8.1.0 は winpthreads 由来で `AddVectoredExceptionHandler` 等 (XP 以降) を import するので、生成物は実際には Windows 2000 では動かない (8.2.9 のリリース版も同じ)。
- コード中の文字列リテラルに非 ASCII を直書きしない (`L"\x3042"` 等を使う)。

## 文字コード (最重要)

- `build/src` 配下の `.cpp` / `.h` / `.rb` は **EUC-JP**、改行は LF (`makedepend.rb` も EUC-JP に揃えてある)。例外として `build/src/uka_regexp/` は git サブモジュール (ASCII のみ、触らない)。コメントやリテラルに日本語を含む。
- 標準の Read / Edit / Write は UTF-8 前提のため EUC-JP を壊す。`.claude/euc-tools/hook.py` (PreToolUse フック) が弾くので、**`euc_read` / `euc_edit` / `euc_write` / `euc_sed` / `euc_grep` を使う** (`.mcp.json` で MCP サーバとして登録済み)。
  - `sed` や PowerShell の `-replace`、`od` / `xxd` 等はフックで禁止。
  - 純 ASCII の `.cpp` / `.h` に日本語を足す場合も `euc_edit` を使う (標準 Edit だと UTF-8 が混入する)。
- 例外: `gcc.mak` / `files.mak` / `kawari_rc_sjis_encoded.h` などは UTF-8 (実質 ASCII)。マニュアルは `../kawari-docs` に移した (旧 `document/` は廃止)。
- `build/BUILD.TXT` は **CP932 (Shift-JIS)**。`euc_*` ツールでは編集できず、標準 Edit / Write も拒否される。変更するなら cp932 を明示して書き換えること。`README.md` は UTF-8 (9.0.0 β1 で旧 `README.jp.md` (CP932) から変換・改名)。
- `libkawari/kawari_rc.cpp` が include する `kawari_rc_sjis_encoded.h` は、`kawari_rc.sjis` を `sjis2ascii.rb` で八進エスケープ化した生成物と思われる。直接手編集せず、元の `.sjis` 側を直す。

## ビルド

ビルド手順の一次資料は `build/BUILD.TXT` (CP932)。**作業ディレクトリは `build/src`**。

```
cd build/src
make -f gcc.mak depend    # ruby で depend.mak を再生成 (先に clean も走る)
make -f gcc.mak
```

- ターゲット機種は `gcc.mak` 冒頭の `MACH_TYPE` (`cygwin|mingw|linux|freebsd|darwin|js`、デフォルト `mingw`) で決まる。機種ごとの差分は `gcc-mach.mak`。FreeBSD は `gmake`。
- 成果物は `build/mach/<MACH_TYPE>/` に出る (`shiori` 共有ライブラリ、`kosui`、`kawari_encode` / `kawari_encode2` / `kawari_decode2`)。リポジトリ直下の `bin/` はコミット済みのリリース用バイナリで、ビルドしても更新されない。
- `all` は最後に `upx` ターゲットを実行するが、`UPX` は既定で空 (圧縮しない)。UPX 圧縮はマルウェア誤検知の元になるため外した。使うなら `make -f gcc.mak UPX=upx`。
- `CXX` は `ifndef CXX` で決まるため、**環境変数 `CXX` が設定されているとそれが優先される**。存在しないパスが入っていると全コンパイルが失敗するので、`CXX=g++` を明示するのが安全。
- Windows では `make` ではなく `mingw32-make` (または `gmake`) を使う。MinGW に同梱されている。
- **64bit 版 (x86_64) もビルドできる** (g++ 13.2 / x86_64-w64-mingw32 と MSVC 14.51 / x64 で、5 ターゲットの生成と kosui・check・shiori・baseware の全スイートの通過を確認。shiori / baseware は DLL と同じビット数の gcc を `-CC` に渡す)。以前は `saori/saori_native.cpp` と `tool/kosui_dsstp.cpp` が 32bit 前提のポインタ⇔整数キャストで通らなかった。直した要点: `SAORI_HANDLE` は `size_t` (LLP64 の `unsigned long` は 32bit で `HMODULE` が切れる)、`SendMessageTimeout` の戻りは `DWORD_PTR`、HWND と整数の変換は `(HWND)(INT_PTR)(LONG)` で符号拡張。ホストが 32bit の SSP なら 32bit 版の DLL が要るので、リリース版 `bin/shiori.dll` は引き続き i386 (32bit)。64bit 版はビルドして試しただけで、実ゴーストでは未検証。
- **MSVC / VC6 でもビルドできる**: `python build/src/msvc_build.py x64|x86|vc6` (Windows、Python 3)。`cl.exe` を直接呼ぶスクリプトで、ソースが EUC-JP なので、変換したコピー (最新 MSVC は UTF-8 + `/utf-8`、VC6 は CP932 + CRLF) を `build/mach/<msvc_x64|msvc_x86|vc6>/work` に作ってコンパイルする (作業ツリーは書き換えない)。成果物は `build/mach/<ターゲット>/` に出る (`.gitignore` 済み)。Visual Studio は vswhere か `--vs-dir`、VC6 は `--vc6-dir` / 環境変数 `VC6DIR` (既定 `C:\Program Files (x86)\Microsoft Visual Studio\VC98`) で探す。`vc_kawari/` の IDE プロジェクト (`.dsp` / `.sln` / `.vcproj`) は古くてソースが追従しておらず、スクリプトに置き換えて削除した。ソースの一覧は `files.mak` を読むので、ファイルを足したら `files.mak` を直すだけでよい。FASTALLOC (`misc/fastalloc.cpp`) は MinGW 専用で入れない。静的 CRT (`/MT`)、リンクは `kernel32 user32 shell32 advapi32` だけで通る。バージョン情報リソース (`kawari_version.rc`) は `rc.exe` で埋め込む。最新 MSVC 14.51 の x64 / x86 と VC6 で、5 ターゲットの生成と kosui・check・shiori・baseware の全スイートの通過を確認 。警告レベルは既定の `/W3` で、`--wlevel 4` で `/W4` にでき、`--warnings` で警告の本文を出す。x64 の `/W3` の警告 (size_t → unsigned int / int / long の縮小、C4267・C4244) は、サイズが 4GB 未満なので `static_cast` で明示して 0 件にした (x86 も 0 件、VC6 は STL 由来の C4503 が 1 件)。**新しく `.size()` / `.length()` を int や unsigned int に入れるときは、`static_cast` を付けること** (VC6 でも通る書き方)。`/W4` の残りは、実害を確かめて放置したもの: C4100 (未使用の引数。仮想関数の空実装と、`so_free` のように ABI が決まっているもの)、C4456・C4457・C4458 (`kawari_compiler.cpp` の入れ子の `t` / `code` / `stmtline`、`kawari_engine.cpp` の `entry`、`kawari_shiori.cpp` の `instance`。内側の変数を内側で使い切っていて、外側を取り違えている箇所は無い)、C4505 (`kawari_decode2.cpp` の使っていない `IsUTF8Text`)、C4702 (`uka_regexp` はサブモジュールなので触らない)。
  - VC6 向けの書き方: `long long` / `ull` は使わず `config.h` の `kawari_int64` / `kawari_uint64`。`wchar_t` は `unsigned short` の typedef なので、**`wostream` に `wchar_t` の 1 文字を `<<` で流すと数値になる** (`L"E"` のように文字列で流す)。狭い文字列リテラルと `L""` の連結 (`L"" KAWARI_AUTHOR`) は不可 (`AtoW` を使う)。テンプレート内の `typename X::value_type(a, b)` と、既定引数の `basic_string<T>::npos` は通らない。付属 SDK に `DWORD_PTR` / `GetWindowLongPtrW` 等が無いので `dsstp.cpp` / `kosui_dsstp.cpp` に代用の定義を置いてある。
  - VC6 の CRT には `_time64` が無く `time_t` が 32bit だが、`date` / `mktime` は Win32 では CRT を使わない (`misc/epochtime.h`。下の「時刻」) ので、2038 年以降も扱え、kosui スイートは全件通る (9.3.1 までは `date` / `mktime` の 4 件が落ちた)。
  - `config.h` の `for` のマクロ (スコープ対策) は `_MSC_VER < 1300` だけ。新しい MSVC の STL はキーワードのマクロ化を `#error` で拒否する。
- **32bit 版 MinGW-w64 (i686) ならビルドが通る** (`shiori.dll` / `kosui.exe` / `kawari_*.exe` の全ターゲットが i386 で生成されることを確認。`UPX=` で upx 無効)。**リリース用は msvcrt 版を使うこと**: UCRT 版 (mingw-builds 16.1.0 等、`g++ -v` の sysroot に `ucrt` と出る) だと実行ファイルが `api-ms-win-crt-*.dll` に依存し、UCRT の無い旧い OS (Windows 7 以前など) で動かない。`-mcrtdll=msvcrt` を付けても、UCRT 版ツールチェインでは `libmsvcrt.a` が `libucrt.a` と同一なので効かない。msvcrt 版なら依存は `KERNEL32.dll` / `msvcrt.dll` (kosui は `USER32.dll` も) だけになる。
  - choco なら管理者シェルで `choco install mingw --version 8.1.0 --x86 -y` (g++ 8.1.0 i686-posix-dwarf、msvcrt 版)。**バージョン指定なしだと最新の UCRT 版が入る**。実体は `C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw32\bin`。
  - 別の g++ (64bit 版、Strawberry Perl の `C:\Strawberry\c\bin` 等) が PATH にあると、こちらを **先頭** に置かないと cc1plus が 64bit の DLL を拾い、何も表示せず `0xC000007B` (終了コード 1/127) で落ちる:
    ```
    export PATH=/c/ProgramData/chocolatey/lib/mingw/tools/install/mingw32/bin:$PATH
    cd build/src && gmake -f gcc.mak CXX=g++ UPX=
    ```
  - 確認は `objdump -p shiori.dll | grep "DLL Name"`。`api-ms-win-crt-*` が出たら UCRT 版でビルドしている。
  ビルドは `build/src` に `.o` と `build/mach/mingw` に成果物を作るので、作業ツリーを汚したくなければ `build/` をコピーして実行する。`"SHIORI/"SHIORIVER" ..."` の文字列連結が C++11 以降で警告になるが無害。
- 最適化は `-O2`。かつては `-O1` 固定だった (README.md の 8.2.5 の項: 辞書に半角中点があると異常動作。原因は UPX か gcc 最適化か不明のまま) が、UPX を外した上で `-O2` でも既存テスト 19 件と半角中点入り辞書の動作を確認した。
- `FASTALLOC` (既定 yes、`MACH_TYPE=mingw` のみ): `misc/fastalloc.cpp` がグローバルの `operator new/delete` を小オブジェクト用のフリーリストに差し替える。インタプリタは小さな確保を大量に繰り返すため、実ゴースト (1000 曲プレイリスト) で処理時間が 1.4〜1.9 倍速くなった。確保済みのメモリは OS に返さない。外すなら `gmake -f gcc.mak FASTALLOC=no`。
- バージョン情報リソース (ファイルのプロパティに出る版): `build/src/kawari_version.rc` を `windres` で `shiori.dll` / `kosui.exe` / `kawari_*.exe` に埋め込む (`MACH_TYPE=mingw` のみ)。版を上げるときは `libkawari/kawari_version.h` の `KAWARI_SUBMINOR`・`KAWARI_CORE_VERSION` に加えて、同ファイルの `KAWARI_VERSION_RC` / `KAWARI_VERSION_RC_STR` も揃えること。
- リリース手順: 32bit MinGW でビルドした 5 ファイル (`shiori.dll` / `kosui.exe` / `kawari_encode.exe` / `kawari_encode2.exe` / `kawari_decode2.exe`) を `bin/` にコピーして版更新と同じコミットに入れ、`git push` 後に GitHub Release (タグ `v8.2.9` の形式。β版は `v9.0.0-beta1` で prerelease にする) を作る。リリースには、`bin/` の 5 ファイルと `license.txt` (= リポジトリ直下の `LICENSE`)、`readme.1st` (= `README.md`、UTF-8 のまま) をフラットに zip にして添付する。zip 名は `kawari-8.2.9.zip` / `kawari-9.0.0-beta1.zip` の形式。コード上の版 (`kawari_version.h`) には β の区別が無い (SHIORI/2.x の GET Version が版の数字を連結して返すため、`KAWARI_SUBMINOR` に文字を足さないこと)。
- `depend.mak` は `foo.cpp : ヘッダ…` の形で、**ヘッダだけ変えても `.o` は再ビルドされない** (`touch` で確認済み)。ヘッダを触ったら `.o` を消してからビルドし直すこと。
- 正規表現エンジン [uka_regexp](https://github.com/ukatech/uka_regexp) (Unlicense、2 ファイル、C++98) は git サブモジュール `build/src/uka_regexp`。clone は `--recursive` か、後から `git submodule update --init` (`posix-build` のチェックアウトは `submodules: true`)。`files.mak` の `CORESRC` に `uka_regexp/uka_regexp.cpp` を入れてある。`makedepend.rb` は `"uka_regexp.h"` のような同じフォルダのヘッダの `#include` を、そのファイルのフォルダ基準で解決する。
- ソースファイルを追加・削除したら `build/src/files.mak` を編集し、`make depend` で `depend.mak` を作り直す。MSVC / VC6 のビルドスクリプト (`msvc_build.py`) も `files.mak` を読むので、追加の作業は要らない。
- 任意機能は `gcc.mak` のフラグで切り替える: `SAORI_NATIVE` (デフォルト yes) / `SAORI_PYTHON` / `SAORI_JAVA` / `SHIORI_PYTHON` / `STLport`。Python 関連の記述は Python 2 構文 (`print sys.version[:3]`)。
- mingw / cygwin で gcc3 系の `wstring` が通らない場合は STLport を使う (`STLport = yes`)。
- `MACH_TYPE = js` / `wasm` は `em++` (Emscripten) でビルドし、`em-pre.js` / `em-post.js` で包んだ `newKawari(onRuntimeInitialized)` を `module.exports` する (upstream の PR #7・#8 を取り込んだもの。旧 `Kawari` コンストラクタは廃止)。ランタイムの準備ができるとコールバックに `{ Module, FS, PATH, ERRNO_CODES, NODEFS, IDBFS, WORKERFS }` が渡る。公開する関数は `gcc-mach.mak` の `EXPORTED_FUNCTIONS`、JS 側へ露出するオブジェクトは `em-pre.js` で管理している。`wasm` は成果物を `build/mach/wasm` に出す (取り込みで `.keep` を追加)。Emscripten が無いので js / wasm のビルドは未検証。
- bcc32 でビルドするにはソースを SJIS に変換する必要がある (`build/conv.sh -c sjis src`、Ruby の `kconv` 前提で古い)。**ファイルをその場で書き換える**ので、作業ツリーではなくコピーに対して実行すること。

## テスト

ビルド後に `powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1` (詳細は `build/src/test/README.md`)。`kosui.exe` に 1 行ずつ評価させる kosui スイートと、`kosui --check` の診断・終了コードを 1 フォルダ 1 ケースで見る check スイート (`check/<名前>/{kawarirc.kis,args,expect,exit}`)、`shiori.dll` を直接 `load` / `request` / `unload` する shiori スイート、本体の代役の窓を使う baseware スイート (Windows のみ)、`multi_*` を試す multi スイート (`harness/multi_harness.c` と `multi/`。下の「マルチインスタンス」) があり、ケースは `kosui/cases.txt` / `shiori/cases.txt` / `check/` に足す。

- POSIX (linux / darwin) では `python3 build/src/test/run_posix.py --bin-dir build/mach/linux` (同じ cases.txt を使い、SHIORI は ctypes で直接呼ぶ。UTF-8 の往復も見る)。GitHub Actions の `posix-build` (手動実行) がこれを走らせる。
- shiori スイートは C のハーネスをその場でビルドするので、**DLL と同じ 32bit の gcc** が `PATH` の先頭に必要 (ビット数が違うとランナーが止まる)。
- テスト用のソース・辞書・ケースは ASCII のみ (本体の EUC-JP と混ぜない)。
- ハーネスは環境変数 `HARNESS_LOADU` があると `load` の代わりに `loadu` (UTF-8 パス) を呼ぶ。`HARNESS_DIR_W` (UTF-16 の環境変数) と併用すると ACP 外の文字を含むフォルダも試せる (DLL もそのフォルダから読む)。`HARNESS_RELOAD=<n>` で `load`/`unload` の繰り返し、`<出力ファイル>` に `-` で標準入出力の対話モード。
- 実在のゴーストの互換テストは `python build/src/test/ghost_compat.py --ghost <ghost>/ghost/master` (基準の DLL と `bin/shiori.dll` に同じ SSP 風のリクエスト列を流して応答・SAORI への送信・セーブデータの読み込みを比べる)。詳細は `build/src/test/README.md`。**実ゴーストの辞書は SAORI 経由で本物の exe を起動する**ので、ハーネスに直接読ませず必ずこのドライバ (コピーの exe を消し、SAORI を `harness/stub_saori.c` に差し替える) を通すこと。
- 9.0.0 の互換性の確認結果 (kakera + OpenKEEPS、8.2.8 との比較): 応答・SAORI への送信・UTF-8・日本語/ACP 外のパス・`load`/`unload` の繰り返しは差分なし。意図した差は、`GET Version SHIORI/2.6` の応答に `Charset` が付くことと、SAORI の `GET Version` を UTF-8 で問い合わせること。新版が書いた `save`/`savecrypt` は旧版で読めない (UTF-8 BOM)。
- kosui を手で使うときの注意: 入力の最後に `exit` が無くても EOF で終わる (9.1.1 までは無限ループした)。非 ASCII を含む出力は未検証。KIS の要素数は `size <エントリ>`。辞書のカンマは単語の区切りで、複数単語のエントリはランダムに 1 つが選ばれる (期待値が揺れる原因)。
- .NET Framework の `Process` は標準入力の先頭に UTF-8 BOM を付ける。kosui のような stdin を読むツールには、`cmd /c "exe < file"` で渡すこと (`run.ps1` はそうしている)。

## 本体との連携 (Direct SSTP、9.0.0 β2 以降)

- `misc/dsstp.*` が本体との通信 (`TBasewareLink`、エンジンが 1 つ持つ)。Windows は `WM_COPYDATA` (dwData=9801) で応答用の隠し窓を作って送る、POSIX は ninix の FMO から UNIX ソケットを探す、js は常に送れない。送り先は `NOTIFY hwnd` の Reference0 (`kawari_shiori.cpp`)。
- KIS は `kis/kis_sstp.*` (`getproperty` / `setproperty` / `executesstp` / `executesstpx`)。結果は `System.SSTP.ResponseCode` / `System.SSTP.Data`。
- SAORI over DirectSSTP は `saori/saori_dsstp.*`。`TModuleFactoryDirectSSTP` が `TModuleFactoryMaster` の先頭にあり、DLL の実体が無い `internal_*` と互換名 (houchi など) のパスを引き受ける。`TModule::RequestMessage` はシリアライズ前のメッセージを直接受けるための口 (空の `ArgumentN` を位置として残すため)。未登録の `internal_*` は `TSaoriPark::GetModule` が `ListModules` を問い合わせて自動登録する。
- リクエストの SecurityLevel が local 以外なら `TBasewareLink::IsAllowed` が偽になり、上のどれも送らない (Direct SSTP は常に Local 扱いになるため)。
- テストは `-Suite baseware` (`test/baseware/`)。ハーネスの `HARNESS_BASEWARE` が本体の代役の窓 (スタブ baseware) を同じプロセスに作り、リクエスト中の `%HWND%` を窓ハンドルに置き換える。POSIX 側 (ninix) と実際の SSP は未検証。

## マルチインスタンス (`multi_*`、Phase 9.3.0)

- `shiori/shiori.cpp` の `multi_load` / `multi_loadu` / `multi_unload` / `multi_request` (ukadoc-spec の `spec/dll.md` の任意の関数。SSP は使わない。js / wasm は `gcc-mach.mak` の `EXPORTED_FUNCTIONS` にも載せた)。id は `TKawariShioriFactory` のハンドル (添字+1、1 以上)。失敗は -1 / FALSE / NULL で、`h` は成功でも失敗でも解放する。YAYA の `CI_check_failed` / `Set_loghandler` / `logsend` は作っていない。
- 従来の `load` / `request` / `unload` のインスタンスは、ファクトリの `list` とは別の専用メンバ (`legacy`) に置く。**どの id でもそこには届かない**ことが構造で決まっている (`list` の中に入れない)。ファクトリは、従来と multi の全てが無くなったときだけ破棄する (`unload()` も `multi_unload` も `DisposeFactoryIfEmpty`)。so_* (`shiori_object.cpp`) は `list` だけを使う。
- **プロセスで共有する可変状態を足さないこと**。乱数 (`TKawariEngine::Random` / `SRandom`)・ラムダの通し番号・`regex_*` の入れ子の深さ・安全モードは、エンジンのメンバにした。大域変数や関数内 `static` を足すと、インスタンスの間で状態が漏れる。`nm` で `.bss` / `.data` の名前付きシンボルを見ると洗い出せる。
- まだ共有しているもの (意図して残した): `rccharset` の診断メッセージの言語 (`kawari_rc.cpp` の `ResourceManager`。`RC.S(...)` の呼び出しが約 40 か所で、エンジンに届かない所 (`kawari_lexer.h`、`kawari_ns.h` など) もある)、同じ SAORI の DLL を複数のインスタンスが登録したとき (OS の `LoadLibrary` の参照カウントで 1 つになるが、`TUniqueModuleFactory` はエンジンごとなので、SAORI の `load` / `unload` はインスタンスごとに呼ばれる)、ファクトリの排他制御 (無い。YAYA も同じ。1 スレッドから順に呼ぶ前提)。
- テスト: `-Suite multi` (45 件)。同じ種で `srand` した 3 つのインスタンス (従来 + multi 2 つ) の乱数列が、1 つだけで動かしたときと同じになること (これは乱数を大域に戻すと落ちる)、無効な id、従来側を先に / multi 側を先に終了、`OnUnload` が全インスタンスで動くこと、全部終了した後の再 load。
- 変更の確認: 全スイート + `ghost_compat7.py` (7 互換モードの実ゴースト) で、単一インスタンスの応答が変わらないことを見た。MSVC x64 と VC6 のビルドも通る。`TKawariEngine` のヘッダを変えたので、`.o` は全部消してビルドし直すこと (`tool/` と `shiori/` も)。

## 診断 (エラー・警告) と `kosui --check` (Phase 9.2.0)

- 書式は 1 つ: `ファイル(行): error K-E0028: 本文` に、呼び出し連鎖の `    in ${entry} at ファイル(行)` が続く。診断の出力は `logger.Diag(LOG_ERROR, kawari_log::E_XXX) << ...;` (`TDiagStream`。末尾の `endl` は不要)。コンパイラの診断は `lexer->error(ERR_ID, 後ろに足す文字列)`。**`GetStream()` に直接書くのは INFO・ダンプ・`echo` だけ**にする (コードも位置も付かず、`--check` の出力に混ざらない)。
- 診断コードは `kawari_log.h` の `enum DiagCode` (`K-E<番号>` / `K-W<番号>` の番号部分)。**追記専用で、番号を変えない・再利用しない**。リソース ID との対応は `kawari_diag.cpp` の `NumberOf()`。メッセージ文を足すときは `kawari_rc.sjis` と `kawari_rc.cpp` の英語表と `kawari_rc.h` の enum を同じ順で更新し、`ruby sjis2ascii.rb libkawari/kawari_rc.sjis | tr -d '\r' > libkawari/kawari_rc_sjis_encoded.h` で再生成する (現状のファイルがこの手順でバイト単位で再現できることを確認済み)。
- 位置: 単語は内容が同じなら 1 つに共有される (`TWordCollection::Insert`) ので、位置は中間コードのノードではなく **(エントリ, 単語 ID)** に付けて `TNS_KawariDictionary::SourceMap()` (`TSourceMap`, `kawari_diag.h`) が持つ。辞書ファイルの読み込み (`LoadKawariDictStream`) と `function` / `lambda` だけが記録する。単語を GC で消すときは必ず `EraseWord` する (ID が再利用されるため。`test/check/id_reuse` が見張っている)。
- 実行フレーム: 単語を実行する箇所は `TKawariVM::TWordFrame` で囲む (`TKVMCodePVW::Run`、`TKVMSetCode_base::Run`、`TKVMCodeEntryIndex::Run`、ユーザ関数、`foreach`、`TKawariEngine::Parse(TWordID)`)。`=kis` ゾーンは `TZoneFrame` で、文ごとの行は `TKVMCodeInlineScript::lines`。**新しく「辞書の単語を実行する」コードを足したらフレームを積むこと**。フレームは loglevel に依存しない (途中で変わっても積み降ろしが崩れないため)。
- 安全モード `TKawariEngine::SetSandbox(true)` (状態はエンジンごと。Phase 9.3.0 で、プロセス大域の `SetFileWritesDisabled` をやめた): ファイル書き込み (書く経路 3 か所が `IsSandbox()` を見て止める。`save` / `savecrypt` = `TKawariEngine::SaveKawariDict`、`textsave` 系 = `kis/kis_file.cpp`、`logfile` = `kis/kis_system.cpp`。前の 2 つは書かずに成功を返す。`misc/wfile.cpp` の `WriteWholeFile` / `kawari_wfopen` 自体は止めないので、**ファイルを書く経路を足すときは、そこで `IsSandbox()` を見ること**)、SAORI の実体 (`TSaoriPark::RegisterModule` が実体を作らない。登録済みと `internal_*` の呼び出しは `Result: 0`)、本体との通信 (`Baseware->SetAllowed(false)`)、`logfile` を止める。**外へ影響する新しい経路 (プロセス起動、ネットワーク、ファイル書き込み) を足すときは、ここで止まることを確かめる**。辞書を読む前に呼ぶこと (`TKawariKosuiAdapter` の検査用コンストラクタがそうしている)。
- `kosui --check [--format text|json] [--werror] [--decl] [--no-lint] [--eval 式]... [--seed n] [rcfile]`: 終了コード 0 = なし / 1 = 警告のみ / 2 = エラー / 3 = 実行できない。出力は診断だけ (`echo` と `loglevel` / `logfile` は出力に影響しない)。JSON は常に UTF-8。`=kis` は実際に実行するので、終わらない繰り返しがあると終わらない。`AttachTarget` は通さない (稼動中のゴーストに繋がってしまう)。
- **時刻 (`date` / `mktime`、`kis/kis_date.cpp`)** は 64bit の通算秒数 (`misc` の `Int64ToString` / `StringToInt64`)。Win32 (`_WIN32`。MinGW・MSVC・VC6、32bit も 64bit も。Cygwin は `time_t` が 64bit なので対象外) は、CRT の `localtime` / `mktime` を **使わない** (Phase 9.3.2)。通算秒数と年月日時分秒の変換は `misc/epochtime.h` の 64bit の暦計算 (`DaysFromCivil` / `CivilFromDays`。グレゴリオ暦の外挿で、1970 年より前も 3000 年より後も同じ規則。扱える範囲は ±3e16 秒 = `Limit()`) で自前で行い、OS には「その UTC 時刻でのローカルタイムの UTC オフセット」だけを尋ねる (`FileTimeToSystemTime` + `SystemTimeToTzSpecificLocalTimeEx` / `SystemTimeToTzSpecificLocalTime` を `GetProcAddress` で実行時に探し、無ければ `GetTimeZoneInformation` のバイアス。yaya-shiori と同じ方式)。夏時間かどうかはその年の 1 月 1 日と 7 月 1 日のオフセットの小さいほうを標準時として比べる。範囲を超えると `date` は空文字、`mktime` は -1。環境変数 `TZ` は見ない (OS のタイムゾーン設定に従う)。`mktime` の夏時間の切り替えでは、存在しない時刻 (春の 2:30) は夏時間側、2 回ある時刻 (秋の 1:30) は先の (夏時間の) ほうになる。`mktime` の年は 1900 未満を 1900 に直す (従来どおり)。POSIX は従来どおり `localtime` / `mktime` (OS の `time_t` 次第。`time_t` が 32bit の環境では 2038 年以降の `date` が空文字)。
  - `epochtime.h` はヘッダだけで、状態を持たない。`OffsetFunc` を渡す `LocalTMWith` / `MakeLocalWith` は OS に依存せず、夏時間の検証に使える (検証は、米国太平洋時間の規則 (2007 年以降) を渡して Python の `zoneinfo` と 4 万件突き合わせ、UTC の暦は Python の `datetime` (年 1〜9999) と 2.5 万件、OS のオフセットは CRT の `_localtime64` と 2 万件突き合わせた。ハーネスは使い捨てで、リポジトリには入れていない)。**夏時間のある地域の実機での確認はしていない** (この PC は JST)。
  - 時刻に関わる KIS は `date` / `mktime` だけ (ほかに `time(NULL)` を使うのは乱数の種で、切り詰めても実害なし)。
- 式 (`$[ ]`、`TValue`) の整数は 64bit (`config.h` の `kawari_int64`。VC6 は `__int64`、それ以外は `long long`)。`+` `-` `*` `**` と単項 `-` は unsigned で計算して 64bit で回り込ませる (符号付きのオーバーフローは未定義動作なので、**足すときも `(kawari_int64)((kawari_uint64)a op (kawari_uint64)b)` の形にすること**)。`TValue(int)` は無く、`TValue(0)` のように int リテラルを渡すと `bool` と曖昧になるので `TValue((kawari_int64)0)` と書く。式の外の `StringToInt` / `IntToString` (`loop` / `substr` / `rand` / エントリの添字など、libkawari・kis・saori・shiori に約 100 か所) は 32bit のまま。7 互換モードの `expr` と `test` の数値演算 (`kis/kis7.cpp` の `Atoi7_64`。別実装) も 64bit (9.3.2 から。7.5.0 は 32bit なので、`kosui7` の「extensions」より前の期待値は 32bit に収まる値だけ。範囲を超える値の期待値は「extensions」に手で書く)。添字などの `Atoi7` (32bit) は残してある。
- `make depend` (ruby) が書く `depend.mak` は CRLF なので、`tr -d '\r'` で LF に戻してからコミットする。
- Bash ツールで python のコードをヒアドキュメントで渡すと `\\` が潰れる。Write ツールでファイルにして実行する。
- 静的 Lint (`libkawari/kawari_lint.*`、`--check` が既定で実行。`--no-lint` で止める): 辞書の全単語と、読み込み時に残しておいた `=kis` ゾーン (`TKawariEngine::SetKeepZones(true)`、検査用コンストラクタが呼ぶ。通常は従来通り実行後に `delete`) の中間コードの木を `TKVMCode_base::GetChildren()` で歩く。コードは **K-W2001** (どこにも定義されていないエントリの参照) と **K-W2002** (組み込み・`function`・`System.Function.*` のどれでもないコマンド名)。実行時のコストは無い (`--check` からだけ呼ぶ)。
  - **子を持つ中間コードのクラスを足したら `GetChildren()` を上書きすること**。漏れると検査が黙って素通りする (`test/check/lint_undefined` は `TKVMCodeEntryCall` の上書きを外すと落ちる)。
  - 1 周目で「定義されているもの」を集め、2 周目で検査する。定義済み扱いは、辞書に存在するエントリ、文の引数にリテラルで書かれた名前、文字列に埋め込まれた `$(...)` の中の単語、先頭だけリテラルで後ろを組み立てる名前 (`launch_a_${n}` の `launch_a_` を接頭辞として覚える)、`System.`・`@`。`${...}` で組み立てた名前は調べない。kakera (OpenKEEPS 入り) で 82 件 → 58 件に絞った。残りは本物の書き間違い (`serstr`、定義の無い `RandomTalkOn`、どこでも設定されない `status`) が多く、偽陽性があるとすれば動的に作られる名前。
  - 警告は (ファイル, 行) の順にそろえて出す。複数行の単語は先頭行の位置になる (単語の中の行は中間コードに無い)。`=kis` ゾーンの文は `lines` から個別の行が付く。
  - `kawari_lint.h` の `TKawariLint` は `Check()` (静的関数) 経由で使う。呼び出し側のスタックにクラスの実体を置くと、`depend.mak` がヘッダの変更を拾わないために `.o` の大きさが食い違ってクラッシュした。
  - コンパイラで拾うもの (木には残らない): **K-W2003** (`regex_*` の引数に引用符のない `( )`。素の括弧は `compileBlock` が中身だけを残して消すので、`(a)` と `a` が同じ木になる)。`TKawariCompiler::SetLintNotes(TLintNotes *)` で受け取り先を付けたときだけ、`compileWord` が「スクリプト中にブロックを読んだ」旗 (`lastwordblock`) を立て、`compileScriptStatement` が所見を `TLintNotes` に積む。受け取り先は `TKawariEngine` が持ち (`LoadKawariDictStream` が `KeepZones` のときだけ付ける)、通常の読み込みでは NULL で何もしない。`compileWord` の旗は終わるときに設定するので、入れ子の単語の旗は外側が上書きする。木で拾うもの: **K-W2004** (`regex_replace(_first)` の置換文字列に引用符のない `${N}` = `TKVMCodeHistoryCall`)。
  - 抑制は `# lint:disable-next-line=K-W2001, K-W2003` (次の行だけ。ユーザーが決めた)。`TKawariPreProcessor` がコメント行を捨てる所で `TLintNotes::ReadComment` に渡し、`TKawariLint::Run` が出力前に `IsSuppressed` で捨てる。K-W2xxx にだけ効き、実行時の診断は抑制しない。複数行の単語の所見は単語の先頭行なので、エントリ定義の前の行に書く。
  - **まだ無い**: カンマによる意図しない単語分割 (「意図しない」を判定できず偽陽性が出るので見送り)。

## 華和梨 7 互換モード (Phase 9.3.0)

`kawarirc.kis` が無く `kawari.ini` があるゴーストを、Phase 7.5.0 の動作で動かす。仕様は `../kawari-docs` の `spec/07-kawari7-compat.md` (正本。変えたら合わせる)。基準のソースは 7.5.0 の配布物 (`kawari-750-kdt`。`kosui.exe` と `shiori.dll` を含む)。

- **モードはゴースト単位**で、辞書を読む前に `TKawariEngine::SetLegacy7(true)` で決める (`DetectLegacy7`: shiori の `Load` と kosui が呼ぶ)。ファイル単位の混在はしない。9 のコンパイラには手を入れず、7.5.0 の字句・文法を `libkawari/kawari7.*` に別に移植し、9 の中間コードに合わせて 7 専用ノード (`TK7*`) を作る。実行時にコンパイルする経路 (`CreateWord` / `GetWordID` / `Parse` / `LoadKawariDict`) は、必ず `CompileWordString` かモード分岐を通すこと (通さないと `adddict` / `set` / `eval` が 9 の文法になって、混在が裏口から入る)。
- ファイル: `libkawari/kawari7.*` (字句・ノード・コンパイラ)、`libkawari/kawari7_engine.cpp` (kawari.ini・辞書・マッチエントリ・ピロ)、`kis/kis7.*` (9 と意味が違うコマンド。**`TKawariVM::LegacyTable` という別の表**に入れて `FunctionCall7` が先に引く。9 の `FunctionTable` や help・lint には出さない)、`shiori/kawari7_shiori.cpp` (SHIORI/2.x の振り分け)。
- **同じ書き方で意味が違うもの** (実在のゴーストで見つかったものを含む): 履歴参照はエントリ呼び出しだけを数える。`if c t f` の `else` 無し。**エントリ名は平坦** (`TNameSpace::SetFlat`。9 は `.` で階層に分け空の節を捨てるので、`A.` が `A` になる。OpenKEEPS 系の辞書が空の変数を連結した `X.` に依存する)。`@` 始まりもローカルにしない (`FlatNames`)。`${A}` は重複を数えて一様に選ぶ (9 の純粋仮想単語・`set` の重複除去は使わない)。`CreateStrWord` は 7 モードでは `TK7String` (9 の文字列は `\` を `\\` で書き出すので、保存→読み込みのたびに `\` が増える)。
- **乱数は 7.5.0 と同じ** (MT19937 と `TKawariEngine::Random()` の式。生成器はエンジンごとで、`misc/misc.h` の `Random` / `SRandom` は廃止)。種と呼び出し順が同じなら応答が 1 バイトも違わない。これが実ゴーストの差分テストの根拠なので、**乱数を引く回数・順序を変える変更は 7 モードでは禁止** (`${A}` で単語が無いとき引かない、など 7.5.0 に合わせてある)。
- SHIORI は **`GET Version SHIORI/2.6` に `SHIORI/3.0 200 OK` で答えて本体に 3.0 を選ばせ** (本体は `GET Version` の応答の先頭行の版で 2.x か 3.0 かを決める。SSP は 2.6 の `GET Version` を先に送る。7.5.0 は `SHIORI/2.6` と答えた)、3.0 の GET / NOTIFY (ID ヘッダ) を `SHIORI/3.0` (Value) で答える。2.x のほかの要求には `SHIORI/2.6` で答える。`Request7` の先頭で 3.0 の要求を 2.x の要求に読み替え (`Normalize30`。**ID に半角の大文字があればイベント、全部小文字なら `GET String` の資源** (`On` 始まりかどうかではない。`ShioriEcho` / `KawariCommand` もイベント)、`OnCommunicate` = COMMUNICATE、`OnTeach` = TEACH、`OnTranslate`、`\` 始まり = GET Word、`getaistate(ex)` = GET Status)、既存の振り分けに乗せて、最後に `Sentence` / `Word` / `Status` / `String` を `Value` に移す。3.0 の NOTIFY は `RequestNotify30` (`ownerghostname` / `otherghostname` は 2.x と同じ、どの ID でも `event.<ID>` を評価)。かつては 3.0 に 400 を返して本体を 2.x に切り替えさせていたが、3.0 を 2.x と同等に扱えるようにしたので廃止した。3.0 の Reference は 8 個より多くても `system.ReferenceN` に入れる (`StoreReferences7`)。本体への送信 (`SetAllowed`) は伝達経路が local のときだけ。`callsaori` は `System.Sender.Path` (大文字始まり) を読むので、7 の `system.Sender.Path` と両方に書く。
- 意図して 7.5.0 と違う点 (仕様書の「意図した違い」と同じ。増やすときは両方に書く): `ver`・`GET Version` は 9 の名乗り、リクエストと SAORI 応答の値は文字列のまま (例外: `debug : debugger` の `ShioriEcho` / `KawariCommand`)、`save` は UTF-8 BOM、`GET Status` の統計、`expr` / `test` の文字列比較は UTF-16、`include` は 16 段まで、Big5 / ISO-8859-1 の `locale` は無し、lint は 7 の単語を調べない。
- 9 の文法の取り込み: `=kis` ～ `=end` は行を集めて 9 の `LoadKawariDictStream` に渡す。単語の中の `$[ ... ]` は、対応する `]` までを切り出して `TKawariCompiler::Compile` に渡す (引用符と `${ }` `$( )` の中の `]` は数えない)。取り込んだ範囲の中は 9 の意味。
- **テスト**: `run.ps1 -Suite kosui7` (`test/kosui7/`。期待値は `test/gen_kosui7_cases.py` で本物の 7.5.0 の `kosui.exe` から作る。手で書いた期待値は「extensions」以降だけ)、`-Suite shiori7` (`REQ` 動詞で SHIORI/2.x の要求をそのまま送る。**`-BinDir` に 7.5.0 のフォルダを渡すと本物の 7.5.0 でも実行でき**、拡張以外が通ることを確かめられる)、`check` の `legacy7_*`。**`test/ghost_compat7.py --ghost <7 のゴースト>/ghost/master`** は、ゴースト同梱の `shiori.dll` (7.5.0) を基準に、SHIORI/2.x の要求列を流して応答のバイト列・SAORI への送信・セーブの往復を比べる (SAORI はスタブ化する。`--real-saori` で無害なものだけ残せる。ゴーストが実時間を使うので、基準どうしの対照と往復比較は 3 回まで再試行する)。提供された実ゴースト: `D:\ssp_src_set\ssp\ghost\kuro_degwin_integration_type` (OpenKEEPS 系)。
- PowerShell の変数名は大文字小文字を区別しない (`run.ps1` でケースのラベル `$name` が引数 `$Name` を上書きして落ちた)。
- 実際の SSP の上では未確認。`run_posix.py` の 7 のスイートは Windows では動かせないので未実行 (構文だけ確認)。

## 実行速度 (Phase 9.2.1)

- 文の実行 (`TKVMCodeScriptStatement::Run`) は、先頭の単語から構築時に決めた `kind` で分岐する (文字列比較しない)。`list` を構築後に書き換える処理を足すなら `Classify()` を呼び直すこと。
- 先頭の単語が固定の文字列の文は、`TFunctionCache` (`kawari_vm.h`) に `System.Function.<名前>` の検索結果を覚える。エントリの名前→ID は変わらないので、`TNameSpace::RegisteredEntryCount()` (エントリは増える一方) が同じなら再利用する。**エントリを `EntryCollection` から消す処理を足すと、この前提が崩れる**。
- `TKawariCompiler::Compile` は、制御文字・`" $ ' ( )`・前後の空白を含まない文字列なら字句解析を通さず `TKVMCodeString` を返す (`IsPlainLiteral`)。字句規則 (`literal_map3`) や前処理を変えたら、元の経路と結果が同じか確かめること。
- `TWordCollection` は ID ごとに `WordIDMap` の iterator (`IDIter`) を持ち、`Delete` で再検索しない。
- 測るときは、辞書の読み込み (約 300ms / 4 万行) と実行を分けること。引用符の中は `${}` が展開されないので、毎回違う単語を作るベンチは引用符なしで書く。`-pg` は全体にかけると mcount が 3 割を占めて歪む。

## 正規表現 (KIS の regex_*)

- `kis/kis_regex.*` が `regex_match` / `find` / `findall` / `count` / `replace(_first)` / `erase(_first)` / `split` / `escape`。仕様は `../kawari-docs` の `kis/regex.md` と `kis/regex_*.md`。複数の結果は引数のエントリに入れる (`split` と違い、中身を置き換える)。
- サブルーチン呼び出し: パターン内に定義の無い `(?&名前)` / `\g<名前>` は、`CRegexpW::Compile` のリゾルバ (`TRegex::Resolve`) が辞書の `${名前}` の展開結果に解決する。エントリが無ければ構文エラー (名前入り)。入れ子の展開は深さ 16 で打ち切る。
- 取り込まれたサブルーチンのグループは、パターン自身のグループの**後ろ**に (サブルーチン内の括弧も含めて) 番号が振られ、`GetGroupCount()` にも数えられる。結果に出すのはパターン自身の分だけなので、`TRegex::GroupCount` は「最初に取り込まれた名前の番号 - 1」で数える (satoriya の実装は取り込み数を引いており、サブルーチン内に括弧があるとずれる)。
- **辞書やスクリプトに書くパターンは、括弧を含むなら `"..."` で囲む**。素の `( )` は華和梨の構文 (グループ) として消える (`kosui --check` が K-W2003 で、置換文字列の素の `${N}` を K-W2004 で警告する)。`"..."` の中は `${}` `$()` が展開されないので、置換文字列の `${1}` も同様。`\n` は改行にならない。
- テストは `test/kosui/cases.txt` (フィクスチャは `kosui/kawarirc.kis` の `re_*`)。

## 華和梨スクリプト (KIS) の勘所

テストやデバッグで辞書を書くときの記憶の鍵。文法の詳細は `../kawari-docs` の `kis/` (KIS コマンド) と `grammar/`、構文 BNF は `spec/06-bnf.md`。以下は `kosui` で動作確認したもの。

```
# 辞書 (kawarirc.kis など)。"エントリ名 : 単語"。コメントは行頭の #
greet : Hello
nest  : ${greet} world!        # ${エントリ} で参照。入れ子も可: ${${key}}
list  : a,b,c                  # カンマで単語を区切る。参照すると 1 つがランダムに選ばれる

# スクリプト領域 (=kis ～ =end) の外の行は辞書、内側は KIS の文の並び
=kis
function twice $(
    echo $@arg[1]$@arg[1];     # $@arg[n] が引数。twice ab -> abab
);
=end
```

```
${greet}                       -> Hello
$[ ${x} + 1 ]                  -> 式の評価 (x が 1 なら 2)
$(echo hi)                     -> hi         インラインスクリプトは $(コマンド 引数...)
$(size list)                   -> 3          エントリの単語数
$(set A "TEST")                -> (空)       代入。次の行の ${A} は TEST。文の間の空白は出力に残る
$(if $[ 1 < 2 ] $(echo yes) else $(echo no))   -> yes
$(adddict list "d")            -> 単語の追加 (size は 4 になる)
```

- SHIORI/3.0 の入口は `System.Callback.OnGET` (GET) と `System.Callback.OnNOTIFY`。リクエストヘッダは `System.Request.<ヘッダ名>` (例: `${System.Request.ID}`, `${System.Request.Reference0}`) に入り、`System.Response.<ヘッダ名>` を設定すると応答ヘッダになる。評価結果が空なら 204、あれば 200 で Value に入る。
- `version` / `craftman` / `shioriid` / `name` はモジュールが自動で答える (辞書で上書き可)。最小の動くハンドラは `build/src/test/shiori/kawarirc.kis`。
- 未定義のエントリを参照すると空文字になりエラーにならない。タイプミスを見つけるには kosui で `loglevel error warning decl` を指定する。

## その他

- 仕様の一次資料はマニュアルのリポジトリ `../kawari-docs` (http://ukatech.shillest.net/kawari-docs/。`kis/` が KIS コマンド、`spec/06-bnf.md` が構文 BNF)。更新履歴は `../kawari-docs` の `other/changelog.md`、バージョンは `libkawari/kawari_version.h`。旧 `document/` (HTML) は廃止して削除した。機能を足したら、マニュアルは kawari-docs 側に書く。
- `build/contrib/` は過去に取り込んだパッチ (Python モジュール化、darwin、SAORI フォールバック等) の保管場所。
- `config.h` が OS / コンパイラ判定 (`KAWARI_WIN32` / `KAWARI_POSIX` 等) を担う。`js` ターゲットは `-U_WIN32` 等で Windows 判定を打ち消している。
