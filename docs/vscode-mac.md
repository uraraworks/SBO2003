# Mac（VSCode）でサーバーを動かしてブラウザから遊ぶ

Windows 版の SboSvr（サーバー）と SboCli（クライアント）を、Mac でビルドして動かすための手順です。
Linux でも同じ手順で動きます。

- **サーバー**: Emscripten で WebAssembly にして、Node.js で動かします。
- **クライアント**: ブラウザ版（`web/README.md`）を、サーバーがそのまま配ります。

```
            ┌──────────── node out/server/SboSvr.js ─────────────┐
ブラウザ ──HTTP/WebSocket──▶ :8080  （ブラウザ版のファイル配布と /ws）  │
Windows 版 ───────TCP──────▶ :2004  （SboSvr.ini の PortNo）            │
            └──────────── データ: out/server-data/ ───────────────┘
```

## なぜ Node.js で動かすのか

SboSvr は 32bit の Windows 用に書かれていて、ポインタの入った構造体をそのままファイルや通信に書き出しています。
Mac のネイティブ（64bit）でビルドするとポインタが 8 バイトになり、保存データや通信の形が Windows 版とずれてしまいます
（実際に比べると 54 個の構造体の大きさが変わりました）。

WebAssembly（wasm32）はポインタが 4 バイトなので、構造体の形が Windows 版と同じになります。
保存データ（`SBODATA/*.dat`）は Windows 版とそのまま行き来できます。
ブラウザ版クライアントと同じ互換層（`platform/`）を使えるのも利点です。

## 用意するもの

| もの | 入れ方の例 |
| --- | --- |
| Xcode Command Line Tools | `xcode-select --install` |
| CMake・Node.js・Python 3 | `brew install cmake node python` |
| Emscripten（emsdk） | 下を参照 |
| VSCode の拡張 | 開くと推奨が出ます（clangd・CMake Tools・WebAssembly DWARF Debugging） |

emsdk はホームフォルダに置くと、ビルドスクリプトが自動で読み込みます。

```sh
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk
./emsdk install latest
./emsdk activate latest
```

## VSCode でのやり方

リポジトリのフォルダを VSCode で開き、コマンドパレットの「タスク: タスクの実行」から選びます。

| タスク | すること |
| --- | --- |
| 全部ビルド（⇧⌘B） | ブラウザ版とサーバーをビルドする |
| サーバー: 起動 | ビルドしてからサーバーを起動する（データフォルダが無ければ作る） |
| サーバー: 停止 | データを保存してからサーバーを止める |
| サーバー: 接続テスト | Windows 版と同じ手順で TCP 接続できるか確かめる |
| ブラウザ版: ビルド / デバッグビルド | ブラウザ版だけビルドする |
| サーバー: ビルド / リリースビルド | サーバーだけビルドする |

サーバーを起動したら、ブラウザで <http://localhost:8080/> を開くと遊べます。
アカウントは最初のログインで作られます（`SboSvr.ini` の `FreeAcount=1`）。

### デバッグ

実行とデバッグの「サーバー（Node.js 版）」で起動すると、デバッガ付きで動きます。
「WebAssembly DWARF Debugging」拡張を入れておくと、`SboSvr/src/*.cpp` などの C++ のソースに
ブレークポイントを置けます（デバッグビルドのみ）。

## ターミナルでのやり方

```sh
tools/web/build.sh            # ブラウザ版をビルド（out/web/dist）
tools/server/build.sh         # サーバーをビルド（out/server/SboSvr.js）
tools/server/run.sh           # 起動（Ctrl+C で保存してから終了）
tools/server/stop.sh          # 別のターミナルから止める
tools/server/check_tcp.py     # 接続テスト
```

`run.sh` に付けた引数は SboSvr.js に渡ります。

| 引数 | 意味 |
| --- | --- |
| `--http-port <番号>` | ブラウザ版を配る HTTP・WebSocket のポート（既定 8080、0 で使わない） |
| `--www <フォルダ>` | ブラウザ版のファイルの場所（既定 `out/web/dist`） |
| `--data <フォルダ>` | データフォルダ（`run.sh` は `out/server-data` を指定済み。変えたい時は環境変数 `SBO_DATA`） |

## データフォルダ

最初の起動で `Release/SBODATA` と `Release/sbosvr.ini` を `out/server-data/` にコピーします。
遊んだ内容はここに保存されます（Windows 版と同じく、定期保存と終了時の保存）。
最初からやり直したい時は、サーバーを止めて `out/server-data/` を消してください。

