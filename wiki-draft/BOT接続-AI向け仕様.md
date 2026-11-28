# BOT接続 AI向け仕様（通信・コマンド・Pythonの例）

目的: この文書だけ読んで、Python で WebSocket 接続の BOT クライアントを書けるようにする。
方針: ソースで確認できた事実だけを書く。確認できなかった点は「未確認」と書く。各項目の末尾に根拠を `ファイル:行` で付ける（行番号はこの下書きを作った時点のリポジトリのもの）。
動作確認: **2026-11-28 に、ローカルで起動したサーバー（`tools/server/build.sh` + `run.sh`、ws://localhost:8080、テスト用のデータ）に対して、§7 の Python（websockets 17.4 / Python 3.13）で実際にログイン・キャラ作成・BOT 宣言・入場・歩行・他キャラ受信まで確認した**。確認できた項目には「**確認済み**」と書いてある。それ以外（本番 IIS 経由の wss など）は未確認のまま。構造体のオフセット表は（実機で CHARINFO・CHARPOS・MAPINFO・MAPPARTSINFO・LOGINRES が下の表どおりに読めることを確認済み）、ソースの `common/TypedefCommon.h` を clang（`--target=wasm32-unknown-emscripten`、Windows 型は最小のスタブ）でレイアウトダンプして得た値（作業用の一時ファイルで、リポジトリには置いていない）。

凡例: `LE` = リトルエンディアン。`u8/u16/u32` = 符号なし 1/2/4 バイト。`BOT側` = 作る BOT、`サーバー` = Sbo2003 サーバー。

---

## 0. 全体像（先に読む）

1. `wss://<ホスト>/ws?cv=<クライアント版>` に WebSocket（バイナリ）でつなぐ。
2. サーバーが最初に 4 バイトの「鍵」を送ってくる。計算して返す（接続前チェック）。
3. サーバーが `CMD_CONNECT`（版番号 44）を送ってくる。
4. `CMD_LOGINREQ`（アカウント名・パスワード）を送る。`CMD_LOGINRES` の `byData == 2` が成功。
5. （新規アカウントでキャラ名が空なら `CMD_CHGNAMESEX` で名前を付ける。）
6. **`CMD_BOTDECLARE`（1 バイト）をここで 1 回送る**（ゲームに入る前）。
7. `CMD_CHGSTATE`（`byData = 5`）で入場。マップ・パーツ・キャラ情報が一気に届く。
8. 約 10 秒ごとに `CMD_HARTBEAT` を送る（90 秒来ないと切られる）。
9. 歩く・向く・押すコマンドを送る。速度制限あり（§4）。

パケットの枠（§2）はすべてのコマンド共通。コマンドの中身は C 構造体のバイト列そのまま（§3 以降）。

---

## 1. WebSocket 接続

| 項目 | 内容 | 根拠 |
|---|---|---|
| パス | `/ws`（HTTP と同じポート） | `server/node/sbosvr_net.js:9`, `:852`（ログ文言 `ws://localhost:<port>/ws`）, `:608`（`SBO_DEFAULT_WS = origin に ws 置換 + "/ws"`） |
| 既定ポート | HTTP 8080（`--http-port` で変更）。本番はサーバー自身は 127.0.0.1:18090 で、手前の IIS が 80/443 を受けて逆プロキシする | `server/server_main.cpp:62,70`, `deploy/production/README.md:6-12`, `deploy/production/sbo2003.env.example`（`SBO2003_HTTP_PORT=18090`） |
| wss の経路 | 本番 URL の想定は `https://sbo2003.urara-works.jp/`。TLS は IIS が終端し、`/ws` を含むすべてを 127.0.0.1:18090 へ中継する。つまり BOT は `wss://sbo2003.urara-works.jp/ws` につなぐ | `deploy/production/README.md:5-12,20`, `deploy/production/web.config:4-5` |
| ブラウザ版の組み立て方 | `https:` のページなら `wss`、それ以外は `ws`。ただしサーバーが配ったページでは `SBO_DEFAULT_WS`（同じ origin の `/ws`）を使う | `web/replace/CUraraSockTCP_web.cpp:28-35`, `server/node/sbosvr_net.js:602-609` |
| フレーム種別 | **バイナリ（opcode 2）**。サーバーは 1 つの `send` を 1 フレーム（FIN=1）で送る | `server/node/sbosvr_net.js:289-302,353` |
| 受信側（サーバー）の対応 | opcode 0/1/2（連結・テキストも中身はバイト列として扱う）、8=close、9=ping→pong を返す。マスク有り・無しどちらも解く（RFC 通りクライアントはマスクが必要） | `server/node/sbosvr_net.js:361-424` |
| サブプロトコル | 必須ではない。ブラウザが `Sec-WebSocket-Protocol` を出したら、その先頭を同じに返すだけ | `server/node/sbosvr_net.js:318-322` |
| Origin 検査 | `onUpgrade` に Origin を調べる処理は見当たらない（読んだ範囲では未実装）。ただし手前の IIS 側の設定は未確認 | `server/node/sbosvr_net.js:305-428` |
| permessage-deflate | サーバーは拡張を返さない（`Sec-WebSocket-Extensions` を出す処理が無い）。ライブラリが提案しても使われない見込み（実機は未確認） | `server/node/sbosvr_net.js:309-323` |
| 1 メッセージに複数パケット | **サーバーは受け取ったバイト列を 1 本のストリームとして切り出す**（メッセージ境界を見ない）ので、BOT→サーバーは 1 メッセージに複数パケットを連結してよいし、1 パケットを複数メッセージに分けてもよい。サーバー→BOT は、読んだ範囲では 1 メッセージ 1 パケットだが、公式クライアントも「念のため続けて読む」実装なので、BOT も**ストリームとして**切り出すこと | `server/CUraraSockTCP_svr.cpp:533-540`（`aRecv` に溜めて `ParseRecv`）, `web/replace/CUraraSockTCP_web.cpp:250-251`, `server/CUraraSockTCP_svr.cpp:399-420,448-462`（`SendToMulti`→`FlushSlot` で 1 回 1 パケット） |
| メッセージ上限 | BOT→サーバー: 1 メッセージ 4MiB+1024 バイト、断片は 4096 個まで。超えると切断 | `server/node/sbosvr_net.js:34,375-416` |
| Python クライアント側の注意（確認済み） | `websockets` 15 以降は `http_proxy` 等の環境変数があると `ws://localhost` でもプロキシ経由で接続しようとして**無言でハングする**。`websockets.connect(url, proxy=None)` を指定すること。`max_size=None` も付ける（MAPINFO は展開後 4 万バイト弱、圧縮前でも 1 メッセージが大きい）。`permessage-deflate` を提案されても接続は成功した（確認済み） | 実機 |
| TCP（参考） | 同じバイト列を TCP（既定 2004）でも受ける。本番では外に開けていない | `server/node/sbosvr_net.js:6-11,836`, `deploy/production/README.md:22` |

### 1.1 クエリ `cv`（クライアント版）— BOT が落とし穴にはまりやすい所

- `sbocli.wasm` がサーバーの配布フォルダにある場合、`?cv=` が今のファイルのハッシュと**違う（または無い）と**、サーバーは**テキストフレーム `SBO_RELOAD`** を送り、close コード 4000 で切る。接続は受け付けられない。
  根拠: `server/node/sbosvr_net.js:335-348`。
- `cv` の値 = `sbocli.wasm` の SHA-1（16 進）の先頭 16 文字。サーバーはこれを配信ページ（`GET /` → `sbocli.html`）の `<head>` に `window.SBO_ASSET_VER = {"sbocli.wasm":"…", …};` として埋め込む。
  根拠: `server/node/sbosvr_net.js:560-568`（`sha1 … hex.slice(0,16)`）, `:571`（`assetVersions`）, `:602-609`（埋め込み）, `:682-683`（`/` → `/sbocli.html`）, `web/shell/sbocli.html:6532-6541`（`cv=` を付ける側）。
- したがって BOT は接続の直前に `GET https://<ホスト>/` を取り、`SBO_ASSET_VER` の JSON から `sbocli.wasm` を抜いて `?cv=` に付ければよい（クライアントを更新（デプロイ）するたびに値が変わる）。**ローカルサーバーでは、`GET /` の `SBO_ASSET_VER` から取った `sbocli.wasm` の値を `?cv=` に付けて接続できた（確認済み。`out/web/dist/sbocli.wasm` が配布されている場合のみ検査が効く）。本番（IIS 経由・wss）では未確認。**
- 任意の `tp=1`（タッチ端末の印。統計用）は付けなくてよい。`server/node/sbosvr_net.js:352`, `web/shell/sbocli.html:6542-6545`。
- 統計用に `User-Agent` が分類・記録される（生の値は残さない）。`server/node/sbosvr_net.js:144-150,352`。

### 1.2 接続まわりの制限（BOT が切られる条件）

