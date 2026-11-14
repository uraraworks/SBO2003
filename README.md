# スクラップブックオンライン（SBO2003）

2003 年に始まった個人制作の 2D MMORPG「スクラップブックオンライン (SBO)」を、いまの環境で動くようにしたものです。
当時の Windows 版（Visual C++ 6.0）のソースをほぼそのまま使い、小さな互換層で置き換えて、
サーバーは Node.js、クライアントはブラウザで動かしています。

<p align="center">
  <a href="https://sbo2003.urara-works.jp/">▶ ブラウザで遊ぶ</a>
</p>

後継の「スクラップブックオンライン ページ.2」は [SBOP2](https://github.com/uraraworks/SBOP2) で公開しています。

## 動かし方（Mac / Linux）

Emscripten（emsdk）と Node.js が必要です。くわしい手順は [docs/vscode-mac.md](docs/vscode-mac.md) を見てください。

```sh
tools/server/build.sh     # サーバー（wasm + Node.js）
tools/web/build.sh        # ブラウザ版クライアント
tools/server/run.sh       # http://localhost:8080/ で遊べます（停止は tools/server/stop.sh）
```

管理者としてログインする時は、`out/server-data/SboSvr.ini` の `[Administrator]` の `Password=` を設定してください（空のままだと管理者ではログインできません）。

## ディレクトリ構成

| フォルダ | 内容 |
| --- | --- |
| `SboSvr/` | サーバー（ゲーム本体の処理） |
| `SboCli/` | クライアント |
| `common/` | サーバーとクライアントで共通の定義・ライブラリ |
| `platform/` | Windows API の互換層（Emscripten 用） |
| `server/` | Node.js 版サーバーの入口と通信（TCP・WebSocket・HTTP） |
| `web/` | ブラウザ版の画面・BGM・アカウントまわり（[web/README.md](web/README.md)） |
| `SboGrpData/` `SboSoundData/` | 画像と効果音 |
| `Release/` | サーバーの最小の初期データ（`SBODATA/`、10×10 の草原だけ）と設定（`sbosvr.ini`） |
| `deploy/` | ステージング（Docker）と本番（Windows VPS）用の一式 |
| `tools/` | ビルド・実行用のスクリプト |

## 含まれていないもの

使わせていただいている BGM・効果音の曲名・ファイル名・作者は、ゲームの `/credits`（中身は [web/credits/credits.json](web/credits/credits.json)）にまとめています。

- **BGM の MIDI**：作曲者さんに再配布の許可をいただけていないため入れていません。無くてもビルドでき、BGM が鳴らないだけです。
  手元にある場合は `SboSoundData/MIDI/` に置くとビルドに入ります。
- **BGM の音声ファイル（.m4a）**：新しく入れ替えた曲です。非公開側（SBO2003-private）の `SboSoundData/Audio/` に置き、
  ビルドで `bgm/audio/` へコピーします（`tools/public/fetch-private-assets.sh` が MIDI と一緒に取ってきます）。無くてもビルドでき、その曲のマップの BGM が鳴らないだけです。
- **ゲームの中身のデータ**：マップ・NPC・アイテム・職業などは、解析するとネタバレになってしまうので入れていません。
  `Release/SBODATA/` は 10×10 の草原が 1 枚あるだけの最小データです（サーバーはマップが無いと自分で草原を作ります。
  作り方は `tools/public/make-min-seed.sh`）。キャラを作って歩くところまでは試せます。
- **プレイヤーのデータ**：アカウントや預かり所などは入っていません。

## ライセンス

本プロジェクトは [GNU General Public License v3.0](LICENSE)（GPLv3）で公開しています。
自由に利用・改変・再配布できますが、**派生物も同じ GPLv3 で公開する必要があります**（コピーレフト）。

同梱している次のものは、それぞれのライセンスに従います。

| もの | ライセンス |
| --- | --- |
| spessasynth_lib（`web/vendor/spessasynth/`、BGM の MIDI シンセ） | [Apache License 2.0](web/vendor/spessasynth/LICENSE) |
| GeneralUser GS（`web/soundfont/`、サウンドフォント） | [GeneralUser GS License](web/soundfont/GeneralUserGS-LICENSE.txt) |
| BIZ UDGothic（`web/font/`） | [SIL Open Font License 1.1](web/font/OFL.txt) |
| DotGothic16（`web/font/DotGothic16-Regular.ttf`、16 ドットの字） | [SIL Open Font License 1.1](web/font/OFL-DotGothic16.txt) |
| Noto Emoji（`web/font/NotoEmoji-Regular.ttf`、白黒の絵文字） | [SIL Open Font License 1.1](web/font/OFL-NotoEmoji.txt) |

---

個人制作・技術検証を目的としたプロジェクトです。
