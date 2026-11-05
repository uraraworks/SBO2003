# SBO2003 ブラウザ版

Windows 版クライアント（SboCli）を Emscripten でブラウザ向けにビルドするための一式です。
ゲームのソースはほぼそのまま使い、Windows の API を小さな互換層で置き換えています。

## いまの状態

| 項目 | 状態 |
| --- | --- |
| ロゴ → タイトル → ログイン画面 | 表示できる |
| アカウント名・パスワード入力 | HTML の入力欄を canvas の上に重ねて入力できる |
| サーバーへの接続 | WebSocket で接続し、ログイン・キャラ作成・マップでの移動まで動く（Node.js 版サーバーで確認） |
| 効果音 | SDL_mixer で鳴らす |
| BGM（MIDI） | MIDI のまま、ページ側の JS シンセ（spessasynth_lib）とサウンドフォント（GeneralUser GS）で鳴らす |
| 管理者ウィンドウ | Web 版には入れていない（MFC のダイアログのため） |
| 保存データ（INI など） | ブラウザの IndexedDB に保存される |

## ビルド

Emscripten（emsdk）が必要です。`~/emsdk` にあれば自動で読み込みます。

```sh
tools/web/build.sh          # リリースビルド
tools/web/build.sh debug    # デバッグビルド（-O0 -g、エラー時に関数名が出る）
```

出力は `out/web/dist/` に出ます（`sbocli.html` / `.js` / `.wasm` / `.data`、BGM 用の `bgm/`）。
ブラウザで開くには HTTP サーバーが必要です。

```sh
cd out/web/dist && python3 -m http.server 8000
# → http://localhost:8000/sbocli.html
```

GitHub の archive URL が使えない環境では、先に `tools/web/prefetch-ports-via-git.sh` で
Emscripten のポート（SDL2 など）を取り込んでください。

## BGM（MIDI）のしくみ

Windows 版は LibMusic.dll（ソース無し）で MIDI を鳴らしていました。ブラウザ版では

1. MIDI はほかのリソースと同じく `.data` に入れて preload する
2. ゲームが `g_LibMusic.Open` を呼ぶと、`LibMusicLoader_web.cpp` が MIDI のバイト列を `window.SboBgm.open` に渡す
3. `sbo_bgm.js` が spessasynth_lib の AudioWorklet シンセで鳴らす（ループ・音量もここ）

シンセ本体とサウンドフォント（約 8MB）は `.data` に入れず、ページを開いたあとで読み込みます。
ブラウザは操作されるまで音を出させてくれないので、最初のキー入力かクリックで鳴り始めます。
BGM の全体の音量は `sbo_bgm.js` の `MASTER_GAIN` で調整できます。
開発者ツールのコンソールで `SboBgm.status()` を呼ぶと、再生位置などを確認できます。

## サーバーにつなぐ

いちばん簡単なのは、Mac / Linux で Node.js 版のサーバーを動かすことです（手順は `docs/vscode-mac.md`）。
サーバーがこのブラウザ版も配るので、<http://localhost:8080/> を開くだけで遊べます。

Windows 版のサーバー（TCP のみ）につなぐ時は、ブラウザは TCP を直接使えないので
WebSocket ↔ TCP の中継（ブリッジ）を挟みます。ブリッジは SBOP2 の WebSocketBridge と同じものです。

```sh
cd tools/web/bridge
npm install
TCP_HOST=127.0.0.1 TCP_PORT=2004 WS_PORT=9100 npm start
```

クライアントは `?ws=` で接続先を指定します。

```
http://localhost:8000/sbocli.html?ws=ws://localhost:9100
```

指定しない場合、Node.js 版サーバーが配ったページなら同じサーバーの `/ws` につなぎます。
それ以外は INI の「Server / Address」と「Port / PortNo」から `ws://アドレス:ポート` を作ります。
https のページから開いた時は `wss://` になります。

そのほかの URL パラメータ:

- `?scale=2` … 表示倍率（省略時は画面に収まる整数倍）
- `?pad=1` / `?pad=0` … 仮想パッドを出す／出さない（省略時はスマホなどタッチ操作の端末だけ出す）
- `?perf=1` … 処理時間を 1 秒ごとに画面の左上へ出す（フレーム・描画・canvas への転送の平均と最大。`Module.sboPerf` でも見られる）

## スマホで遊ぶ

タッチ操作の端末では、画面を全体に広げて仮想パッドを出します（SBOP2 と同じ作り）。

- 縦持ち: ゲーム画面 → パッド → メッセージログ の順に縦に並べる
- 横持ち: スティック｜ゲーム画面｜ボタン の 3 列（ログはチャットボタンのシートで読む）
- スティックは上下左右の 4 方向（このゲームは斜めに歩けないため）
- ボタン: キャンセル(Z)・決定(X)・調べる(S)・表示(Tab)・戦闘(Ctrl)・チャット
- チャットボタンで下から入力シートが開き、発言とログの確認ができる
- シートの「吹き出し」タブで吹き出しを選べる（押すとすぐ出してシートを閉じる。最近使ったものと、前に開いていたタブを覚える）。PC 版の吹き出しウィンドウの小窓はスマホでは出さない
- 吹き出しウィンドウは画面に重なるので、スマホでは最初は出さない（オプションから出せる）
- 管理者ウィンドウ・デバッグ情報などの小窓も画面が隠れるので、スマホでは出さない（管理作業は PC で）