| 条件 | 値 | 根拠 |
|---|---|---|
| 同じ IP の同時接続 | 8 まで（127.x と ::1 は数えない）。環境変数 `SBO2003_MAX_CONN_PER_IP` で変わる | `server/node/sbosvr_net.js:41,166-176` |
| つないでから最初のデータまで | 60 秒（`SBO2003_FIRST_DATA_SEC`） | `server/node/sbosvr_net.js:42,220-222` |
| 最後に受け取ってから無通信 | 180 秒で切断（`SBO2003_IDLE_SEC`）。これとは別に、ゲーム内の生存確認は 90 秒（§3.5） | `server/node/sbosvr_net.js:43,223-225` |
| 受信量 | 1 秒に 200 フレーム（`nPkts` は WS フレーム数）または 256KiB を、3 秒続けて超えると切断 | `server/node/sbosvr_net.js:44-46,206-214,396` |
| 送信待ち | 8MiB を超えると切断（読まずに放置しない） | `server/node/sbosvr_net.js:47,226-228` |
| 満員 | スロット満杯なら即切断。ログイン時の上限は `MAX_CONNECTCOUNT`＝500（BOT も数える） | `server/node/sbosvr_net.js:182-188`, `docs/bot.md:40` |

---

## 2. パケットの枠（長さ・CRC・圧縮・接続前チェック）

### 2.1 バイト順・パディング

- **すべてリトルエンディアン**（x86 の構造体をそのままコピーして送っている）。
  根拠: `server/CUraraSockTCP_svr.cpp:402-409`（`CopyMemory(&Info…)` で `DWORD` を直接コピー）, `tools/server/check_tcp.py:34-45`（`struct.unpack('<II')`）。
- 構造体は `#pragma pack` が付いているものだけ詰まっている。**付いていないものは C の自然なアライメント（`DWORD` は 4 の倍数）のままバイト列にされる**ので、`byCmd` の直後に 3 バイトの隙間（0 のはず）が入る（例: `CMDPARA_DATA1` は 12 バイト）。ビルドは 32 ビット（Windows 版と Emscripten の wasm32）。ポインタは 4 バイト。
  根拠: `common/TypedefCommon.h` の `#pragma pack(push, 1)` で囲まれたものだけが詰まる（例: `:3115-3146`, `:4156-4435`, `CMDPARA_CHARPOS` の直前 `:4995`）。`CMDPARA_CHARPOS` は詰め有り（17 バイト）、`CMDPARA_DATA1`・`CMDPARA_LOGINREQ`・`CMDPARA_CHARPUSH`・`CMDPARA_CHARINFOREQ` は詰め無し（`:2941-2946`, `:3009-3016`, `:5137-5142`, `:5019-5024`）。
- 受信側（サーバー）は、届いた中身の後ろに 8192 バイトの 0 埋めを足してから処理する。短いパケットでも構造体の外は 0 として読まれる（落ちはしない）が、**意味のある値として頼らないこと**。
  根拠: `server/CUraraSockTCP_svr.cpp:38-64`。

### 2.2 パケットのヘッダ（8 バイト）

```
offset 0  u32 LE  dwSize  … 本体の長さ（圧縮した場合は圧縮後の長さ。ヘッダ 8 バイトは含まない）
offset 4  u32 LE  dwCRC   … 本体（圧縮後のバイト列）の CRC32
offset 8  本体 dwSize バイト
```

- CRC32 は標準の zlib の `crc32`（多項式 0xEDB88320、初期値・最終 XOR が 0xFFFFFFFF）と同じ。Python なら `zlib.crc32(body)`。
  根拠: `common/crc.cpp:48-76`, `tools/server/check_tcp.py:41,49`（実際に `zlib.crc32` で使っている参考実装）。
- 本体が空（`dwSize == 0`）のパケットは**キープアライブ**として読み捨てられる（CRC 欄は見ない）。接続前チェックが終わった後なら送ってよい。
  根拠: `server/CUraraSockTCP_svr.cpp:579-582`。
- 本体が 4MiB（`URARASOCK_MAXPACKETSIZE`）を超えるパケット、接続前チェック中に 4 バイト以外のパケット、CRC 不一致（または CRC が 0）は**即切断**。
  根拠: `server/CUraraSockTCP_svr.cpp:35-37,548-557,584-589`。
