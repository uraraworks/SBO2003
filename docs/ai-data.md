# AI から設定データを操作する（データ API・MCP）

管理者ウィンドウで編集している「設定データ」を、AI（Claude Code など）から読み書きできます。
サーバーに HTTP の API（`/api/data/*`）と MCP の入口（`/mcp`）があり、中身は同じです。
まとめて JSON ファイルに書き出したり、JSON から取り込んだりする時は [data-json.md](data-json.md) を見てください。

## 扱えるデータ

| 種類（kind） | 中身 |
| --- | --- |
| `magic` | 特技（魔法）情報 |
| `magicUser` | 特技を使える職業（職業の番号の一覧） |
| `equUser` | 装備できる職業（職業の番号の一覧） |
| `job` | 職業情報とレベルアップ情報（`levelUp`） |
| `stTmp` | ステータステンプレート（NPC・敵の能力のひな形） |
| `item` | アイテム情報（種別ごとの詳細は `detail`） |
| `maker` | 作り手（料理人・鍛冶屋など）。名前と台詞 3 つ（`szMsgTalk`・`szMsgDone`・`szMsgLack`）。NPC の台詞を `@c` + 番号 2 桁にすると開く |
| `recipe` | レシピ。作り手（`byMaker`）・できる物（`wItem`）と個数（`byCount`）・材料 4 種まで（`awMatItem`・`abyMatCount`）・手数料（`dwGold`）・並び順（`wSort`） |

セーブデータの残りも読み書きできます。**プレイヤーのキャラと預かり所（`leave`）だけは読むだけ** です。

| 種類（kind） | 中身 | 書き込み |
| --- | --- | --- |
| `char` | キャラ全員（プレイヤー・NPC）。能力・持ち物・装備・位置、NPC の台詞（`szNpcSpeech`）、巡回ルート（`route`）、転職前の職業で覚えていた特技（`jobMagic`）など。`isPlayer`・`online` でプレイヤーか・ログイン中か分かる | 保存される NPC だけ（下の「NPC」） |
| `map` | マップの情報・イベント（`events`）・お店（`shops`）・メッセージ（`messages`）・アイテム配置（`items`） | できる（`events`・`shops`・`messages` は配列で丸ごと置き換え。`items` は読むだけ。大きさの変更・マップの追加はできない） |
| `mapParts` | マップパーツ（マス目に入る番号）の表示・当たり判定・アニメのコマ（`anime`） | できる（追加もできる。消せない） |
| `efc` | エフェクト情報（アニメのコマ `anime`） | できる（追加もできる。255 個まで。消せない） |
| `comEfc` | 吹き出し情報 | できる（99 個で固定なので追加はできない） |
| `leave` | 預かり所（キャラの `dwLeaveID` がこの番号） | 読むだけ |
| `subGame` | 爆弾ゲーム・迷路・ダンジョン・戦場ゲーム・すごろくの設定 | `detail` の項目。迷路は追加・止める（`byType` 0）もできる（下の「迷路」。ほかの種別の変更・追加・すごろくのマスは管理者ウィンドウで） |

マップのマス目（パーツの番号・フラグ・屋根）は `sbo_map_tiles`（`/api/data/tiles`）で 2 次元の配列として読めます（1 回最大 128×128）。
書き換えは `sbo_map_tiles_set`（`/api/data/tiles/set`）で、左上 `(x, y)` とパーツ番号の 2 次元配列 `parts` を渡します（下見 → 確定、履歴に残り戻せる）。

### NPC（`char`）

- NPC は **`npcId`（NPC番号）** で見分けます。`id`（キャラのテーブル番号）は再起動で詰め直されて変わることがありますが、`npcId` は変わりません。
  セーブデータの `dwNpcID` 列に保存され、前のセーブデータを読んだ時は自動で振ります。