- 設定ファイルは `out/server-data/SboSvr.ini` です（ポート番号・管理者アカウントなど）。
- ブラウザ版の接続先は、サーバーが配ったページなら自動で同じサーバーの `/ws` になります。
  別の場所につなぐ時は `?ws=ws://ホスト:ポート/ws` を付けます。

## 管理者ウィンドウ

管理者権限のあるキャラでログインすると、ゲーム画面の横に「管理者用ウィンドウ」が出ます。
中身は Windows 版のダイアログを見たまま再現したもので、
メニューから開く各画面（マップのパーツ・キャラ情報・アイテム一覧・職業情報など）もページ上の小窓として動きます。

- 試しに使う時は `SboSvr.ini` の `[Setting]` に `AllAdmin=1` を書くと、新しく作ったアカウントが全員管理者になります
  （本番では使わないでください）。
- 小窓はタイトル部分をドラッグして動かせます。
- ダイアログの配置は `SboCli/SboCli.rc` から `tools/web/gen_resources.py` が自動で表にします。
- MFC で作られた「管理者ウィンドウ」（職業情報・サブゲーム情報・ステータステンプレート情報）も、
  その下に小窓として出ます。MFC は `platform/include/afx_compat.h` の小さな互換クラスで動かしています。
- ブラウザでは閉じるまで待つ `DoModal` が使えないので、モーダルダイアログは `DOMODAL_THEN`
  （`src/AdminWindow/DlgModal.h`）で「閉じたら続きを呼ぶ」形にしています。Windows 版の動きは同じです。

## 困った時

- **`emsdk が見つかりません`**: 上の手順で `~/emsdk` を用意するか、`source <emsdk>/emsdk_env.sh` してから実行してください。
- **`TCP ポート 2004 で待ち受けできません`**: 別のサーバーが動いています。`tools/server/stop.sh` で止めるか、
  `SboSvr.ini` の `PortNo` を変えてください。
- **ブラウザに「見つかりません」と出る**: 先に `tools/web/build.sh` でブラウザ版をビルドしてください。
- **CPU を少し使い続ける**: Windows 版と同じく、キャラの処理を 1 周するたびに 10ms 休む作りのためです
  （リリースビルドの方が軽くなります）。

## 仕組み（ファイル）

| 場所 | 内容 |
| --- | --- |
| `CMakeLists.txt` / `CMakePresets.json` | サーバーのビルド設定（`mac-debug` / `mac-release`） |
| `tools/server/sources.txt` | コンパイルするゲーム側のソースの一覧（ブラウザ版は `tools/web/sources.txt`） |
| `tools/server/prepare.py` | ソースを SJIS の文字列に直して `out/cmake-server/sbosvr/src` へ置き、ソース一覧を作る |
| `server/server_main.cpp` | 入口（引数を読んで WinMain を呼ぶ） |
| `server/CUraraSockTCP_svr.cpp` | 通信クラスのサーバー版（パケットの切り出し・接続前チェック・CRC・圧縮） |
| `server/node/sbosvr_net.js` | Node.js 側の TCP・HTTP・WebSocket と Ctrl+C の処理 |
| `server/w32_headless.cpp` / `server/SvrStub.cpp` | 画面なしで使わない部分の代わり（HTML 部品・Web 更新・管理ダイアログ） |
| `platform/` | Win32 互換層（ブラウザ版と共通）。`w32_dlg.cpp` はダイアログとメニュー、`w32_ctrl.cpp` はコンボ・リスト・スクロールバー・リストビュー、`w32_mfc.cpp` は MFC の互換クラス |
| `web/w32_popup_js.inc` | 管理者ウィンドウなどの小窓を HTML で描く部分 |
| `SboSvr/src/SboSvr.cpp` | Node.js 版だけ、メインループを 1 回分ずつ回すように分岐している |

## Windows 版のファイル

Windows ネイティブ版はもう作らないので、VC6 のプロジェクト（`.dsw` / `.dsp`）、ランチャー（`sbo/`・`SboLaunch/`）、
`Release/` の配布物（`.exe`・`.dll`・`.bat` など）、Windows 版だけの実装は消してあります。
`Release/` には、サーバーの元データ（`SBODATA/`・`SBODATA(初期状態)/`・`sbosvr.ini`）だけが残っています。

消す前の最後の状態は main のコミット `51d9e3f` です。必要になったら、そこから取り出せます。

```bash
git checkout 51d9e3f -- sbo SboLaunch Release     # フォルダごと取り出す
git show 51d9e3f:SboCli/SboCli.dsp > SboCli.dsp   # 1 ファイルだけ取り出す
```