- 受信ごとに、サーバーは `byCmd`（本体の先頭 1 バイト）が `CMD_MAX`（195）以上なら黙って捨てる。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:3912-3915`, `common/TypedefCommon.h`（`CMD_MAX` は enum の末尾）。

### 2.3 圧縮

- 送る側の規則: 本体が **128 バイト以上**なら zlib 圧縮を試す。うまく縮んだら本体を次の形にする。縮まなければ（または 128 未満なら）無圧縮のまま。
  ```
  [0xFF][元の長さ u32 LE][zlib 形式の圧縮データ]
  ```
  `dwSize` と CRC は**この 0xFF から始まる本体全体**について付ける。
  根拠: `server/CUraraSockTCP_svr.cpp:299-326`（サーバーの送信）, `web/replace/CUraraSockTCP_web.cpp:196-219`（クライアントの送信）, `common/CUraraSockTCP.h:38`（`URARASOCK_USEZLIBSIZE (128)`）。
- 受信側の判定: **本体の先頭が 0xFF で、かつ長さが 5 以上なら圧縮**。そうでなければ無圧縮。コマンド番号は最大 194 なので、無圧縮の本体が 0xFF から始まることはない。
  根拠: `server/CUraraSockTCP_svr.cpp:591-619`, `web/replace/CUraraSockTCP_web.cpp:286-310`。
- zlib の形式: `deflateInit`/`inflateInit` の既定（zlib ヘッダ付き）。Python の `zlib.compress` / `zlib.decompress` でそのまま扱える。サーバーは展開後の長さが 0 または 4MiB 超だと切断する。
  根拠: `common/myZlib/myZlib.cpp:44-70,134-160`（`deflateInit`/`inflateInit` を既定で呼ぶ。`wbits` 指定なし）, `server/CUraraSockTCP_svr.cpp:594-601`。
- **BOT→サーバーは常に無圧縮で送ってよい**（サーバーは 0xFF で始まらない本体を無圧縮とみなし、長さ制限は 4MiB のみ）。実装を簡単にするため推奨。サーバー→BOT は 128 バイト以上のものがほぼ圧縮で来るので、展開は必須。
  根拠: 上記の受信判定。

### 2.4 接続前チェック（鍵のやり取り）— 暗号ではない

1. 接続直後、サーバーが**通常のパケット枠で 4 バイトの本体**（`DWORD` の鍵）を送る。鍵はサーバー側の `GetTickCount()`（ミリ秒）。つまり最初に受け取るのは `dwSize=4` の 12 バイトのメッセージ。
   根拠: `server/CUraraSockTCP_svr.cpp:481-482`（`dwPreCheck = GetTickCount()` / `QueuePreCheck`）, `:511-521`。
2. BOT は次を計算して、**同じ枠（`dwSize=4`、CRC 付き）の本体 4 バイト**で返す。
   ```
   answer = ((key & 0x12BE46AC) * 0x12BE46AC) & 0xFFFFFFFF   # u32 LE で 4 バイト
   ```
   根拠: `server/CUraraSockTCP_svr.cpp:564-572`（`dwKey &= PRECHECK; dwKey *= PRECHECK;` を比較）, `web/replace/CUraraSockTCP_web.cpp:268-276`（クライアントの計算）, `common/CUraraSockTCP.h:36`, `tools/server/check_tcp.py:46-48`（同じ式の Python 参考実装）。
3. 一致すればサーバーが通常処理に移り、`CMD_CONNECT` を送ってくる（§3.1）。不一致は即切断。
4. これは**暗号や鍵交換ではない**。以降のデータは暗号化も署名もされず、圧縮と CRC だけ。

---

## 3. ログインから入場まで

各コマンドの 1 バイト目 `byCmd` は `common/TypedefCommon.h` の `CMD_` enum の値（`CMD_CONNECT = 1` から連番）。主なコマンド番号（enum を数えた値。`CMD_BOTDECLARE = 194` は `docs/bot.md` の記述と一致）:

| 名前 | 番号 | 向き | 備考 |
|---|---|---|---|
| CMD_CONNECT | 1 | S→B | 接続応答 |
| CMD_LOGINREQ | 2 | B→S | |
| CMD_LOGINRES | 3 | S→B | |
| CMD_CHGSTATE | 4 | 両方 | 入場要求（B→S）／状態通知（S→B） |
| CMD_CHARINFO2 | 5 | S→B | 名前などの簡易情報（`CHARINFOREQ` への返事） |
| CMD_MAPINFO | 6 | S→B | |
| CMD_MAPPARTSINFO | 7 | S→B | |
| CMD_CHGDIRECTION | 8 | 両方 | 向き変更 |
| CMD_CHARPOS | 9 | 両方 | 位置 |
| CMD_CHARINFOREQ | 10 | B→S | キャラ情報要求 |
| CMD_SPEECH | 11 | 両方 | 発言（本書では詳述しない） |
| CMD_ONLINE | 12 | S→B | オンライン数 |
| CMD_HARTBEAT | 13 | 両方 | 生存確認（綴りは HART のまま） |
| CMD_CHGNAMESEX | 23 | B→S | 名前と性別 |
| CMD_CHARPUSH | 26 | B→S | 押す |
| CMD_CHARINFO | 87 | S→B | キャラ情報（詳細） |
| CMD_BOTDECLARE | 194 | B→S | |

根拠: `common/TypedefCommon.h:212-433`（enum。上は定義順に数えた値）。管理者用コマンド（`CMD_MAPPARTSADD` 以上）は管理者だけ受け付けるが、`CMD_BOTDECLARE` などは判定から外してあるので通常アカウントでも通る（`SboSvr/src/SboSvrRecvCmd.cpp:3945`, `docs/bot.md:27`）。

### 3.1 CMD_CONNECT（S→B）— 12 バイト

`CMDPARA_DATA1`（共通の汎用データ構造）:

| offset | 型 | 名前 | CONNECT での意味 |
|---|---|---|---|
| 0 | u8 | byCmd | 1 |
| 1 | u8 | byData | **版番号 = 44（`VERSIONVAL`）** |
| 2-3 | - | 隙間 | |
| 4 | u32 | pID | 自分の接続 ID（100000 + スロット番号） |
| 8 | u32 | dwTblID | 0 |

- 全体 12 バイト。公式クライアントは `byData != 44` なら「バージョンが違います」で止める。BOT も 44 かを確かめるとよい。
  根拠: `common/TypedefCommon.h:25,2941-2946`, `SboSvr/src/SboSvrSendCmd.cpp:369-380`, `SboSvr/src/SboSvr.cpp:252-262`, `SboCli/src/SboCliRecvCmd.cpp:3626-3657`。
- 公式クライアントは、このあと `CMD_LOGINREQ` を送る（端末トークンがあれば `CMD_WEBACCOUNT` のトークンログイン。本書では扱わない）。

### 3.2 CMD_LOGINREQ（B→S）— 48 バイト

`CMDPARA_LOGINREQ`（詰め無し）:

| offset | 型 | 名前 | 内容 |
|---|---|---|---|
| 0 | u8 | byCmd | 2 |
| 1-3 | - | 隙間 | 0 |
| 4 | u32 | pID | 申告値（**サーバーは無視**して接続のものを使う） |
| 8 | u32 | dwTblID | 0（無視される） |
| 12 | char[14] | szAcc | アカウント名。末尾 NUL 必須（実質 13 バイトまで） |
| 26 | char[14] | szPas | パスワード。同上 |
| 40 | char[6] | szMacAdr | MAC アドレス 6 バイト（BOT は 0 で可。下記） |

全体 48 バイト。
根拠: `common/TypedefCommon.h:3009-3016`, `SboCli/src/SboCliSendCmd.cpp:180-193`（クライアントの送り方）, `SboSvr/src/SboSvrRecvCmd.cpp:4143-4165`（`pID` を接続のもので上書き・最終バイトを NUL にする）。

- **文字コード**: サーバーは `strcmp` でバイト列をそのまま比較する（変換無し）。ID・パスワードの規則（使える文字）をサーバー側で縛るコードは今回読んだ範囲では見つからなかった（未確認）。ブラウザ版の入力欄は `maxlength="13"`（`web/shell/sbocli.html:2748,2751`）。**ASCII の英数字にしておくのが安全**（**確認済み**: UTF-8 のバイト列（`ひらがな`/`パス１`）で自動作成でき、同じバイト列で再ログインできた。同じ見た目の文字を Shift_JIS（cp932）で送ると**別のアカウントとして新規作成**された＝サーバーは生バイトで比較するだけ。結論: 文字コードの規定は無く、ASCII が無難。UTF-8 で通すなら全角は 3 バイトなので 13 バイトに収める）。14 バイトぴったりを送ると最終バイトが NUL にされて 13 バイトに切り詰められ、13 バイトで作ったアカウントと同じになった（確認済み）。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4180-4195`（`strlen`＋`strcmp`）。
- **新規アカウントの自動作成**: 知らないアカウント名で送ると、サーバー設定 `[Setting] FreeAcount`（既定 TRUE）が有効なら新しいアカウントを作る。同じ接続元からの作成には回数制限（既定 1 時間 5・1 日 20）があり、**同じ MAC アドレス（0 以外）で 2 つ目は作れない**（`code 5`）。MAC を全部 0 にすると、この重複判定は通る（確認済み: MAC=0 で何人でも作れた）。**ただし作成回数の制限（1 時間 5 件）は実際に効いた**: 同じ IP から 5 件ほど自動作成した後の 6 件目が `byData=5` で断られた（確認済み）。テスト用のローカルサーバーでは `out/server-data/SboSvr.ini` の `[Setting]` に `FreeAcountPerHour=0` と `FreeAcountPerDay=0`（0 で無制限）を足して再起動すると外れる。
  根拠: `SboSvr/src/SboSvr.cpp:424-427`, `SboSvr/src/SboSvrRecvCmd.cpp:3984-3985`（`g_nFreeAcountPerHour = 5`・`PerDay = 20`）, `:4101-4135`（`LoginLimitCheckCreate`）, `:4290-4310`（MAC 重複＝code 5）, `docs/account.md:38`（「同じ端末から 2 つ目のアカウントは作れません」）。
- **ログイン失敗の制限**: 1 つの接続で 5 回失敗すると切断、同じ IP で 20 回失敗（15 分間）すると 15 分間拒否。管理者アカウントは別枠でもっと厳しい。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:3987-3993`（定数）, `:4095`（接続ごとの失敗回数）。
- ログイン前にサーバーが受け付けるのは `CMD_LOGINREQ` と `CMD_WEBACCOUNT` だけ。ログイン後に再度 `CMD_LOGINREQ` を送っても捨てられる。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:3924-3932`。

### 3.3 CMD_LOGINRES（S→B）— `CMDPARA_DATA1` と同じ 12 バイト

| offset | 型 | 内容 |
|---|---|---|
| 0 | u8 | 3（`CMD_LOGINRES`） |
| 1 | u8 | **結果コード `byData`** |
| 8 | u32 | `dwTblID` = 自分のキャラのテーブル ID（成功時。**以降、位置・向き・押す・生存確認のコマンドに入れる自分の ID**） |

`byData` の意味（サーバーが返すもの + クライアントの文言）:

| 値 | 意味 |
|---|---|
| 0 | アカウント名が空 |
| 1 | パスワード違い（アカウントが無い時もこれ。自動作成が無効の場合）。確認済み |
| 2 | **成功** |
| 3 | すでにログイン中（二重ログイン拒否）。**確認済み**: 同じアカウントで 2 本目をつなぐと `byData=3`（`dwTblID` は入る）。BOT を異常終了させた直後の再ログインも、サーバーが切断に気づくまで 3 になりうる |
| 4 | 定員オーバー（BOT 込みで 500） |
| 5 | アカウント作成の拒否（MAC 重複・作成回数超過。後者は確認済み） |
| 6 | ログイン要求の拒否（ログイン禁止中のアカウント、失敗が多い接続元など） |
| 7 | 端末トークンが無効（トークンログイン時のみ。BOT は使わない） |

根拠: `SboSvr/src/SboSvrSendCmd.cpp:384-398`, `SboSvr/src/SboSvrRecvCmd.cpp:4143-4320`（各コード）, `SboCli/src/SboCliRecvCmd.cpp:3663-3735`（コードの意味）。