- 書き換えられるのは保存される NPC だけです（プレイヤー・発生元から出た敵は断ります）。
- 足す時は `sbo_add` で `dwMapID`・`dwX`・`dwY` を必ず渡します（他は省くと 0 や空）。
- 消す時は `sbo_update` の変更に `{"deleted": true}` を入れます。
- 元に戻す・しおりで戻すと、足した NPC は消え、消した NPC は **同じ `npcId` で** 足し直します。
- 遊んでいる間に変わる項目（位置・向き・HP など）は、「後から変わったか」の判定では見ません（歩き回る NPC でも戻せる）。

**アカウント名・パスワード・IP アドレス・MAC アドレスは、どのキーでも出しません。**

項目名はソースの変数名と同じです（`szName`・`wMana` など）。意味・範囲・他のデータの番号の参照は
`sbo_schema`（`/api/data/schema`）で見られます。

## 迷路（`subGame` の `byType` 2）

マップの四角い範囲を、誰もいなくなるたびに自動で作り直す迷路です。迷路ごとに見た目・通路の太さを変えられ、いくつでも置けます。
BGM・暗さはマップの設定（`map` の `byBGM`・`byLightLevel`）なので、雰囲気を変えたい迷路は別のマップに置きます（マップは管理者ウィンドウで作る）。

- 足す: `sbo_add` で `{"kind":"subGame","records":[{"byType":2,"detail":{...}}]}`。止める: `sbo_update` で `{"byType":0}`（入口・ゴールのイベントを外す。描いた迷路の形はマップに残る）。
  元に戻す・しおりで戻すと、足した迷路は止まります。`byType` 0 の枠は迷路にして使い回せます。
- `detail` の主な項目
  - `dwMapID`・`byX1`〜`byY2`: 迷路を作る範囲（マップの中。外周は壁になる）
  - `byStartX/Y`: 入口のマス（範囲の外）。ここに乗ると迷路の中のランダムな所へ飛ぶ。`wChipIDStart` がこのマスに置かれる
  - `byGoalToX/Y`: ゴールに着いた後に出るマス（範囲の外）。ゴールの位置は作るたびに変わる（`wChipIDGoal`）
  - `wChipIDRoad`・`wChipIDBlock`: 道と壁のパーツ（壁は通れないパーツにする）
  - `wChipIDRoad2`＋`byRoad2Rate`・`wChipIDBlock2`＋`byBlock2Rate`: 道・壁に混ぜるパーツと割合(%)（落ち葉・花・コケなど。0 で混ぜない）
  - `wChipIDBlockFace`: すぐ下が道の壁の下の段に置くパーツ（石壁の側面など。0 で使わない）
  - `byPathWidth`: 通路の太さ 1〜3 マス（0 は昔からの 2 マス）。範囲が同じでも 1 だと入り組み、3 だと広々する
- 設定を変えると、中に誰もいなければすぐ作り直します（遊んでいる人がいれば、みんな出て 5 秒後）。

## 作り手とレシピ（`maker`・`recipe`）

町の作り手に頼んで、材料から物を作ってもらう仕組みのデータです。**サーバーのデータなので公開リポジトリには入れません。**
`SBODATA/SboDataMakerInfo.dat`・`SboDataRecipeInfo.dat` が無い時は「作り手 0 件・レシピ 0 件」で起動し、
管理ページの「レシピ」タブか API で初めて保存した時に作られます。

- 作り手を足すと番号（0 から）が決まります。NPC（`char` の `szNpcSpeech`）の台詞を `@c00` のように **`@c` + 作り手の番号 2 桁** にすると、
  その NPC に話しかけた時（S キーでも）に作り手の画面が開きます。`@0`〜`@9`・`@a`・`@k` とはぶつかりません。
