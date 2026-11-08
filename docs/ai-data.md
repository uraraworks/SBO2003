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

セーブデータの残りは **読むだけ** です（バランス調整のシミュレーションやシナリオ作りの手伝い用）。

| 種類（kind） | 中身 |
| --- | --- |
| `char` | キャラ全員（プレイヤー・NPC）。能力・持ち物・装備・位置、NPC の台詞（`szNpcSpeech`）、転職前の職業で覚えていた特技（`jobMagic`）など。`isPlayer`・`online` でプレイヤーか・ログイン中か分かる |
| `map` | マップの情報・イベント（`events`）・お店（`shops`）・メッセージ（`messages`）・アイテム配置（`items`） |
| `mapParts` | マップパーツ（マス目に入る番号）の表示・当たり判定・アニメのコマ（`anime`） |
| `efc` | エフェクト情報 |
| `comEfc` | 吹き出し情報 |
| `leave` | 預かり所（キャラの `dwLeaveID` がこの番号） |
| `subGame` | 爆弾ゲーム・迷路・ダンジョン・戦場ゲーム・すごろくの設定 |

マップのマス目（パーツの番号・フラグ・屋根）は `sbo_map_tiles`（`/api/data/tiles`）で 2 次元の配列として読めます（1 回最大 128×128）。

**アカウント名・パスワード・IP アドレス・MAC アドレスは、どのキーでも出しません。**

項目名はソースの変数名と同じです（`szName`・`wMana` など）。意味・範囲・他のデータの番号の参照は
`sbo_schema`（`/api/data/schema`）で見られます。

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
  その後に別の変更があったデータは、`force` を付けない限り戻しません。追加した作り手・レシピは削除（`byState` 0）にして戻します。
  それ以外の種類の追加は消せないので戻せません。
  管理ページの「AI連携」タブからも履歴を見て「元に戻す」ができます。
- **しおり（お試しの区切り）** … お試しで作り変える前に、名前を付けて「しおり」を付けておくと、
  その後の変更を「このしおりまで戻す」で種類をまたいでまとめて元に戻せます（下見 → 確定）。
  記録ごとに「しおりの時の姿」に戻すので、途中で何回変えていても 1 回で戻ります。
  AI 以外（管理者ウィンドウなど）からも変わったデータがあると、`force` を付けない限り戻しません。
  しおりは 100 個まで覚えます。しおりの後の履歴が 500 件より前に流れて消えていると戻せません。
  管理ページの「AI連携」タブの「しおり」から付ける・戻すができます。
  AI に頼む時の例:「しおりを付けてから、レジェンタの料理人の台詞をお試しで変えて」→ 見てやめるなら「さっきのしおりまで戻して」。
- **管理者ウィンドウの「元に戻す」にも出る** … AI の変更は、管理者ウィンドウの画面ごとの「元に戻す」にも「AIキー「名前」さんが変えた」として並びます。
  どちらから戻しても、もう一方の履歴はその後の変更として扱うので、知らずに上書きすることはありません。
- **日ごとの控え** … その日初めてそのデータを変える前に、データファイルを `SBODATA/AiBackup/日付/` に写しておきます。
- **呼び出しの回数** … キーごとに 1 分 300 回までです。

## HTTP の API

すべて POST・JSON です。キーは `Authorization: Bearer <キー>`（または `X-Sbo-Key: <キー>`）ヘッダーで渡します。

| URL | 渡すもの | 返るもの |
| --- | --- | --- |
| `/api/data/schema` | `{}` | 種類と項目の説明 |
| `/api/data/list` | `{kind, q?, match?, offset?, limit?, fields?}` | 一覧（`q` は名前の一部、`match` は `{"isPlayer": true}` のような値の一致） |
| `/api/data/get` | `{kind, id}` | 1 件 |
| `/api/data/tiles` | `{id, x?, y?, w?, h?}` | マップのマス目（`id` は `map` の番号） |
| `/api/data/update` | `{kind, updates:[{id, changes}], note?, previewId?}` | 下見 / 反映の結果 |
| `/api/data/add` | `{kind, records:[{...}], note?, previewId?}` | 下見 / 反映の結果 |
| `/api/data/history` | `{kind?, limit?}` | 変更履歴（新しい順） |
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
| `sbo_map_tiles` | マップのマス目 |
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