- **確認済み**: 新規アカウントも既存も、`CMD_LOGINREQ` への返事は `CMD_CHARINFO`(87) → `CMD_LOGINRES`(`byData=2`) の順で、さらに `CMD_BGM 系(136)`・`154` が続く。**CHARINFO が LOGINRES より先に届く**ので、自分の ID は LOGINRES で確定してから、すでに受けた CHARINFO を探して名前が空かを見る（BOT は CHARINFO を溜めておく）。新規キャラの初期位置は マップ 0 の (16,10)、向き 1。
- 成功（`2`）の直前に、サーバーは自分の `CMD_CHARINFO`（§5.2。新規ならキャラ名が空）を送り、直後に BGM 設定などを送る。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4258-4264`（`SendCmdCHARINFO` → `SendCmdLOGINRES`）。
- 注: アカウント名が空（`b''`）で送ると、`byData=0` は返らず**接続がそのまま閉じられた**（close 1000。確認済み。ソースの `byData 0` とは異なる）。
- **この時点でキャラ名（`szName`）が空なら新規キャラ**。次の §3.4 が必要。名前があれば §3.5 へ。

### 3.4 新規キャラ: CMD_CHGNAMESEX（B→S）

名前と性別を送る。**可変長形式**（§5.1 の「文字列を後ろに付ける詰め直し」）で、`szName[64]` を抜いた固定部分 16 バイト＋名前（UTF-8、NUL 終端）。

| 位置 | 型 | 内容 |
|---|---|---|
| 0 | u8 | `byCmd` = 23 |
| 1-3 | - | 0 |
| 4 | u32 | `pID`（無視される） |
| 8 | u32 | `dwTblID` = 自分の ID（LOGINRES で貰った値。**一致しないと無視される**） |
| 12 | u8 | `bySex`（0 以外で女性） |
| 13-15 | - | 3 バイトの 0（構造体の後ろの隙間。**固定部分 16 バイトに含まれるので、短くしない**） |
| 16〜 | char[] | 名前（UTF-8、NUL 終端） |

- 名前の規則（サーバー側）: 表示幅 14（半角 14 文字／全角 7 文字）まで、`SboTextIsNameSafe`（Shift_JIS にある文字だけ。絵文字不可）、NG ワード、同名不可。失敗すると `CMD_ERR`（`SBOERR_NAME`）とキャラ情報が返る。名前が既にあるキャラに名前付きで送っても何も変わらない。
  根拠: `common/TypedefCommon.h:5118-5124`（構造体）, `common/SboPacket.cpp:140-145,160-172`（固定部分の切り出し・`SboPackGet` は固定部分が届いていないと NULL）, `SboSvr/src/SboSvrRecvCmd.cpp:6458-6560`, `common/SboText.h:30-36`。
- 成功すると自分の `CMD_CHARINFO` が返る（名前が入る）。**確認済み**: 固定部 16 バイト＋UTF-8 名前（`ぼっと５` など全角）で受理され、CHARINFO が名前付き（168+名前バイト+1）で返った。名前が既に使われている場合は `CMD_ERR`(28、12 バイト) が返り、キャラ名は空のまま（確認済み。BOT は名前を変えて再送すること）。名前の付いたキャラへ **ID を変えずに**同じ名前を使い回すとこの重複に当たるので、BOT 名はアカウントごとに別にする。

### 3.5 BOT 宣言 → 入場 → 生存確認

**CMD_BOTDECLARE（B→S）: 本体は 1 バイト `[194]` だけ**（枠は他と同じ: `dwSize=1`＋CRC＋本体）。返事は無い。

- **送る位置**: `CMD_LOGINRES` が成功（`byData == 2`）になった後、`CMD_CHGSTATE`（入場）を送る前。ログイン前（`nNo < 0`）に送ると捨てられる。入場後（`byGameState >= GAMESTATE_MOVE`）も無視される。取り消せない。ログアウト・切断で消えるので毎回送る。
- **確認済み**: BOTDECLARE を送って入場した接続がいる間、`GET /health` が `{"onlinePlayers":1,"bots":1}`（普通のプレイヤー 1 人＋BOT 1 人の時）になり、切断すると両方 0 に戻った。宣言した接続は BOT として別集計（`g_dwBotOnline`）される。満員判定には含まれる。それ以外の挙動（速度制限・発言制限など）は通常プレイヤーと同じ。
  根拠: `common/TypedefCommon.h:432,4882-4884`, `SboSvr/src/SboSvrRecvCmd.cpp:3924-3927`（ログイン前は不可）, `:8534-8546`（`BotDeclare`）, `:8576-8581`（受信。未ログインなら無視。本体の長さ自体は検査していない）, `docs/bot.md:5-12,23-47`, `wiki-draft/BOT接続（案）.md`。

**CMD_CHGSTATE（B→S）で入場**（**確認済み**: 返事は下の順で来て、最後の方に `CMD_CHGSTATE`(`byData=5`) が届く。初回の MAPINFO は 39977 バイト（84×96）、MAPPARTSINFO は 32650 バイト（1785 パーツ）で、§5.5/5.6 のパースがそのまま通った）: `CMDPARA_DATA1`（12 バイト）で `byCmd=4`、`byData = 5`（`GAMESTATE_MOVE`）、他は 0 でよい。サーバーは `pID` / `dwTblID` を接続から決める。キャラ名が空だと何もしない。入場後の再送は無視。

- 公式クライアントは `byData = GAMESTATE_MOVE` を送る（`SboCli/src/SboCliKeyProc.cpp:1616`）。`GAMESTATE_MOVE = 5`（`LOGOFF=0, LOGO, TITLE, LOGIN, CONNECTLOST, MOVE=5, NPC`）。
  根拠: `common/TypedefCommon.h:662-670`, `SboCli/src/SboCliSendCmd.cpp:225-236`, `SboSvr/src/SboSvrRecvCmd.cpp:4343-4360`。

**入場直後にサーバーが送ってくる順番**（`RecvDataProcCHGSTATE` の呼び順）:
ステータステンプレート → `CMD_MAPPARTSINFO`(7) → 吹き出し・エフェクト・職業・装備職業・魔法の各情報 → アイテム情報 → 付与表・レシピ・町転送 → 依頼 → **`CMD_MAPINFO`(6)** → イベント座標 → **`CMD_CHARINFO`(87) を画面内の全キャラ分（自分が先頭）** → 所持品・ステータス → **`CMD_CHGSTATE`(4)、`byData=5`（入場完了の合図）** → `CMD_ONLINE` → システムメッセージ（`CMD_SYSMSG`）数通。
根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4408-4490`（呼び順）。
※ BOT が処理しないコマンドは、`dwSize` を読んで捨てればよい（枠は共通）。

**ハートビート（生存確認）**:
- 確認済み: 10 秒ごとに送ると同じコマンド 12 バイトが返ってくる。
- BOT→サーバー: `CMD_HARTBEAT`（13）を `CMDPARA_DATA1` 12 バイトで送る。`dwTblID` は**自分の ID**（違うと無視される）。サーバーは同じコマンドを返す（`CMD_HARTBEAT`、12 バイト、他は 0）。
- 間隔: 公式クライアントは **10 秒**（`SENDHARTBEATTIME = 10000` ms）ごと。サーバーは入場後、最後の受信から **90 秒**超えたら切断する（受信が無くても入場した時刻から数える）。
  根拠: `common/TypedefCommon.h:166`, `SboCli/src/SboCliMain.cpp:3108-3120`, `SboCli/src/SboCliSendCmd.cpp:536-552`（クライアントの送り方）, `SboSvr/src/SboSvrRecvCmd.cpp:5133-5157`（受信と返信）, `SboSvr/src/SboSvr.cpp:1141-1152`（90000 ms）, `SboSvr/src/SboSvrSendCmd.cpp:1613-1626`（返信）。
- ※ これとは別に、WebSocket の ping には pong を返す（`server/node/sbosvr_net.js:407-409`）。サーバーから ping を送る処理は見当たらない。無通信 180 秒の切断（§1.2）も、ハートビートを送っていれば起きない。

---

## 4. 動く（歩く・向きを変える・押す）

### 4.1 方角と座標

- 向き `byDirection`: **0=上、1=下、2=左、3=右**。座標は `X` が右、`Y` が下に増える。1 歩の差分は `dx = (0,0,-1,1)[向き]`、`dy = (-1,1,0,0)[向き]`。
  根拠: `common/TypedefCommon.h:2351-2354`（向きの軸のコメント）, `SboSvr/src/Glbldata.cpp:91-92`（`anPosAddTblX/Y`）。
- 座標は 0 始まり。サーバーのマップ範囲は `X < nCX`、`Y < nCY`（`byX`/`byY` は 1 バイトなので、**255 までのマップ**を前提にしている）。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4905-4912`, `common/TypedefCommon.h:4997-5006`（`byX`,`byY` は `BYTE`）。

### 4.2 CMD_CHGDIRECTION（B→S）: 向きだけ変える — `CMDPARA_DATA1` 12 バイト

`byCmd=8`、`byData=向き(0〜3)`、`dwTblID=自分の ID`（`pID` は無視）。向きが 4 以上、または自分以外の ID だと無視。
根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4538-4565`, `SboCli/src/SboCliSendCmd.cpp:244-282`。
サーバーは変化したとき周囲に `CMD_CHGDIRECTION`（同じ形。`dwTblID` が動いたキャラ、`byData` が向き）を送る。`SboSvr/src/SboSvrSendCmd.cpp:1064-1083`。

### 4.3 CMD_CHARPOS（B→S）: 1 マス歩く — 17 バイト（詰めあり）

`CMDPARA_CHARPOS`（`#pragma pack(1)`）:

| offset | 型 | 名前 | 内容 |
|---|---|---|---|
| 0 | u8 | byCmd | 9 |
| 1 | u32 | pID | 申告値（無視される） |
| 5 | u32 | dwTblID | **自分の ID** |
| 9 | u32 | dwMapID | **今いるマップ ID。サーバーの知っている値と違うと何もしない** |
| 13 | u8 | byX | **行き先の X**（今の位置 + dx） |
| 14 | u8 | byY | **行き先の Y** |
| 15 | u8 | byDirection | 向き(0〜3)。サーバーはこの値を向きとして採用する |
| 16 | u8 | byFlg | 0 |