- `maker`：`byState`（1 有効・2 停止中・0 削除済み）・`szName`・`szMsgTalk`・`szMsgDone`・`szMsgLack`。台詞は `@1`（改行）・`@2`（文の終わり）などが使えます。
- `recipe`：`byState`・`byMaker`・`wSort`（小さい順に並ぶ）・`wItem`・`byCount`（1 以上）・`awMatItem`（4 つ。使わない欄は 0）・`abyMatCount`・`dwGold`（手数料）。材料は 1 種類以上必要です。
- 材料は持ち物（装備中・捨てられない物を除く）のあと預かり所から使われます。できた物は持ち物へ、いっぱいなら預かり所へ届き、両方いっぱいなら断られます。
- 消す時は `byState` を 0 にします（空き枠になり、次に足す時に使い回されます）。
- 変更はログイン中のプレイヤー全員にすぐ届きます。下見 → 反映・変更履歴・元に戻すは他の種類と同じです。

例（始まりの町の料理人）：

```json
{"kind": "maker", "records": [{"szName": "料理人", "szMsgTalk": "いらっしゃい。@1何を作りましょうか？", "szMsgDone": "はい、できました。@2", "szMsgLack": "材料が足りないようです。@2"}]}
{"kind": "recipe", "records": [{"byMaker": 0, "wSort": 10, "wItem": 215, "byCount": 1, "awMatItem": [228, 148, 0, 0], "abyMatCount": [2, 1, 0, 0], "dwGold": 10}]}
```

## つなぎ方（Claude Code）

1. 管理ページ（`/admin`）の「AI連携」タブで、キーを発行します。
   - **読むだけ** … 一覧・検索・履歴を見るだけ
   - **書き込みもできる** … 変更・追加・元に戻すもできる
   - 期限は 7 日・30 日・90 日・なし から選べます。キーは発行した時に一度だけ表示されます。
2. 表示されたコマンドを Mac のターミナルで実行します。

   ```sh
   claude mcp add --transport http sbo2003 https://sbo2003.urara-works.jp/mcp --header "Authorization: Bearer sboai_..."
   ```

3. Claude Code で「アイテムの剣を値段順に並べて」「戦士の Lv10 の必要経験値を 1.2 倍に」のように頼みます。

使わなくなったキーは「AI連携」タブで取り消してください。サーバーの環境変数 `SBO_AI_API=off` で、この API と `/mcp` を丸ごと止められます。

## 安全のしくみ

- **AI 用キー** … 管理者パスワードは AI に渡しません。サーバーにはキーの SHA-256 だけを保存します（`SBODATA/SboWebAiKeys.json`）。
- **下見 → 確定の 2 段階** … 書き込み（変更・追加・元に戻す）は、まず何も変えずに「変更前 → 変更後」の差分と `previewId` を返します。
  同じ内容に `previewId` を付けてもう一度呼んだ時だけ反映します（10 分以内・1 回だけ）。下見の後に誰かがそのデータを変えていたら反映しません。
  範囲外の数・長すぎる文字列・知らない項目名は、下見の時点で断ります。
- **すぐ反映・すぐ保存** … 反映すると、管理者ウィンドウから変えた時と同じようにログイン中の全員へ送り、データファイルもすぐ保存します。
- **変更履歴と元に戻す** … 反映するたびに変更前と変更後を `SBODATA/SboWebDataHistory.json` に残します（直近 500 件）。
  1 回の呼び出し（1 種類のデータ・何件でも）が履歴 1 件で、元に戻すのもその単位です。
  その後に別の変更があったデータは、`force` を付けない限り戻しません。追加した NPC は消し、作り手・レシピは削除（`byState` 0）にして戻します。
  それ以外の種類（マップパーツ・エフェクト）の追加は消せないので戻せません。
  管理ページの「AI連携」タブからも履歴を見て「元に戻す」ができます。
