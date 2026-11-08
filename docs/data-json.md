# サーバーデータの JSON 書き出し・取り込み

サーバーの設定データを種類ごとの JSON ファイルに書き出し、非公開リポジトリ（SBO2003-private）の `data/` で管理します。
直した JSON はサーバーへ取り込めます。中身はネタバレになるので、公開リポジトリには置きません。

使うのは `tools/data/sbo-data.mjs`（Node.js 18 以上、追加のパッケージは要りません）です。
中身は [AI 用データ API](ai-data.md) を呼んでいるだけなので、取り込みも AI の書き込みと同じく
「下見 → 確定」で、変更履歴に残り、管理ページ「AI連携」から元に戻せます。

## 使い方

```sh
# 書き出し（サーバー → 非公開リポジトリの data/*.json）。読むだけのキーで足ります
node tools/data/sbo-data.mjs export --target prod

# 見比べ（data/*.json とサーバーの違いを出すだけ。何も変えません）
node tools/data/sbo-data.mjs diff --target staging

# 取り込み（違う項目だけをサーバーで下見 → 確認して y で反映）。書き込みもできるキーが要ります
node tools/data/sbo-data.mjs import --target staging --note "料理のレシピを追加"
```

- `--target prod` は本番、`--target staging` はステージング、`--url http://localhost:8080` のように直接も指定できます（環境変数 `SBO2003_DATA_URL` でも）。
- キーは環境変数 `SBO2003_DATA_KEY` か `--key-file ファイル` で渡します（管理ページ /admin の「AI連携」で発行）。
  クラウドの環境では `SBO2003_AI_KEY`（本番）・`SBO2003_STAGING_AI_KEY`（ステージング）も使います。
- `--kinds recipe,maker` で種類をしぼれます。`--dir フォルダ` で置き場所を変えられます（無ければ `tools/server/find-data.sh` と同じ探し方で非公開リポジトリの `data/`）。
- `--yes` を付けると確認を飛ばして反映します。

## 書き出す種類

| 種類 | 取り込み |
|---|---|
| magic・magicUser・equUser・job・stTmp・item・maker・recipe | できる |
| char（保存される NPC だけ） | できる（`npcId` で突き合わせ。下の「NPC」） |
| map（マス目は除く）・mapParts・efc・comEfc・subGame | できる（subGame は `detail` の項目だけ） |
| leave | 書き出さない（プレイヤーの預かり所） |

マップのマス目はこのツールでは扱いません（大きいので）。AI に頼むか、API の `/api/data/tiles/set` を使います。

## 取り込みのきまり

- 記録は番号（`id`）で突き合わせ、サーバーと違う項目だけを送ります。読むだけの項目（職業の `dwJobID` など）は送りません。
- サーバーより大きい番号の記録は「追加」になります。番号はサーバーの件数からの続き番号にしてください。
  追加を元に戻せるのは NPC・作り手・レシピだけです（マップパーツ・エフェクトの追加は戻せません）。
- ファイルから消した記録は、サーバーからは消しません。作り手・レシピをやめる時は `byState` を 0（削除）か 2（停止中）にします。
- 中身の確かめ（知らない項目・値の範囲など）はサーバーが下見の時にします。1 つでもおかしければ何も反映しません。
- NPC（`char.json`）は番号（`id`）ではなく `npcId` で突き合わせます（`id` は再起動で動くので書き出しません）。
  - `npcId` の無い記録は新しい NPC として足します（`dwMapID`・`dwX`・`dwY` が要る）。足した後は export し直して `npcId` を付けてください。
  - サーバーに無い `npcId` は、その番号で足し直します（消した NPC を戻す時）。
  - ファイルから消した NPC はサーバーからは消しません。読むだけの項目（能力・持ち物など）と向きは書き出しません。歩き回る NPC は今の位置（`dwX`・`dwY`）も書き出しません。
- ファイルは 1 記録＝項目ごとに 1 行（レベルアップ情報のような配列は 1 要素 1 行）にしてあるので、git の差分が読みやすくなります。

## 例: ステージングで試したレシピを本番へ

```sh
node tools/data/sbo-data.mjs export --target staging --kinds maker,recipe   # ステージングの中身を data/ へ
node tools/data/sbo-data.mjs import --target prod --kinds maker,recipe      # 本番へ（下見を見て y）
```