全体 17 バイト（`sizeof` = 17）。
根拠: `common/TypedefCommon.h:4996-5006`（構造体と `#pragma pack`）, `SboCli/src/SboCliSendCmd.cpp:296,375-381`（クライアントが `(現在位置 + 向きの差分)` を行き先に入れて送る）, `SboSvr/src/SboSvrRecvCmd.cpp:4878-4985`。

**確認済み**: 歩けた時、サーバーは**自分宛には `CMD_CHARPOS` を返さない**（成功の ack は無い）。BOT は送った行き先を自分の位置として覚えておき、自分宛の `CMD_CHARPOS` が来た時だけ押し戻しとして上書きする。同じマップの他のキャラ（普通のプレイヤー）には、歩くたびに `CMD_CHARPOS`（17 バイト、`byFlg=0`）が 1 歩ごとに届いた。**確認済み**: 向き `byDirection` は 0=上 1=下 2=左 3=右（BOT が右へ 3 を送ると相手側の CHARPOS も dir=3、x が +1）。

サーバーが無視する／押し戻す条件（押し戻す＝**サーバーが知っている自分の位置を `CMD_CHARPOS` で自分に送り直す**。BOT は自分宛の CHARPOS を受けたらそれを正として座標を直すこと）:
- `dwTblID` が自分以外、または `dwMapID` がサーバーの知っているマップと違う → 無視。
- 向きが 4 以上、座標がマップの外 → 無視（応答も無し）。
- 現在地から 2 マス超（X か Y）離れた行き先 → 押し戻し。
- 倒れている（HP 0）・取引中・壁（ぶつかる当たり判定）・閉まった扉・動かない NPC の上 → 押し戻し。
- **速度が速すぎる** → 押し戻し（**確認済み**: 20 ms 間隔で上下に 26 歩連打したら、約 6 歩ぶんは通り、その後は自分宛 `CMD_CHARPOS`（サーバーが知っている位置）が続けて返ってきた）。
- **壁**（確認済み）: ローカルのパーツ判定で「壁」と出たマスへ強制的に送ると、自分宛の `CMD_CHARPOS`（元の位置・向き）が返った。BOT の側の壁判定とサーバーが一致した。
- 席（落ち物パズル・ピンボール・迷路・じゃん）に座っている、引っ張られている、列に戻る途中 → 押し戻し／無視。
根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4878-4960`（上の各分岐）, `:4747-4781`（`MoveGuardCanEnter`）。

当たり判定（歩けないマス）のうち、**パーツ**による分は、`CMD_MAPPARTSINFO` の `byHitType` が次のものを塞ぐ: `PARTSHIT_BLOCK1(1)`, `HIDE2(4)`, `COUNTER(5)`, `HIDE4(9)`, `HIDE6(11)`, `BLOCK2(2)`。それ以外（0=なし、3=隠れる、6,7=足跡、…）は歩ける。大きいキャラ（`wGrpMain` が 3232/4848）は横に 1〜2 マス余計に見る。橋のマス（`BRIDGEV/H`）には上下の通り方の決まりがある。
根拠: `SboSvr/src/SboSvrRecvCmd.cpp:2899-2943,2945-3010`, `common/TypedefCommon.h:1066-1092`（`PARTSHIT_*` の番号）。扉・印で変わるマス（MAPSWAP）は本書では扱わない（未確認事項）。

### 4.4 速度制限（歩く速さ）

サーバー側の定数（`MoveGuardCheck`）:

| 定数 | 値 | 意味 |
|---|---|---|
| `MOVEGUARD_STEPMS` | 280 ms | 公式クライアントの 1 マスの時間 |
| `MOVEGUARD_RATEPERCENT` | 130 | 公式の速さの 1.3 倍まで許す |
| 1 歩の代金 | `280 * 100 / 130 = 215 ms`（整数計算） | 歩数ぶんだけバケツから引く |
| `MOVEGUARD_BURSTSTEPS` | **6** | バケツの大きさ ＝ 215 × 6 = 1290 ms ぶん |

仕組み: 時間が経つとバケツ（トークン、ms）が貯まる（上限 1290 ms）。1 歩ごとに `(|ΔX| + |ΔY|) × 215 ms` を引き、足りなければ「速すぎる」で押し戻す。サーバーが勝手に位置を動かした時（押された・マップ移動など）はバケツを満タンにして仕切り直す。管理者は検査されない。
**定常速度は約 215 ms/歩（約 4.65 歩/秒）まで**。実際には余裕を見て 230〜280 ms 間隔くらいを使うのが安全（これは推奨で、ソースの値ではない）。

> 注意: 「4 歩分のバケツ」は記憶違いと思われる。ソースでは `MOVEGUARD_BURSTSTEPS = 6`（約 1290 ms ぶん）。

根拠: `SboSvr/src/SboSvrRecvCmd.cpp:4686-4716`（定数・歩の代金）, `:4810-4875`（`MoveGuardCheck`。バケツの上限は `MoveGuardStepMs() * MOVEGUARD_BURSTSTEPS`）。
押し戻した回数が 10 秒に 10 回に達するとログに残るが、切断まではしない。`SboSvr/src/SboSvrRecvCmd.cpp:4698-4699,4796-4807`。

公式クライアントの歩き方（BOT の手本）:
1. 向きを決める（ローカルで）。
2. 行き先の `CMD_CHARPOS` を送る（直前の応答（自分の位置の CHARPOS）が来るまで次を送らない＝ 1 回に 1 つだけ待つ）。
3. 行き先に「押せる NPC」がいれば、`CMD_CHARPOS` は送らず `CMD_CHARPUSH`（§4.5）を送る。
根拠: `SboCli/src/SboCliSendCmd.cpp:296-470`（特に `g_bReqestPos`、`:455-466` の押す分岐）。
この「応答待ち」は公式クライアントの自主規制で、サーバーは強制しない（速度制限が代わりに効く）。サーバーが自分宛に `CMD_CHARPOS` を返してくる条件の細かい所（毎回返すのか、変化のあったタイマー周期だけか）は、サーバーのキャラ処理ループ（`SboSvr/src/SboSvr.cpp:1165-1171`）が移動フラグを見て送る、までを確認した。到着の遅れの上限は**未確認**。

### 4.5 CMD_CHARPUSH（B→S）: 押す — 16 バイト（詰め無し）

| offset | 型 | 名前 | 内容 |
|---|---|---|---|
| 0 | u8 | byCmd | 26 |
| 4 | u32 | pID | 無視 |
| 8 | u32 | dwTblID | 自分の ID |
| 12 | u32 | dwTblIDPush | 押す相手の NPC の `dwTblID`（CHARINFO で届く値） |

- 全体 16 バイト（`CMDPARA_CHARPUSH`。公式クライアントはこの 16 バイトで送る）。
- 押せる NPC は `byNpcType` が **`NPCTYPE_BLOCK(3)`, `BLOCK2(4)`, `BLOCK3(9)`, `BLOCK4(10)`, `CHGPUSH(21)`** のもの（`BLOCK2〜4` は押すとサイコロの目が変わる）。
  根拠: `common/TypedefCommon.h:1095-1121`（番号は enum を数えた値）, `SboSvr/src/SboSvrRecvCmd.cpp:7993-8015`。
- 受け付け条件（サーバー）: 押す NPC が隣（`nDX ∈ [-2,1]`, `nDY ∈ [-1,1]`。縦にずれている時は `nDX <= 0`。大きいキャラの左端基準）、`IsNearRecvChar(…,2)`（同じ空間・近く・見える）、NPC が最後に動いてから 300 ms 以上。押す向きは**押した人と NPC の位置関係**で決まる（保存されている向きではない）。NPC は 1 マス進む。壁・他キャラ・イベントでふさがれていれば**反対向きに跳ね返って 1 マス**進もうとする。進めなければ動かない。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:8016-8210`。
- 返事: このコマンドを受けると、サーバーはまず自分宛に `CMD_CHARPOS`（自分の位置の確認）を返す。押された NPC の移動は、NPC の `CMD_CHARPOS` として周囲に流れる。
  根拠: `SboSvr/src/SboSvrRecvCmd.cpp:8004-8006`（`SendCmdCHARPOS (pChar->pID, …)`）, `:8195-8210`（`CHARINFOFLG_MOVE` を立てる）。
- **サッカーのボールがこの「押せる NPC」かは未確認**。リポジトリのソース・データ（`SBODATA` は含まれていない）に「サッカー」の語は `wiki-draft/BOT接続（案）.md` と管理画面の資料出力にしか無く、ボールの NPC 種別を決めている箇所を見つけられなかった。`BOT接続（案）.md` は「サッカーのボールを含む」NPC の位置・動きを BOT が受け取る想定と書いているだけ（これも案）。実際のボールの `byNpcType` は、BOT が `CMD_CHARINFO` で受け取った値で判定すること（上の 5 種類なら押せる）。