- **しおり（お試しの区切り）** … お試しで作り変える前に、名前を付けて「しおり」を付けておくと、
  その後の変更を「このしおりまで戻す」で種類をまたいでまとめて元に戻せます（下見 → 確定）。
  記録ごとに「しおりの時の姿」に戻すので、途中で何回変えていても 1 回で戻ります。NPC の追加・削除・マス目の書き換えもまとめて戻ります。
  AI 以外（管理者ウィンドウなど）からも変わったデータがあると、`force` を付けない限り戻しません。
  しおりは 100 個まで覚えます。しおりの後の履歴が 500 件より前に流れて消えていると戻せません。
  管理ページの「AI連携」タブの「しおり」から付ける・戻すができます。
  AI に頼む時の例:「しおりを付けてから、レジェンタの料理人の台詞をお試しで変えて」→ 見てやめるなら「さっきのしおりまで戻して」。
  「しおりを付けてから、この町に村人を 3 人足して、お店の品ぞろえを変えて、広場のマス目を石畳にして」→「しおりまで戻して」でも全部戻ります。
- **管理者ウィンドウの「元に戻す」にも出る** … AI の変更は、管理者ウィンドウの画面ごとの「元に戻す」にも「AIキー「名前」さんが変えた」として並びます。
  どちらから戻しても、もう一方の履歴はその後の変更として扱うので、知らずに上書きすることはありません。
- **日ごとの控え** … その日初めてそのデータを変える前に、データファイルを `SBODATA/AiBackup/日付/` に写しておきます（マップは全マップのファイル）。
- **呼び出しの回数** … キーごとに 1 分 300 回までです。

## HTTP の API

すべて POST・JSON です。キーは `Authorization: Bearer <キー>`（または `X-Sbo-Key: <キー>`）ヘッダーで渡します。

| URL | 渡すもの | 返るもの |
| --- | --- | --- |
| `/api/data/schema` | `{}` | 種類と項目の説明 |
| `/api/data/list` | `{kind, q?, match?, offset?, limit?, fields?}` | 一覧（`q` は名前の一部、`match` は `{"isPlayer": true}` のような値の一致） |
| `/api/data/get` | `{kind, id}` | 1 件 |
| `/api/data/tiles` | `{id, x?, y?, w?, h?}` | マップのマス目（`id` は `map` の番号） |
| `/api/data/tiles/set` | `{id, x, y, parts:[[...]], note?, previewId?}` | マス目を書き換える（下見 / 反映の結果） |
| `/api/data/update` | `{kind, updates:[{id, changes}], note?, previewId?}` | 下見 / 反映の結果 |
| `/api/data/add` | `{kind, records:[{...}], note?, previewId?}` | 下見 / 反映の結果 |
| `/api/data/history` | `{kind?, limit?}` | 変更履歴（新しい順。マス目は `kind: "tiles"`） |
| `/api/data/undo` | `{historyId, force?, note?, previewId?}` | 下見 / 反映の結果 |
| `/api/data/bookmark` | `{name, note?}` | しおりを付ける（書き込みもできるキーだけ。データは変わらない） |
| `/api/data/bookmarks` | `{limit?}` | しおりの一覧（新しい順。`changes` はしおりの後の変更の数） |
| `/api/data/rollback` | `{bookmarkId, force?, note?, previewId?}` | しおりまで戻す（下見 / 反映の結果） |

`job` の `levelUp` は、配列で渡すと丸ごと置き換え（件数も変わる）、`{"4": {"dwExp": 100}}` のようなオブジェクトで渡すとその行だけ変わります。
`item` の `byType` を変えると `detail` は 0 から作り直します（種別ごとの項目はスキーマの `detailByType`）。

## MCP のツール