## メッセージログ

Windows 版のメッセージログウィンドウは、ブラウザ版ではページの HTML のチャット欄で代用します
（`web/replace/SboCliMsgLogWindow_web.cpp`）。PC は画面右下のパネル、スマホは上のとおりです。
ログはファイルには保存しません。

## 仕組み

```
ゲームのソース（UTF-8）
   │ tools/web/src_prepass.py    #include のパスや全角空白など、VC6 由来の書き方を直す
   ▼
out/web/src
   │
   │ em++（platform/include の windows.h などを使う）
   ▼
platform/     … Win32 互換層（サーバーと共通）
web/          … ブラウザ専用の部分（HTML 部品・入口）
web/replace   … Windows 専用クラスの Web 版（Windows 版の実装は削除済み）
```

- **文字コード**: ソースもゲーム内部の文字列も UTF-8 です（サーバーとの通信・データファイルも UTF-8）。
  文字数の計算は `common/SboText` の関数でまとめて行い、表示幅は ASCII と半角カナを 1、それ以外を 2 と数えます。
  古い Shift_JIS のデータは、サーバーの初回起動時にバックアップ（`SBODATA_SJIS_日時`）を取ってから UTF-8 に変換します
  （`SboSvr/src/SboSvrDataUtf8.cpp`、変換済みの目印は `SBODATA/SboDataUtf8.txt`）。
- **絵文字**: 発言では絵文字を使えます。本文のフォント（BIZ UDゴシック）に無い文字は、白黒の Noto Emoji
  （`web/font/NotoEmoji-Regular.ttf`）で描きます（`platform/w32_gdi.cpp` の SplitTextRuns）。
  白黒なので、文字色や縁取りがほかの文字と同じように効きます。キャラ名には使えません（`SboTextIsNameSafe`）。
- **画面**: 最初に作られたトップレベルウィンドウを canvas に対応させます。GDI は 32bit DIB に描き、
  描画が終わったら canvas へ転送します。
- **文字**: BIZ UDゴシック（`web/font/`、SIL Open Font License）を SDL_ttf で 1bit 描画します。
- **入力欄・ボタン**: EDIT / BUTTON コントロールは HTML の `<input>` / `<button>` を重ねて代用します。
- **リソース DLL**: `tools/web/gen_resources.py` が `.rc` からリソース表を作り、
  `FindResource` / `LoadResource` / `LoadString` はその表と preload したファイルを使います。
- **メインループ**: `SboCliMain` のループを 1 回分ずつ（`MainLoopStep`）に分け、
  ブラウザでは `emscripten_set_main_loop` から描画フレームごとに呼びます。
- **保存先**: 実行ファイルは `/sbo/SboCli.exe` にあることにしています。`/sbo` は IndexedDB に保存されます。

## ファイル

| 場所 | 内容 |
| --- | --- |
| `platform/w32_core.cpp` | ウィンドウ・メッセージキュー・タイマー・キーボード/マウス |
| `platform/w32_gdi.cpp` | DC・ビットマップ・BitBlt・文字描画（文字と画面転送はブラウザ版のみ） |
| `platform/w32_sjis.cpp` | 文字列の境目の処理（ブラウザに渡す前の UTF-8 チェック、古いデータ用の SJIS ⇔ UTF-8） |
| `platform/w32_file.cpp` | ファイル・INI・LoadLibrary・リソース |
| `platform/include/` | windows.h などの代わり（中身は `win32compat.h`） |
| `web/w32_html.cpp` | EDIT / BUTTON を HTML 部品で代用 |
| `web/w32_perf.cpp` | 処理時間の計測（`?perf=1`） |
| `web/web_main.cpp` | 入口（IndexedDB を読み込んでから `SboCliMain` を呼ぶ） |
| `web/replace/` | CDXAudio（SDL_mixer）、CUraraSockTCP（WebSocket）、LibMusic・MAC アドレス・管理画面の代わり |
| `web/shell/sbocli.html` | ページの HTML（キー・マウスの受け渡しもここ） |
| `web/bgm/sbo_bgm.js` | BGM プレイヤー（`LibMusicLoader_web.cpp` から `window.SboBgm` を呼ぶ） |
| `web/vendor/spessasynth/` | MIDI シンセ spessasynth_lib（Apache-2.0）。更新は `tools/web/update_spessasynth.sh` |
| `web/soundfont/` | サウンドフォント GeneralUser GS 2.0.3（SF3、ライセンスは同じ場所の txt） |
| `tools/web/` | 前処理・リソース表・ビルドスクリプト・ブリッジ |

## 今後

1. 戦闘・アイテム・チャットなど、遊びの中身を一通り確かめる
2. 保存データも UTF-8 にする（SBOP2 と同じ方針）