---

## 5. 受け取るデータ

### 5.1 可変長の文字列（「詰め直し」形式）

`CMD_CHARINFO`・`CMD_CHARINFO2`・`CMD_CHGNAMESEX` などは、構造体の固定長の文字列欄（`szName[64]`）を**送らず**、固定部分だけ先に並べ、その後ろに文字列を NUL 終端で並べる。
```
[固定部分（構造体から szName[64] を除いたバイト列）][名前 UTF-8 \0]
```
根拠: `common/SboPacket.h:1-12`, `common/SboPacket.cpp:30-100,121-128`。
- 文字列の長さは欄の大きさ-1 まで（63 バイト）で切られる。
- `CMD_CHARINFO` は名前の後ろに、**構造体の `pszNpcSpeech`（4 バイトのポインタ欄）が 0 以外のときだけ**NPC の台詞（UTF-8、NUL 終端）が続く。`CMD_CHARINFO2` も同じ。ポインタの値そのものに意味は無い（0 かどうかだけ）。
  根拠: `SboSvr/src/SboSvrSendCmd.cpp:766-777`（送信側）, `SboCli/src/SboCliRecvCmd.cpp:5157-5165,5237-5297`（受信側。`pChar->pszNpcSpeech` が非 0 のとき `pDataNext` から読む）。

### 5.2 CMD_CHARINFO（S→B、番号 87）— 他キャラの位置・向き・名前・NPC 種別

構造体 `CMDPARA_CHARINFO`（詰め無し、全体 232 バイト）のうち、**ワイヤ上は 168 バイトの固定部分＋名前＋（台詞）**。ワイヤ位置は、構造体位置から 64（`szName` ぶん）を引く（`szName` より後ろの欄）。

主要項目（`構造体 offset` ／ `ワイヤ offset`。`szName`（構造体 120〜183）は固定部分に無い）:

| 構造体 | ワイヤ | 型 | 名前 | 内容 |
|---|---|---|---|---|
| 0 | 0 | u8 | byCmd | 87 |
| 4 | 4 | u32 | pID | プレイヤー ID（NPC は `dwTblID+1` のものがある。全 NPC がそうかは未確認） |
| 8 | 8 | u32 | dwTblID | **キャラのテーブル ID（BOT はこれを主キーにする）** |
| 12 | 12 | u8 | byAdminLV | 管理権限（他人には 0 にして送る） |
| 13 | 13 | u8 | byCharState | キャラ状態（`CHARSTATE_DEAD` など） |
| 15 | 15 | u8 | byDirection | 向き 0〜3（99 は「消える」を意味するとクライアントが扱う） |
| 19 | 19 | u8 | byNpcType | **0=プレイヤー（未設定）**、NPC は §4.5 の種別 |
| 31 | 31 | u8 | byHide | 隠れている |
| 34 | 34 | u16 | wGrpID | 画像 ID |
| 36 | 36 | u16 | wGrpMain | 画像種別（0=標準、2=横16縦32、3=32×32、4=48×48。大きいキャラは当たりも広い） |
| 54 | 54 | u16 | wMaxHP | |
| 56 | 56 | u16 | wHP | |
| 64 | 64 | u32 | dwMapID | マップ ID |
| 80 | 80 | u32 | dwX | X 座標 |
| 84 | 84 | u32 | dwY | Y 座標 |
| 120 | （無し） | char[64] | szName | → ワイヤでは固定部分の後ろに `名前\0` |
| 184 | 120 | char[14] | szAcc | 他人には空 |
| 198 | 134 | char[14] | szPas | 他人には空（読みに行かないこと） |
| 220 | 156 | u32 | pszNpcSpeech | 0 以外なら末尾に台詞 |
| 224 | 160 | u32 | clName | 名前の色（COLORREF: 0x00BBGGRR） |
| 228 | 164 | u32 | clSpeech | 台詞の色 |

- 名前は固定部分（ワイヤ 168 バイト）の直後から NUL まで。
- 他人向けには、ログイン回数・プレイ時間・作成日・復活地点・NPC の発生設定・内部番号などは 0 や -1 でマスクされる（本人と管理者には全部入る）。
  根拠: `common/TypedefCommon.h:2683-2744`（構造体）, レイアウトダンプ（オフセット）, `common/SboPacket.cpp:100-105`（固定部分の組み立て）, `SboSvr/src/SboSvrSendCmd.cpp:663-790`（送信）, `:598-640`（`MaskCharInfoForOther`）。

**確認済み**: 入場直後に、同じマップの他キャラ（普通のプレイヤー、NPC の「管理者キャラ」）の `CMD_CHARINFO` が届き、後から入ってきたキャラは先にいたキャラ側にも CHARINFO で届いた。BOT 宣言後も範囲は変わらない。

**配信される範囲**: 入場時の全キャラ送信と、その後の更新は、自分と同じマップで **`|ΔX| ≤ 9` かつ `|ΔY| ≤ 9`**（`DRAW_PARTS_X/Y = 7` + 2）のキャラだけ（公式画面の周り）。範囲外のキャラは届かない。`CMD_CHARPOS`（§5.3）の配信範囲はやや広く `|Δ| ≤ 10`（+3）。BOT 宣言をしても、現状この範囲は**変わらない**（範囲購読は「案」で未実装）。
根拠: `SboSvr/src/SboSvrSendCmd.cpp:690-698`（+2）, `:125-170`（`SendCmdInside2` の +3）, `common/TypedefCommon.h:167-168`, `docs/bot.md:53-55`（今後の予定）, `wiki-draft/BOT接続（案）.md`（案）。

### 5.3 CMD_CHARPOS（S→B）— 動いた・消えたキャラの位置

形式は §4.3 と同じ 17 バイト（弓の飛翔中は後ろに `BOWFLIGHTINFO` 24 バイトが付くことがある。`byFlg & 0x02`）。

- `dwTblID`・`dwMapID`・`byX`・`byY`・`byDirection` を読む。**`byDirection == 99` は「画面から消える」の意味**（クライアントが削除する）。
- `byFlg & 0x08`（`FLGCHARPOS_BOWHIT`）は弓の命中通知。BOT は無視してよい。
- **知らない `dwTblID` が来たら** `CMD_CHARINFOREQ`（番号 10、`CMDPARA_CHARINFOREQ`、16 バイト: `byCmd` / `pID`=4 / `dwTblID`=自分 8 / `dwTblIDReq`=知りたい ID 12）を送る。返事は `CMD_CHARINFO2`（番号 5、§5.4）で、名前などが届く。返事が許されるのは、同じマップ・同じ空間で `|Δ| ≤ 13`、または仲間・自分の傭兵など。
- **確認済み**: 他人が接続を切ると、残った側に `byDirection=99` の `CMD_CHARPOS` が届いた（消える）。
- 自分宛の `CMD_CHARPOS` は「サーバーが認めた自分の位置」。押し戻し（§4.3）もこれで来る。
根拠: `common/TypedefCommon.h:4996-5024`, `SboCli/src/SboCliRecvCmd.cpp:5542-5600`（99・未知キャラの扱い）, `SboSvr/src/SboSvrRecvCmd.cpp:5003-5064`（CHARINFOREQ の許可範囲 `DRAW_PARTS+6`）, `SboSvr/src/SboSvrSendCmd.cpp:1089-1112`（送信）。

### 5.4 CMD_CHARINFO2（S→B、番号 5）— 名前・位置の簡易版

`CMDPARA_CHARINFO2`（全体 132 バイト。ワイヤは 68 バイトの固定部分＋名前＋台詞）。

| 構造体 | ワイヤ | 型 | 名前 |
|---|---|---|---|
| 0 | 0 | u8 | byCmd=5 |
| 4 | 4 | u32 | pID |
| 8 | 8 | u32 | dwTblID |
| 12 | 12 | u32 | dwMapID |
| 17 | 17 | u8 | byCharState |
| 18 | 18 | u8 | byDirection |
| 22 | 22 | u8 | byNpcType |
| 26 | 26 | u16 | wGrpID |
| 28 | 28 | u16 | wGrpMain |
| 38 | 38 | u16 | wMaxHP |
| 40 | 40 | u16 | wHP |
| 48 | 48 | u32 | dwX |
| 52 | 52 | u32 | dwY |
| 56 | （無し） | char[64] | szName → ワイヤでは固定部分（68 バイト）の後ろに `名前\0` |
| 120 | 56 | u32 | pszNpcSpeech（0 以外なら末尾に台詞） |
| 124 | 60 | u32 | clName |
| 128 | 64 | u32 | clSpeech |

根拠: `common/TypedefCommon.h:2747-2778`, レイアウトダンプ, `SboSvr/src/SboSvrSendCmd.cpp:806-900`。

### 5.5 CMD_MAPPARTSINFO（S→B、番号 7）— パーツごとの当たり判定