| ツール | 内容 |
| --- | --- |
| `sbo_schema` | 種類と項目の説明 |
| `sbo_list` / `sbo_get` | 一覧・1 件 |
| `sbo_map_tiles` / `sbo_map_tiles_set` | マップのマス目を読む・書き換える（2 段階） |
| `sbo_update` / `sbo_add` | 変更・追加（2 段階） |
| `sbo_history` / `sbo_undo` | 変更履歴・元に戻す（2 段階） |
| `sbo_bookmark` / `sbo_bookmarks` / `sbo_rollback` | しおりを付ける・一覧・しおりまでまとめて戻す（2 段階） |
| `sbo_parts_dex` / `sbo_parts_samples` | パーツ図鑑・組み合わせの見本（下の「マップづくりの手伝い」） |
| `sbo_parts_dex_update` | パーツ図鑑の名前・タグ・説明・見本を変える（書き込みもできるキーだけ） |
| `sbo_plan_save` / `sbo_plan_list` / `sbo_plan_get` / `sbo_plan_delete` | 配置案（ゲームのデータは変えないので、読むだけのキーでも使える） |

読むだけのキーでは、書き込みのツールは出てきません。

## マップづくりの手伝い（パーツ図鑑・配置案）

マップパーツ（マス目に入る番号）には名前も意味も無いので、AI が新しい街やダンジョンを考えられるように、
**パーツ図鑑** と **配置案** を用意しています（`server/node/sbosvr_mapdesign.js`・管理ページの「パーツ図鑑」「配置案」タブ）。
どちらもゲームのデータは変えません。

- **パーツ図鑑**（`SBODATA/SboWebPartsDex.json`）… パーツごとの名前・タグ・使い方のメモと、組み合わせの見本（壁で囲った部屋など）。
  データから自動で分かること（通れるか・上を歩くと消えるか・全マップで使われた回数・よく使われているマップ・よく上下左右に来るパーツ）も一緒に返します。
  ファイルが無い時は、管理ページと一緒に配る `web/account/parts-dex-seed.json`（よく使われている約 230 個の名前と見本 3 つの下書き）から始めます。
  管理ページで直すか、書き込みもできるキーの AI が `sbo_parts_dex_update` で直します。
- **配置案**（`SBODATA/SboWebMapPlans.json`）… AI が `sbo_plan_save` で保存したマス目（最大 128×128・200 件まで）。
  管理ページの「配置案」で、パーツ番号・マス目の線・書き込み（marks）を重ねた画像で見られ、PNG と番号の表（タブ区切り）で持ち出せます。
  気に入った案は、それを見ながら管理者ウィンドウで実際に置きます。

AI への頼み方の例:「sbo_parts_dex で水・壁・床のパーツを調べて、見本も参考に、港町の市場の配置案を 40×30 で作って sbo_plan_save で保存して」

HTTP では `/api/data/parts/dex`・`/api/data/parts/samples`・`/api/data/parts/update`・`/api/data/plans/save`・`/api/data/plans/list`・
`/api/data/plans/get`・`/api/data/plans/delete`（引数は MCP のツールと同じ）。

## ステージング

ステージングはサイト全体に Basic 認証がかかっていますが、AI のツールは `Authorization` ヘッダーに AI 用キーを入れるのでぶつかります。
そのため `/mcp` と `/api/data/*` だけは Basic 認証の外に置いています（AI 用キーで守られます。`deploy/staging/sbo2003.caddy.template`）。

## ファイル

| ファイル | 内容 |
| --- | --- |
| `server/web_data_svr.cpp` | データそのものの読み書き（項目の表・範囲の確かめ・反映と保存） |
| `server/node/sbosvr_data.js` | キー・下見・履歴・元に戻す・HTTP の API・MCP |
| `server/node/sbosvr_mapdesign.js` | パーツ図鑑・配置案（同じキー・同じ MCP にツールを足す） |
| `web/account/admin-design.js` | 管理ページの「パーツ図鑑」「配置案」タブ |
| `web/account/admin-recipe.js` | 管理ページの「レシピ」タブ（作り手とレシピを決める。保存は `/api/admin/recipe/save`） |
| `web/account/admin-npcroute.js` | 管理ページの「NPCの巡回」タブ（NPC の道しるべを置く。保存は `/api/admin/npcroute/set`） |
| `web/account/parts-dex-seed.json` | パーツ図鑑の最初の下書き |
| `web/account/admin.html` | 管理ページの「AI連携」タブ |
