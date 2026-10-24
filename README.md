# 華和梨 (KAWARI)

**プログラマブル準AI 華和梨 Phase 9.2.0**

伺か用の SHIORI (擬似AI) モジュールです。

## 開発元

- **現在の開発**: [Ukagaka Tech Team](https://github.com/ukatech) — <https://github.com/ukatech/kawari>
- **オリジナル**: KAWARI Development Team — <https://github.com/kawari/kawari>

Phase 9 系は、オリジナルの華和梨をフォークし、[ponapalt](https://github.com/ponapalt) と [Claudia](https://ponadocs.shillest.net/claudia/) (Claude) が大幅に改修したものです。
その後、Ukagaka Tech Team に所属してメンテナンスを続けています。
ライセンスと著作権表示はオリジナルのものを引き継いでいます (`LICENSE` を参照)。

## これは何ですか

華和梨 Phase 9 のリリース版です。
ライセンスを始め、新仕様に関するかなりの情報がドキュメント化されています。
マニュアルは別のリポジトリ ([kawari-docs](https://github.com/ukatech/kawari-docs)) にあり、オンラインで読めます。

- マニュアル: <http://ukatech.shillest.net/kawari-docs/>

## 華和梨 8 からの乗り換え

**華和梨 Phase 9 は、Phase 8 (8.2.x) の置き換えとして、そのまま差し替えて使えます。** ゴーストの辞書 (`kawarirc.kis` など)・SAORI・本体との連携を書き換えずに、`shiori.dll` を差し替えるだけで動きます。Phase 7 から 8 へ上がったときのような、辞書の書き直しが必要になる非互換な変更はありません。Phase 7 → 8 の変更点は[マニュアルの移行ガイド](http://ukatech.shillest.net/kawari-docs/migration/)に載っていますが、これは Phase 8 のゴーストが対象で、Phase 9 に上げるだけなら読む必要はありません。

**差し替えるときに知っておくこと** (意図した違いです)

- `save` / `savecrypt` は UTF-8 (BOM あり) で書き出します。Phase 9 は 8.2.x が書いたセーブを読めますが、Phase 9 が書いたセーブを 8.2.9 以前に戻して読むことはできません (差し替えを戻す可能性があるなら、セーブのバックアップを)
- SHIORI の応答に `Charset` ヘッダが付き、SAORI への `GET Version` は UTF-8 で送ります (応答に `Charset` が無ければ、以後は Shift_JIS)。ふつうの SHIORI / SAORI には影響しません
- 文字列の大小比較とエントリの列挙順が、Shift_JIS のバイト順ではなく Unicode のコードポイント順になりました (ASCII だけなら変わりません)
- 不具合の修正で、これまで偶然動いていた異常な書き方の結果が変わることがあります (整数のあふれ、`date` に負の時刻を渡したときの異常終了など)。細かい一覧は[マニュアルの更新履歴](http://ukatech.shillest.net/kawari-docs/other/changelog/)を参照してください
- 診断 (エラー・警告) のログの書式が変わりました (9.2.0)。ログを機械的に読んでいる場合だけ影響します

## 目次

- [開発元](#開発元)
- [華和梨 8 からの乗り換え](#華和梨-8-からの乗り換え)
- [変更履歴 (ChangeLog)](#変更履歴-changelog)
  - [Phase 9](#phase-9)
  - [Phase 8.2](#phase-82)
  - [Phase 8.1](#phase-81)
  - [Phase 8.0](#phase-80)
- [今後の予定 (Future)](#今後の予定-future)

---

## 変更履歴 (ChangeLog)

新しい順に並べています。「非公開」とあるものは一般には公開されなかった版です。

### Phase 9

#### 2026/10/23 Phase 9.2.0 (Core 4.2.0)

**診断 (エラー・警告) の整理**

- コンパイラ・インタプリタ・辞書・KIS コマンド・SAORI のエラーと警告すべてに、コード (`K-E0028` / `K-W0003` など。番号は変更も再利用もしない) と位置 (辞書ファイルと行) を付け、書式を 1 つにした: `ファイル(行): error K-E0028: 本文`。続けて呼び出し連鎖 (`in ${エントリ} at ファイル(行)`) を最大 8 行出す
- `loglevel` で有効にしていない種別は何も出さず、辞書の意味も変えない (未定義のエントリは今まで通り空になる)。コンパイルエラーの書式は変わったが、メッセージの文面は同じ

**`kosui --check` (検査モード) を追加**

- `kosui --check [--format text|json] [--werror] [--decl] [--no-lint] [--eval スクリプト]... [--seed 数] [rcfile]`: 辞書を**安全モード**で読み込み、出た診断を報告して終了する。標準入力は読まず、稼動中のゴーストにも接続しない。終了コードは 0 = 問題なし / 1 = 警告のみ / 2 = エラー / 3 = 実行できない
- 安全モードでは、ファイルを書かず (`save` / `textsave` / `logfile` などは書かずに成功する)、SAORI の実体を読み込まず、本体との通信 (Direct SSTP) もしない。ただし `=kis` は実際に実行する
- 出力は診断だけ。`--format json` は常に UTF-8 の JSON 1 つ。`--eval` で読み込み後にスクリプト (イベントなど) を評価できる

**静的検査 (Lint)**

- `kosui --check` は、読み込み後に辞書の全単語と `=kis` を、実行せずに調べて警告する (`--no-lint` で止める): `K-W2001` どこにも定義されていないエントリの参照、`K-W2002` 組み込みでも定義済みの関数でもないコマンド名、`K-W2003` `regex_*` の引数にある引用符のない `( )` (括弧が消える)、`K-W2004` `regex_replace` の置換文字列にある引用符のない `${N}`
- 華和梨ゴースト 129 個で試したところ、警告の多くは本物の書き間違いだった (`setstr` を `serstr` と書いたもの、`$([ ... ])` の `[`、`);` の後に孤立した `else`、コメントアウトされた定義の参照など)
- 偽陽性は `# lint:disable-next-line=K-W2001, K-W2003` (次の行にだけ効く。華和梨のコメントは行頭の `#` だけなので行末には書けない) で抑制できる

**時刻の 64 ビット化 (2038 年問題)**

- `date` と `mktime` が 64 ビットの通算秒数で動くようになった (32bit の mingw-w64 は `time_t` が 32 ビットで、2038/01/19 12:14:07 (JST) で頭打ちになっていた)。`date %s 4102444800` や `mktime 2100 1 1 0 0 0` が使える
- Windows 版 (C ランタイムの msvcrt) の限界は残る: 1970 年より前と、3000 年末あたりより後は変換できず、`date` は空文字を返し、`mktime` は `-1` を返す。POSIX 版は OS の `time_t` の範囲による
- 式 (`$[ ]`) の整数は 32 ビットのままなので、2038 年以降の通算秒数どうしの計算 (`$[ $(date %s) - ${t0} ]`) はまだ正しく動かない。日数などに直してから計算すること

#### 2026/10/22 Phase 9.1.1 (Core 4.1.1)

**文字コードを指定するテキストファイル操作**

- `textload_charset エントリ ファイル 文字コード` / `textsave_charset ファイル 文字コード エントリ...` / `textappend_charset ファイル 文字コード エントリ...` を追加。`textload` / `textsave` / `textappend` と同じ動作で、ファイルの文字コードを指定できる。従来のコマンドは変わらない
- 文字コードは Shift_JIS・UTF-8・EUC-JP・ISO-2022-JP・EUC-KR・GB2312。`textload_charset` は `auto` (UTF-8 / Shift_JIS の自動判定) も指定できる
- Windows ではコードページ 20932 / 50220 / 949 / 936 を使う。そのため GB2312 は GBK、EUC-KR は UHC として扱う。POSIX は iconv を使う
- 未知の文字コードや、実行環境が対応していない文字コードを指定するとエラーになり、`textload_charset` はエントリを変えず、`textsave_charset` / `textappend_charset` はファイルを書き換えない

**不具合の修正**

- 落ちる不具合: 辞書の `${$}` のように `$` の後ろが置換にならない単語、`$[ (-2147483647-1) / -1 ]`、`date` に負の時刻を渡したとき。`load` / `request` / `unload` の中の例外が本体に漏れないようにした
- `${エントリ}` の形の単語が片付けられて ID が再利用されると、集合演算 (`${a+b}` や `size` など) から単語が消えていた
- `char_at "" 0` が NUL 文字を返していた。`inc` / `dec` が int の上限・下限であふれていた。`mktime` が夏時間を無視していた
- `save` / `savecrypt` / `textsave` に絶対パスを渡すと、本体のカレントフォルダにファイル名だけで書き出していた。相対パスと同じくゴーストのフォルダに書き出すようにした
- 同じ SAORI の DLL を 2 回読み込むと、最後に解放しても DLL が残っていた
- POSIX: ISO-2022-JP への書き出しで変換できない文字があると以降が化けていた。ninix との Direct SSTP で SIGPIPE により落ちることがあった
- kosui は入力の最後に `exit` が無くても終わるようになった。`kawari_encode` などはフォルダ名の `.` を拡張子とみなしていた (入力を出力で上書きしうるファイルは処理しない)
- その他、細かな不具合 ([マニュアルの更新履歴](http://ukatech.shillest.net/kawari-docs/other/changelog/)を参照)

#### 2026/10/19 Phase 9.1.0 (Core 4.1.0)

**無名関数**

- `lambda` を追加。無名の関数を定義して、その名前 (`System.Lambda.<n>`、定義済みの関数と重ならない) を返す。定義の書き方は `function` と同じで、返った名前は `$(${f} 引数)` のようにコマンド名として呼び出せるほか、別の関数に渡して使える
- クロージャではない。`lambda` を定義した場所の `@` で始まるローカルエントリ (外側の関数の `@arg` など) は引き継がない。不要になったら `rmfunc` で消す

**正規表現エンジンの更新 (uka_regexp)**

- パターンの先頭に `(*UCP)` を書くと、`\d` `\w` `\s` `\b` と POSIX クラスが Unicode のプロパティで判定される (既定は従来どおり ASCII のみ)
- 16 bit の `wchar_t` (Windows) で、パターン中のサロゲートペアを 1 文字として扱う。`.` や `[^a]` が絵文字全体に一致し、一致の開始・終了位置がペアの途中になることはない。KIS に渡す位置は、従来どおり `wchar_t` 単位
- `a{,m}` を `a{0,m}` として受け付ける。`\K` を先読み・後読みの中に書くとエラーにする。破滅的なバックトラックが多項式時間で終わる
- 出力先の文字列が対象文字列や置換文字列と同じ領域を指す場合の `Replace()` の不具合と、内部バッファの容量計算のオーバーフロー (エラーで中断する) を修正。開始位置がサロゲートペアの途中にあるときは次の文字境界に進める

**ドキュメント**

- 実装されないまま残っていた `expr` / `test` の記述をマニュアルから削除した

#### 2026/10/18 Phase 9.0.0 β4 (Core 4.0.0)

**正規表現**

- 伺か共用の正規表現エンジン [uka_regexp](https://github.com/ukatech/uka_regexp) (Unlicense) を載せ、正規表現を使う KIS コマンドを追加
  - `regex_match` / `regex_find` / `regex_findall` / `regex_count` / `regex_replace` / `regex_replace_first` / `regex_erase` / `regex_erase_first` / `regex_split` / `regex_escape`
  - 構文は PCRE2 に近い (Unicode プロパティ、最短・所有量指定子、先読み・後読み、名前付きグループ、再帰など)。`\d` `\w` `\s` `\b` は ASCII のみが対象。オプションは `i` `s` `m` `x`
  - マッチした部分やグループ、分割結果は、指定したエントリに入る
- パターンの中のサブルーチン呼び出し `(?&名前)` `\g<名前>` で、パターン内に定義の無い名前を、辞書のエントリ `${名前}` の展開結果として取り込めるようにした。部品にした正規表現を辞書に置いて組み合わせられる
- パターンに括弧を含むときは `"..."` で囲むこと (そのままだと華和梨の構文として括弧が消える)
- uka_regexp は git のサブモジュール (`build/src/uka_regexp`)。取得は `git clone --recursive`、または clone 後に `git submodule update --init`
- `makedepend.rb` が、同じフォルダのヘッダを `"foo.h"` と書いた `#include` を解決できるようにした (文字コードも CP932 から EUC-JP にそろえた)

#### 2026/10/18 Phase 9.0.0 β3 (Core 4.0.0)

**不具合の修正**

- SAORI over DirectSSTP のモジュール名の小文字化が、本体の `LC_CTYPE` に依存していた (`tr_TR` では `I` が `ı` になり、`INTERNAL_...` を内蔵モジュールと判定できなかった)。ASCII の英字だけを小文字にするように修正
- STL の誤用を修正
  - SAORI の唯一性保証ファクトリのデストラクタが、`erase()` 後のイテレータを使っていた
  - `find()` の結果を `unsigned int` に入れて `npos` と比較していた。64bit の POSIX では比較が成り立たず、`gsub` (一致が尽きたとき) が例外を投げ、`tr` が範囲外を読み、`isexist` が区切り文字を補わなかった。`split` / `match` 系 / SAORI のステータス行は挙動の変更なしで `size_type` にそろえた。32bit の Windows では変化なし
  - `substr` の長さが `int` をあふれうる
  - `=` が復号結果より多い壊れた Base64 を復号すると例外が出ていた
  - 内部のみ (今のところ KIS から届かない): `TEntry::Pop` が空のエントリで `back()` を呼ぶ、`TEntry::Replace` が末尾の次の添字を受け付ける

#### 2026/10/18 Phase 9.0.0 β2 (Core 4.0.0)

**本体との連携**

- 本体 (SSP / ninix) のプロパティシステムと SSTP の `EXECUTE` を使う KIS コマンドを追加
  - `getproperty プロパティ名 [既定値]` / `setproperty プロパティ名 値`
  - `executesstp コマンド [引数...]` / `executesstpx 格納エントリ コマンド [引数...]`
  - 応答コードは `System.SSTP.ResponseCode`、追加データは `System.SSTP.Data` に入る (`executesstpx` は指定エントリにも入る)
  - 送り先は `NOTIFY hwnd` で知る本体のウィンドウ (Windows は Direct SSTP の `WM_COPYDATA`、POSIX は ninix の UNIX ソケット)
- SAORI over DirectSSTP に対応
  - 本体に内蔵されたモジュール (`internal_crypto` など、互換の `houchi` / `httpc` / `string` など) を、DLL なしの SAORI として `callsaori` で呼べる
  - `saoriregist` に実体のない DLL 名を指定するか、未登録の `internal_*` を直接呼ぶ (初回に `internal_system` の `ListModules` で確かめて自動登録)。DLL の実体があれば従来どおり DLL として読む
- 上記は SecurityLevel が local 以外のリクエストの処理からは使えない
- Linux のビルドに `-lrt -lpthread` を追加 (ninix の FMO)
- `System.Sender.Path` が一度もセットされず、`callsaori` が常に `SecurityLevel: Local` を送っていた不具合を修正 (リクエストの伝達経路 `local` / `external` / `unknown` が入り、SAORI へ渡す SecurityLevel になる)

#### 2026/10/18 Phase 9.0.0 β1 (Core 4.0.0)

**内部文字列**

- 内部文字列をすべて `wchar_t` 化 (Windows は UTF-16、POSIX は UTF-32)。Windows では BMP 外の文字 (絵文字など) は 2 文字と数える
- 文字列の大小比較 (`compare`、式中の `<` `>`、エントリ列挙順) を SJIS のバイト順から Unicode のコードポイント順に変更 (ASCII のみなら変わらない)
- 2 バイト目が `0x5C` 等になる SJIS 文字や IBM/NEC 拡張文字をファイル名・フォルダ名に使うと誤動作する不具合を修正 (`isdir` / `isexist` / `readdir` 等)

**文字コード (辞書・ファイル)**

- 辞書ファイルの UTF-8 (BOM あり、またはファイル全体が UTF-8 として正当) と Shift_JIS (CP932) を自動判定
  - 暗号化行 (`!KAWA0000` / `!KAWA0001`) は、ファイルに BOM があれば復号結果を UTF-8、なければ Shift_JIS として読む
- `save` / `savecrypt` の出力を UTF-8 (BOM あり) に変更
  - **`savecrypt` した辞書は 8.2.9 以前の華和梨では正しく読めません**
- `textload` が UTF-8 / Shift_JIS を自動判定。`textsave` / `textappend` は UTF-8 (BOM なし) で書く
  - `textappend` は Shift_JIS の既存ファイルにも UTF-8 で追記する
- `kawari_encode` / `kawari_encode2` は入力が UTF-8 なら出力に BOM を付ける。`kawari_decode2` は入力に BOM があれば出力にも付ける
- `rccharset` が UTF-8 指定でも日本語メッセージになる
- `logfile` は UTF-8 (BOM なし) で出力。kosui のコンソール入出力は ACP (POSIX は UTF-8)

**SHIORI / SAORI**

- SHIORI: リクエストの `Charset` ヘッダで解釈し、同じ文字コードで応答 (無い/不明なら Shift_JIS)
  - 全応答に `Charset` ヘッダを付ける
  - `System.Response.Charset` (`Shift_JIS` / `UTF-8`) で指定も可能
- SHIORI: `loadu` (UTF-8 パスの `load`) をエクスポート
- SAORI: UTF-8 で送り始め、`GET Version` 応答の `Charset` に従う (無ければ Shift_JIS)
  - 応答は `Charset` に従って解釈
  - DLL が `loadu` を持てば `loadu` を呼ぶ

**ビルド・プラットフォーム**

- Win32 API を W 版 (UNICODE) に変更、`WINVER` は `0x0500` (Windows 2000)
- 非 Windows の文字コード変換に iconv を使用 (FreeBSD / darwin は `-liconv`)

### Phase 8.2

#### 2026/10/17 Phase 8.2.9

- Windows 版 (mingw) のバイナリにバージョン情報リソースを追加
- 小オブジェクト用アロケータの導入と単語比較の軽量化で処理速度を向上 (mingw 版)

#### 2008/03/30 Phase 8.2.8

- `textsave` コマンドが複数のエントリを同時に保存できないバグを修正 (Thanks: Don, ukiya, xenon)

#### 2008/02/16 Phase 8.2.7

- `insertstr` コマンドの動作が `insert` コマンドと同じになっていたバグを修正
- `entrycount` コマンドが正しいエントリ総数を返さないバグを修正 (Thanks: おば９)

#### 2008/01/27 Phase 8.2.6

- `gsub` コマンドが連続した pattern を `""` で置換すると、置換漏れを起こすバグを修正 (Thanks: C.Ponapalt)

#### 2008/01/26 Phase 8.2.5

- 辞書中に半角中点 (･) があると異常動作を起こすバグを修正
  (UPX によるエンバグ？ gcc 最適化を `-O1` にすることで対応)
- `isexist` コマンドがルートフォルダで動作しないバグを修正
- 幸水に quiet モード追加。コマンドラインオプションで `--quiet` を指定することにより、初期化ファイルを読み込むと直ちに終了する
- ninix-aya 用コンパイル時のエラー修正 (Thanks: paulliu)

#### 2005/10/30 Phase 8.2.4

- `rfind` コマンドが検索に失敗したり、仕様にない数値を返すバグを修正 (`libkawari/kawari_ns.cpp` の `TEntry::RFind` を修正)
- Visual C++ 7.1 でコンパイルする際のプロジェクトファイルを添付 (Thanks: 芝やんさん)
- 華和梨が Visual C++ 7.1 でコンパイルできない問題を修正 (`misc/l10n.cpp` の `ctow` 関数の式解釈順序依存部分を削除) (Thanks: 芝やんさん)
- メイクファイル (`gcc.mak`) ninix-aya 向け部分の typo を修正 (Thanks: jadoさん)

#### 2005/07/03 Phase 8.2.3

- 辞書系コマンドのインデックス管理コードを整理
- `inc` / `dec` の添え字を範囲指定可能に機能拡張。指定範囲の単語をすべて `inc` / `dec` する

#### 2005/06/21 Phase 8.2.3RC (非公開)

- 演算式で数値/文字列自動判定が間違っていたバグを修正
- リリースに使用するコンパイラを MinGW 4.1.0 (gcc 3.4.2) に
- リリースのバイナリを、gcc による直接 dll 生成に切り替え (dllwrap は obsolete?)
- gcc 3.x の warning 対応
- `split` のぬるぽバグを修正
- `inc` / `dec` が負の添え字でハングするバグを修正
- `mktime` コマンド追加

#### 2004/09/05 Phase 8.2.2

- Phase 8.2.1 において、辞書系コマンドのインデックスで負の値が使えなくなっていたバグを修正

#### 2004/09/04 Phase 8.2.1

- 辞書系コマンドに共通で、「ー」など一部の文字を含むエントリ名のインデックス指定ができないバグを修正 (Thanks: 89さん@OpenKEEPS スレ)
- あべさんの ninix-aya 向けパッチ、phonohawk さんの偽林檎向けパッチをソースに取り込み (Thanks: あべさん、phonohawk さん)

#### 2004/02/01 Phase 8.2.0

- `date` コマンドに第 2 引数を epoch からの経過秒数と解釈し、現在の時刻と同じようにフォーマット出力するよう機能追加
- `split` に第 3 引数として `""` を与えた時ハングアップするバグを修正
- `split` に第 3 引数として `""` を与えた場合と第 3 引数を省略した場合が等価になるよう機能追加 (どちらも文字ごとに分割する)
- `split` にあわせ、`join` も第 3 引数省略時は `""` を与えた場合と等価になるよう機能追加
- `listsub` コマンドが、すでに消去したエントリツリー構造まで見つけるバグを修正
- 文字列検索/置換系コマンドにおいて、マッチパターンに `""` を指定した場合、マニュアル通りにすべての文字境界にマッチするよう修正
- 文字列検索/置換系コマンドにおいて、存在しないインデックスを指定した場合、ハングアップしたり、正しくない検索結果を返すバグを修正
  - 存在しないインデックスを与えた場合、失敗 (`""` もしくは `-1`、コマンド依存) を返す
- `rsub` コマンド追加
- `getenv` コマンド追加
- `textappend` コマンド追加
- `continue` 構文コマンド追加。このコマンドはループ構文中でのみ機能する
- 上記に合わせ、`break` 構文コマンドはループ構文中でのみ機能するよう変更

### Phase 8.1

#### 2003/05/18 Phase 8.1.0

- `split` の空要素に関する扱いが Phase 7.3.1 の頃と異なっていたバグを修正
- 要素を区切り文字で結合するコマンド `join` 追加
- コンパイルに用いる UPX のバージョンを 1.2.4 に変更
- `saorilist` コマンド追加
- `function <関数名>` で関数定義を取れるようにした
- `rmfunc` 構文コマンド追加
- `xargs` の第一引数に、存在しない、もしくは空のエントリ名を指定するとエラーになるバグを修正
- `shiftcode` が `shift` と同じ実装になっていたバグを修正 (Thanks: ふぁんとむ９さん)
- 空文字列とのマッチが true とならないことがあったバグを修正
- 華和梨エンジンを完全オブジェクト化 (ログストリームなども含む)
  - 同一プロセス内に複数インスタンスの生成が可能 (1 スレッド/インスタンス制限有り)。主に偽林檎、ninix への適用を前提
  - 現状では複数インスタンス生成をサポートする SAORI が存在しない。native な SAORI については、唯一性保証クラスでガードしている。追って、オブジェクト SAORI 規格の策定が必要
- 勝手規格 SHIORI OBJECT Interface API (`so_*`) の追加
- POSIX プラットフォーム向け SHIORI インターフェースの再定義と、SHIORI-POSIX メンテナンスの凍結
- Java, Python の SHIORI-Server, SAORI-Client I/F に仮対応。主に偽林檎、ninix 対応の叩き台

#### 2002/09/05 Phase 8.1.0 beta1 (非公開)

- 引数により `reverse` が動かなくなっていたバグを修正
- ファイル/ディレクトリの存在確認コマンド群 `isdir` / `isexist` / `isfile` 追加
- `date` コマンドに `%s` (1970/1/1 00:00:00 UTC からの経過秒数) 追加
- ユーザーズマニュアルの `!~` 記述ミスと KIS リファレンスの `date` 説明不備を修正 (Thanks: たくるさん)
- マニュアルの「置換できないケース」に「集合演算式の演算子」記載漏れを修正 (Thanks: ダイスさん)
- `find`、`rfind` が存在しないエントリを検索した時、必ず 0 を返すバグを修正 (Thanks: とりあたまさん)
- SAORI の多重ロードなどに関する対策 (Thanks: りゅうさん、lmmさん)

### Phase 8.0

<details>
<summary>Phase 8.0.0 の正式公開以降 (2002/06/06 〜 2002/07/05)</summary>

#### 2002/07/05 Phase 8.0.0

- KIS コマンドオブジェクトがデストラクトされていなかったバグを修正 (Thanks: 今井さん)
- 幸水起動時に、デフォルトの `System.DataPath` としてフルパスを設定するようにした
- ドキュメント追加 (幸水によるデバッグなど)、整備
- KIS 強化短期決戦プロジェクトであった Project KIU は、当初のスケジュールからは大幅に遅れたものの、より現実に即した形で、より高い品質を達成したものと信ずる。Project KIU は、本日をもって解散される
- 以降のメンテナンスは、通常モードの華和梨開発チームがメンテナとなって行われる
- 正式公開

#### 2002/06/23 Phase 8.0.0 RC3 patch1 (非公開)

- 各種メッセージの日本語化、一晩で作業完。時間って貴重
- 日本語化メッセージを使うには、`rccharset` コマンドを使ってください。例: `rccharset Shift_JIS;`

#### 2002/06/22 Phase 8.0.0 RC3

- 何カ所か、無条件に `max=list.length()-1` しているところがあった
- 配列アクセスの逆コンパイルで、`$[]` が増殖するバグを修正
- `if` の逆コンパイルで `else` が余計に入るバグを修正
- `clear` で存在しない単語をインデックスすると止まる (4G 回のループが回る) バグを修正 (Thanks: みずひらさん)
- 色々ドキュメントを手直し (Thanks: みずひらさん)。特に `communicate` のドキュメントバグを修正 (Thanks: lmmさん)

#### 2002/06/18 Phase 8.0.0 RC2 patch2

- kosui が、カレントディレクトリの `kawarirc.kis` を読まないバグを修正 (Thanks: 118さん@さおりスレ, みずひらさん)
- ついでに、デフォルトの DataPath が空になるバグを修正
- ついでに、`kawarirc.kis` を読まなくする `-norc` オプションを追加
- エラーメッセージの分離/まとめ作業開始

#### 2002/06/17 Phase 8.0.0 RC2 patch1

- 演算式の `(` 〜 `)` が逆コンパイルされないバグを修正 (Thanks: 時乃さん)
- `name`, `craftman` のリソース問い合わせに対応。オーバーライド可 (Thanks: たくるさん)

#### 2002/06/15 Phase 8.0.0 RC2

- ビルドのシステムを多少変更
- kosui の日本語メッセージ削除
- destruct 時に `writeprotect` を外す
- コンパイラの無限ループをまた一つ潰した

#### 2002/06/06 Phase 8.0.0 RC1 (非公開)

- 単語中の空白の扱いを以前と同等のものに戻す。実際の動作は以下
  - 行頭行末の空白を削除
  - エントリ定義の `:` や文デリミタ (`,` や `;`) の直後の空白を削除
  - 単語の末尾に「空白＋文デリミタ」がある場合、その空白を削除

</details>

<details>
<summary>Phase 8.0.0 のベータ期 (2002/04/15 〜 2002/06/02)</summary>

#### 2002/06/02 Phase 8.0.0 β10.1

- 以下のバグ/不具合を修正
  - shiori.dll の動作がどんどん遅くなるバグ
  - `ASSERT_IF_PROTECTED` が名前を正しく吐かない (直したのを忘れていた)

#### 2002/05/27 Phase 8.0.0 β10

- エントリ内検索コマンド `find` / `rfind`
- entry 操作コマンド `cleartree` / `listsub` / `listtree` / `copy` / `copytree` / `move` / `movetree`
- 華和梨デバッガ ON/OFF コマンド `debugger`
- 試験的に完全複数行モード導入。普通っぽい。丸括弧は必須で、この丸括弧は、その中で `,` による文のセパレートが可能な点でブロックとは異なる

  ```
  エントリ名 [, エントリ名 ... ] (
     文, 文,
     文, 文
  )
  ```

- 演算式で真偽値を `"true"` / `"false"` で出力する (仕様どおり) よう修正
- 以下のバグ/不具合を修正
  - 演算式の結合順序に関する大バグ (Thanks: みずひらさん)
  - DataPath 設定バグ (Thanks: こやまさん)
  - SecurityLevel 設定 (されない) バグ
  - `readdir` の結果格納バグ (Thanks: 今井さん)
  - `substr` の引数判定バグ
  - SAORI-Client にて `GET Version` 時にプロトコル名が付かないバグ (Thanks: 名無しさん@里々スレ)
  - ログ設定を無視して出力する部分
  - 様々なドキュメントバグ (keeps8 のトークも含む)
- ちょびっと高速化

#### 2002/05/04 Phase 8.0.0 β9.1

- エントリ配列呼び出しで、負の値を認識していなかった
- Direct SSTP により、kdb (もどき) 復活。受け側にスクリプトが必要

#### 2002/05/02 Phase 8.0.0 β9

- コンパイラの無限ループバグを修正。昔直した覚えがあるんだけどなー
- `kis_reference.html`。ついに終わったよー！
- β公開開始

#### 2002/05/01 Phase 8.0.0 β8.2 (非公開)

- `compare`
- `textload` / `readdir` の引数の順番を変更
- `version` / `craftman` / `shioriid` のオーバーライドを許す
- 存在しないエントリに対するリードアクセスでログを吐くケースを実用域に限定。`loglevel decl` 増設

#### 2002/04/28 Phase 8.0.0 β8.1 (非公開)

- `wordcount`, `entrycount`
- ライセンス問題完全決着。やっと公開できる

#### 2002/04/27 Phase 8.0.0 β8

- `System.Resource.` を守ることは無意味なので、2.5 リソース取得手法を `System.Callback.OnResource` に変更
- `foreach` がバグっていた！
- string 系 KIS で負のインデックスを認識
- `substr` の第三引数の解釈を部分文字列の長さに変更
- `writeprotect` コマンド追加
- Logger 若干修正
- 存在しないエントリを呼び出した時にログ

#### 2002/04/26 Phase 8.0.0 β7.1

- 文字列の逆コンパイルバグ
- `match_at` の SJIS 問題
- `cncpath` その他の SJIS 問題

#### 2002/04/25 Phase 8.0.0 β7

- mingw 版に差し替え
- `getrandom` が登録されていなかった (-_-;
- garbage collector
- `KIS_logfile` の未初期化ポインタによるリロード落ちを修正

#### 2002/04/24 Phase 8.0.0 β6

- `programming.html`
- リソースコンパイルに失敗していたので、一時的に取り外し

#### 2002/04/23 Phase 8.0.0 β5

- `programming.html` 腑分け
- さとー氏のドキュメント導入
- ドキュメントちまちま手直し
- ベア文字列をモード分け。強烈
- ついに `test`, `expr` コマンドを削除。dll が再び 300KB を切った

#### 2002/04/22 Phase 8.0.0 β4

- `securitylevel` 指定コマンド
- `insert`, `match_at`
- `erase` は必要なかった。`clear` があった
- MT BSD ライセンス版
- `randomseed` 復活 → `srand`
- `start.html`
- license 問題対応完了。MT ライセンス表記まで含めたライセンス情報は、`ver license;` で出力。もう面倒…
- `license.html`

#### 2002/04/19 Phase 8.0.0 β3

- エントリ名を読む部分の文法と実装を若干修正。結局 `$$entry[0][1]` とかできるように……
- `callsaori` で落ちる件。半ば bcc のバグ
- SHIORI/3.0 対応
- Log 機能、再々度強化
- BNF 修正とそれに合わせたコンパイラの変更
- `index.html`

#### 2002/04/17 Phase 8.0.0 β2

- SAORI Client
- Log 機能、再度強化

#### 2002/04/15 Phase 8.0.0 β1

- 新インタプリタと Phase 7.9.0 をマージ

</details>

---

## 今後の予定 (Future)

### 9.0 (???)

- (未定) namespace