入場直後に 1 回（最初の方）。**マップの床・壁がどのパーツ番号に対応するかの表**。

```
offset 0  u8   byCmd = 7
offset 1       （隙間 1 バイト）
offset 2  u16  wCount          … パーツの数
offset 4  以降 wCount 個: [MAPPARTSINFOSAVE 16 バイト][ANIMEINFO 6 バイト × byAnimeCount]
```
`MAPPARTSINFOSAVE`（16 バイト）の主要項目: offset 0 `byViewType`, **1 `byHitType`（当たり判定。§4.3 の表）**, 2 `byAnimeType`, 3 `byAnimeCount`（直後に続く `ANIMEINFO`(6 バイト) の数）, 4 `byLevel`, 5 `byLight`（最上位ビット 0x80 が「低い」。低いマスもぶつかる）, 6 u16 `wGrpID`, 8 u16 `wBackID`, 10 `byDamage`（ダメージ床）, 11 `byPoison`, 12 u32 ポインタ欄（無意味）。
パーツ番号 = このリストの添字（0 始まり）。
根拠: `common/TypedefCommon.h:1966-1978`, `SboSvr/src/SboSvrSendCmd.cpp:550-595`, レイアウトダンプ。

### 5.6 CMD_MAPINFO（S→B、番号 6）— マップの床・壁データ

```
offset 0       u8   byCmd = 6
offset 4..17775     MAPINFO 構造体（17772 バイト、offset 4 から）
offset 17776   u16 × (nCX × nCY)   … pwMap（マスごとのパーツ番号。LE）。添字 = x + y * nCX
その後       店情報 byCountShop 個 [SHOPINFO][文字列 MAX_SHOPMSG 個（NUL 終端）]、続いて マップ名（NUL 終端）
```
全体は 17776（`sizeof(CMDPARA_MAPINFO)`）＋`2 × nCX × nCY`＋店・名前。圧縮（128 バイト以上）されて来る。

`MAPINFO` の主要項目（パケット先頭からの offset ＝ 構造体 offset + 4）:

| パケット offset | 型 | 名前 |
|---|---|---|
| 4 | u32 | dwID（マップ ID。`CMD_CHARPOS.dwMapID` に使う） |
| 8 | i32 | nCX（横マス数） |
| 12 | i32 | nCY（縦マス数） |
| 16, 20 | i32 | nStartX, nStartY（初期位置） |
| 24, 28 | i32 | nEntranceX, nEntranceY（入口） |
| 32 | u16 | wBGM |
| 34 | u8 | byCountShop（店の数） |
| 35 | u8 | byFlg（`MAPFLG_EFECT 0x01` / `BATTLE 0x02` / `SUBGAME 0x04` / `DARK 0x08` / `SPEECH 0x10` / `MOVE 0x20` / `MAGIC 0x40` / `NOCHARLIGHT 0x80`） |
| 36 | u8 | byLightLevel |
| 37 | u8 | byWeather |
| 他 | | ポインタ欄（40〜）はすべて無意味（受信側が自分で作り直す）。`byViewRange`（構造体 354 → パケット 358）は「見える範囲」 |

- **壁かどうか**は `pwMap[x + y*nCX]` のパーツ番号を `MAPPARTSINFO`（§5.5）の `byHitType` で引いて判定する。サーバーのマップは、扉が閉じた絵のまま入っているので、扉（イベント）の開閉は別（`CMD_EVENTPOS` など。本書では**未確認**）。「印で変わるマス」は `CMD_MAPSWAP`(168) で別送される（**未確認**）。
- マップが変わる（ワープ・移動）たびに `CMD_MAPINFO` が届く。届いたら、キャラ一覧（BOT 側の覚え）を作り直すこと（公式クライアントは全消去している）。
根拠: `common/TypedefCommon.h:2294-2337`（MAPINFO）, `:4984-4987`（CMDPARA_MAPINFO）, `SboSvr/src/SboSvrSendCmd.cpp:455-540`（送信の並び）, `SboCli/src/SboCliRecvCmd.cpp:4834-4950`（受信の並び）, レイアウトダンプ（offset）。

---

## 6. 文字コード

- **ゲーム内の文字列は UTF-8**。2026-11 の「段階 3」で Shift_JIS から切り替えた。版番号 `VERSIONVAL = 44` のコメントも「文字列を UTF-8 にした」。保存データ（SBODATA）も起動時に一度だけ Shift_JIS から UTF-8 に変換される。UTF-8 として不正なバイトは、1 バイトの半角 1 文字として数える。
  根拠: `common/SboText.h:9-10`, `common/TypedefCommon.h:25`, `SboSvr/src/SboSvrDataUtf8.cpp:3-14`。
- 名前（`szName`）・NPC の台詞・システムメッセージ・チャットは UTF-8 で届く／送る。固定長の欄（`szName[64]` など）は終端 NUL 込みのバイト数で、文字の途中では切らない。
- キャラ名に使える文字は「Shift_JIS にある文字」だけ（絵文字不可）。表示幅は半角 1・全角 2 で数え、最大 14。
  根拠: `common/SboText.h:30-36`, `SboSvr/src/SboSvrRecvCmd.cpp:6473-6486`。
- ID・パスワードは §3.2 の通り、サーバーは変換しない生バイト比較（**確認済み**。UTF-8 でも Shift_JIS でも通るが、別のアカウントとして扱われる）。ASCII だけにすれば問題にならない。
- `CMD_SPEECH`（チャット）の本文の細かい形式は本書では調べていない（未確認）。

---

## 7. 動く Python 例（**動作確認済み・2026-11-28**）

ローカルサーバー（ws://localhost:8080）に対して、新規アカウント作成→キャラ作成→BOT 宣言→入場→歩行→他キャラ受信まで動いたコード。普通のプレイヤー（`bot=False`）をもう 1 本つなぎ、BOT の歩きが相手側に `CMD_CHARPOS` で届くことも確認した。使い方: `python3 bot.py <ID> <PW> 0123`（数字は歩く向きの列。0上 1下 2左 3右）。本番につなぐ時は `ws://` を `wss://`、`urllib` の URL を `https://` にし、`proxy=None` は外す（プロキシ経由が必要なら）。本番での動作は未確認。

```python
import asyncio, json, re, struct, sys, urllib.request, zlib
import websockets

PRECHECK = 0x12BE46AC
CMD_CONNECT, CMD_LOGINREQ, CMD_LOGINRES, CMD_CHGSTATE = 1, 2, 3, 4
CMD_CHARINFO2, CMD_MAPINFO, CMD_MAPPARTSINFO = 5, 6, 7
CMD_CHGDIRECTION, CMD_CHARPOS, CMD_CHARINFOREQ = 8, 9, 10
CMD_HARTBEAT, CMD_CHGNAMESEX, CMD_CHARPUSH = 13, 23, 26
CMD_CHARINFO, CMD_BOTDECLARE = 87, 194
GAMESTATE_MOVE = 5
DX = (0, 0, -1, 1); DY = (-1, 1, 0, 0)
import os
STEP_SEC = float(os.environ.get('STEP_SEC','0.25')); FORCE = os.environ.get('FORCE')=="1"

def pack_packet(body):
    return struct.pack('<II', len(body), zlib.crc32(body) & 0xFFFFFFFF) + body

class Unpacker:
    def __init__(self): self.buf = b''; self.precheck_done = False
    def feed(self, data):
        self.buf += data; out = []
        while len(self.buf) >= 8:
            size, crc = struct.unpack_from('<II', self.buf)
            if len(self.buf) < 8 + size: break
            body = self.buf[8:8+size]; self.buf = self.buf[8+size:]
            if size == 0: continue
            if crc != zlib.crc32(body) & 0xFFFFFFFF: raise IOError('crc')
            if size >= 5 and body[0] == 0xFF:
                n = struct.unpack_from('<I', body, 1)[0]
                body = zlib.decompress(body[5:]); assert len(body) == n
            out.append(body)
        return out

def data1(cmd, data=0, pid=0, tbl=0): return struct.pack('<BB2xII', cmd, data, pid, tbl)
def loginreq(acc, pas, mac=b'\0'*6): return struct.pack('<B3xII14s14s6s2x', CMD_LOGINREQ, 0, 0, acc, pas, mac)
def namesex(tbl, sex, name): return struct.pack('<B3xIIB3x', CMD_CHGNAMESEX, 0, tbl, sex) + name.encode() + b'\0'
def botdeclare(): return bytes([CMD_BOTDECLARE])
def charpos(tbl, map_id, x, y, d): return struct.pack('<BIIIBBBB', CMD_CHARPOS, 0, tbl, map_id, x, y, d, 0)
def charinforeq(tbl, target): return struct.pack('<B3xIII', CMD_CHARINFOREQ, 0, tbl, target)

def parse_charinfo(b, off=168):
    cstr = lambda p: (b[p:b.index(0, p)].decode('utf-8', 'replace'), b.index(0, p)+1)
    name, nxt = cstr(off)
    return dict(tbl=struct.unpack_from('<I', b, 8)[0], dir=b[15], npc=b[19],
                grp=struct.unpack_from('<H', b, 34)[0], hp=struct.unpack_from('<H', b, 56)[0],
                map=struct.unpack_from('<I', b, 64)[0], x=struct.unpack_from('<I', b, 80)[0],
                y=struct.unpack_from('<I', b, 84)[0], name=name)

def parse_mapinfo(b):
    map_id, cx, cy = struct.unpack_from('<Iii', b, 4)
    cells = struct.unpack_from('<%dH' % (cx*cy), b, 17776)
    return dict(id=map_id, cx=cx, cy=cy, cells=cells)

def parse_parts(b):
    n = struct.unpack_from('<H', b, 2)[0]; p = 4; hit = []
    for _ in range(n):
        hit.append(b[p+1]); p += 16 + 6*b[p+3]
    return hit
BLOCKING = {1, 2, 4, 5, 9, 11}

def log(*a): print(*a, flush=True)

async def run(host, acc, pas, name='ぼっと', sex=0, bot=True, steps=None, tag='B', secure=False, waittime=8):
    html = urllib.request.urlopen('http://%s/' % host).read().decode('utf-8', 'replace')
    ver = json.loads(re.search(r'SBO_ASSET_VER\s*=\s*(\{.*?\});', html, re.S).group(1))['sbocli.wasm']
    url = 'ws://%s/ws?cv=%s' % (host, ver)
    st = dict(me=None, map=None, parts=None, chars={}, pos=None, entered=False, seen=[])
    async with websockets.connect(url, max_size=None, proxy=None) as ws:
        un = Unpacker()
        async def send(body): await ws.send(pack_packet(body))
        async def enter():
            if bot: await send(botdeclare()); log(tag, 'BOTDECLARE sent')
            await send(data1(CMD_CHGSTATE, GAMESTATE_MOVE, 0, st['me']))
        async def beat():
            while True:
                await asyncio.sleep(10)
                if st['me'] is not None: await send(data1(CMD_HARTBEAT, 0, 0, st['me']))
        async def walker():
            while not st['entered']: await asyncio.sleep(.1)
            await asyncio.sleep(1)
            for d in (steps or []):
                m = st['map']; px, py = st['pos']
                x, y = px+DX[d], py+DY[d]
                ok = 0 <= x < m['cx'] and 0 <= y < m['cy'] and st['parts'][m['cells'][x+y*m['cx']]] not in BLOCKING
                log(tag, 'step', d, (px,py), '->', (x,y), 'ok' if ok else 'blocked(local)')
                if ok or FORCE:
                    await send(charpos(st['me'], m['id'], x, y, d)); st['pos'] = (x, y); st['sent'] = st.get('sent', 0) + 1
                await asyncio.sleep(STEP_SEC)
            await asyncio.sleep(waittime)
            await ws.close()
        hb = asyncio.create_task(beat()); wk = asyncio.create_task(walker())
        try:
          async for msg in ws:
            if isinstance(msg, str): raise RuntimeError(msg)
            for body in un.feed(msg):
                if not un.precheck_done:
                    key = struct.unpack('<I', body)[0]
                    ans = ((key & PRECHECK) * PRECHECK) & 0xFFFFFFFF
                    await send(struct.pack('<I', ans)); un.precheck_done = True
                    log(tag, 'precheck key', hex(key)); continue
                cmd = body[0]
                if cmd not in st['seen']: st['seen'].append(cmd)
                if cmd == CMD_CONNECT:
                    log(tag, 'CONNECT ver', body[1], 'len', len(body), 'pID', struct.unpack_from('<I', body, 4)[0])
                    await send(loginreq(acc, pas))
                elif cmd == CMD_LOGINRES:
                    log(tag, 'LOGINRES raw', body.hex(), body[1], 'len', len(body), 'tbl', struct.unpack_from('<I', body, 8)[0])
                    if body[1] != 2: raise RuntimeError('login failed %d' % body[1])
                    st['me'] = struct.unpack_from('<I', body, 8)[0]
                    c = st['chars'].get(st['me'])
                    if c is not None and c['name'] == '':
                        log(tag, 'new char -> CHGNAMESEX', name); st['pending_name'] = True
                        await send(namesex(st['me'], sex, name))
                    else:
                        await enter()
                elif cmd == CMD_MAPPARTSINFO: st['parts'] = parse_parts(body); log(tag, 'PARTS', len(st['parts']), 'len', len(body))
                elif cmd == CMD_MAPINFO:
                    st['map'] = parse_mapinfo(body); st['chars'].clear()
                    m = st['map']; log(tag, 'MAPINFO id', m['id'], m['cx'], m['cy'], 'len', len(body))
                elif cmd == CMD_CHARINFO:
                    c = parse_charinfo(body); st['chars'][c['tbl']] = c
                    log(tag, 'CHARINFO', c, 'len', len(body))
                    if c['tbl'] == st['me']:
                        st['pos'] = (c['x'], c['y'])
                        if st.get('pending_name') and c['name'] != '':
                            st['pending_name'] = False; log(tag, 'named ->', c['name']); await enter()
                elif cmd == CMD_CHGSTATE:
                    log(tag, 'CHGSTATE', body[1])
                    if body[1] == 5: st['entered'] = True
                elif cmd == CMD_CHARPOS:
                    _, _, tbl, mid, x, y, d, f = struct.unpack_from('<BIIIBBBB', body)
                    log(tag, 'CHARPOS', tbl, 'map', mid, (x, y), 'dir', d, 'flg', f, 'len', len(body), '(me)' if tbl == st['me'] else '')
                    if tbl == st['me']: st['pos'] = (x, y); log(tag, '  ^ pushback/self pos')
                    if d == 99: st['chars'].pop(tbl, None)
                    elif tbl in st['chars']: st['chars'][tbl].update(x=x, y=y, dir=d)
                    else: await send(charinforeq(st['me'], tbl))
                elif cmd == CMD_CHARINFO2:
                    log(tag, 'CHARINFO2', 'len', len(body), struct.unpack_from('<I', body, 8)[0], body[68:body.index(0,68)].decode('utf-8','replace'))
                elif cmd == CMD_HARTBEAT: log(tag, 'HARTBEAT echo', len(body))
                else: log(tag, 'cmd', cmd, 'len', len(body))
        except websockets.ConnectionClosed as e:
            log(tag, 'closed', e)
        finally:
            hb.cancel(); wk.cancel()
    log(tag, 'seen cmds', st['seen'])
    return st

if __name__ == '__main__':
    acc, pas = sys.argv[1].encode(), sys.argv[2].encode()
    steps = [int(c) for c in sys.argv[3]] if len(sys.argv) > 3 else []
    asyncio.run(run('localhost:8080', acc, pas, steps=steps))
```

並行接続の確認は、上の `run()` を `asyncio.gather` で 2 本（`bot=True` と `bot=False`、ID・キャラ名は別々）走らせ、同時に `GET /health` を見るだけでよい。

---

## 8. 未確認・今後確認したい点（まとめ）

2026-11-28 のローカル検証で確認できたもの: ログイン・新規作成・キャラ名付け（UTF-8）、ID/PW の文字コード（生バイト）、`cv`、BOT 宣言と `/health` の bots、入場、歩行、壁・速度の押し戻し、他キャラの CHARINFO/CHARPOS、再ログインで位置が残ること。

1. 本番（IIS 経由の wss、`https://sbo2003.urara-works.jp/`）での接続。ローカルは ws・プロキシ無し。Origin の扱いも未確認。
2. 扉（`EVENTTYPE_DOOR`）・印で変わるマス（`CMD_MAPSWAP`=168）・イベント座標（`CMD_EVENTPOS`=15）の読み方。入場時に 168（14 バイト）と 15（440 バイト）は届くことだけ確認、中身は未解析。歩けるかの判定に必要になる場合がある。
3. サッカーのボールのデータ（NPC 種別・初期位置・コート範囲）。今回のデータには NPC は「管理者キャラ」(npc=1) だけで、`CMD_CHARPUSH` は**試していない**。範囲購読（案）は未実装で、現状は画面の周り（±9 マス）の外は見えない。
4. `CMD_SPEECH` など発言系、戦闘・取引・店のコマンドの形式（本書の対象外）。
5. 歩行速度の限界値の精密な測定（連打で約 6 歩のあと押し戻しが始まることだけ確認。215 ms/歩の定常値は未測定）。
6. アカウント名が空の時にソースは `byData=0` を返すはずだが、実機は接続を閉じた（理由は未調査）。BOT を強制終了した直後の再接続で LOGINRES 無しに閉じられた例が 1 回あった（原因未調査）。
7. 構造体のオフセットは wasm32 のレイアウト。Windows 版（32 ビット MSVC）でも同じとみなしているが未確認。
