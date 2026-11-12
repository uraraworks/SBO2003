# ドット絵工房（/kobo）

プレイヤーみんなでゲームのドット絵を描き合う仕組み。GitHub のドット絵版。

- 管理者が **お題**（新しく描くか・ゲーム画像のどこを描き直すか・元イラスト・見本）を出す
- プレイヤーは **案** を描いて出す。一から描くか、誰かの「出した」案を元に描き足す
  （案は上書きせず、親つきで足していくので **系譜**（木）になる）
- 案には **作業中**（本人が保存のたびに上書き。見せるかは本人が選ぶ）と **出した**（固定。描き足しの元にできる）がある
- 案への **コメント** は誰でも書けるが、**返事できるのはその案を描いた本人と管理者だけ**（プレイヤー同士のやりとりは無し）。
  本人は自分の案へのコメントを隠せる（隠したものは管理者に届く）
- **なじんでる**（応援）は 1 人 1 案 1 回。採用の参考にするだけ
- プレイヤーは **提案**（「こういうのどう？」）を出せる。見えるのは管理者だけ。管理者が「お題にする」とお題になる
- 管理者が案を 1 つ **採用** する。描いた人・元になった案の人・提案した人が協力者として残る
- 名前はキャラ名か匿名かを案・提案ごとに選ぶ（匿名はみんなには「匿名さん」、管理者には誰か分かる）。
  匿名で出した提案はお題になっても匿名のまま
- 出す時に同意をもらう（自分で描いた／GPLv3 で配布されることがある／他の人が描き足してよい／採用されないこともある）

設計の話し合い: https://claude.ai/artifact/KBRUbgm5jpzTAPYxzxTB6U

## 絵のきまり

- 絵はすべて **8bit パレット PNG**（colortype 3）。パレットはお題の元のゲーム画像のパレット（256 色）そのまま。
  **透明はパレットの黒 (0,0,0)**（番号 0 ではない。dot-core.js の findTransparentIndex）。
- 案の PNG は「お題の描く範囲」と同じ幅・高さで、PLTE がお題のパレットとバイト単位で同じでないと受け付けない。
- プレイヤーが選べる色は、お題の `colors`（パレットの全色から、透明の番号以外の真っ黒を除いたもの。真っ黒はゲームではどれも透明になるため）だけ。見本で使っている色は `sampleColors` で、描く画面の先頭に「見本の色」として並ぶ。

## 保存

- 一覧: `SBODATA/SboWebKobo.json`（`{version:1, tasks:[], works:[], comments:[], cheers:[], proposals:[]}`）
- 画像: `SBODATA/kobo/<id>.png`（`<id>` は英数字 12 文字の乱数。JPEG の元イラストだけ `.jpg`）
- 書き込みはご意見箱と同じく「別名で書いてから rename」。

### お題（task）

```
{
  id, title, desc,                 // desc は 2000 字まで
  kind: 'char' | 'part' | 'effect',// NPC・敵 ／ 背景パーツ ／ エフェクト
  state: 'open' | 'judging' | 'done' | 'hidden',  // 募集中 ／ 選考中 ／ 採用済み ／ 隠す（管理者だけ見える）
  fresh: true | false,             // true = 新しく描く（白紙から。今の絵 base は無し・x,y は 0）。
                                   //   ゲーム画像のどこに置くかは採用後に管理者が合成する。sheet はパレットと見本を借りるだけ
                                   // false = ゲーム画像の一部を描き直す（採用案を sheet の x,y に貼れる）
  sheet: 'char05.png',             // 元のゲーム画像（SboGrpData/grp）
  x, y, w, h,                      // 描く範囲（元画像の中の位置。ピクセル。fresh の時は大きさだけ = コマ × コマ数）
  cw, ch,                          // 1 コマの大きさ（16 など）
  preview: 'walk' | 'tile' | 'anim',
      // walk … 1 行に「向き 4 つ × 2 コマ」が並ぶキャラ（char01〜16 の 1 行 = 128×16）。
      //         向き d のコマは列 d*2 と d*2+1。向きの並びは 0=上 1=下 2=左 3=右。
      //         大きいキャラ（cw×ch が 32×32 など）も同じ並び（コマの大きさが違うだけ）
      // tile … 範囲を縦横に 3×3 並べて継ぎ目を見る（背景パーツ）。コマが複数あればアニメで切り替え
      // anim … 範囲のコマを左上から右へ、上から下へ順に再生（エフェクト）
  frameMs: 200,                    // アニメ 1 コマの時間
  palette: [[r,g,b], ...256],      // 元画像のパレット
  colors: [番号, ...],             // 選べる色（透明以外の真っ黒を除いた全色）
  sampleColors: [番号, ...],       // 見本（描き直しなら今の絵も）で使っている色。描く画面で先頭に並べる目安
  transparent: 番号,               // 透明の番号（パレットの黒）
  base: 画像id,                    // 描く範囲の今の絵（お題を作った時に切り出したもの）。「今の絵から描く」の元
  bg: 画像id | '',                 // 等倍で並べて見る時の背景（16×16 程度のマップパーツ 1 マス。敷き詰めて使う）
  ref: 画像id | '',                // 元イラスト（PNG/JPEG。何を描くか）
  samples: [画像id, ...],          // 見本（どう描くか）。同じ画像の既存キャラなど 0〜4 個。パレット PNG
  proposalId: '' | 提案id,         // 提案から作ったお題
  adopted: null | { workId, at, credits:[{role:'draw'|'base'|'proposal', name, anonymous}] },
  at, updatedAt
}
```

### 案（work）

```
{
  id, taskId,
  parent: '' | 案id,               // 元にした「出した」案。'' は一から（お題の今の絵 base から始めても '' でよい）
  account, name,                   // name は出した（最後に保存した）時のキャラ名
  anonymous: bool,
  note: '',                        // ひとこと（200 字まで）
  status: 'wip' | 'posted',
  share: bool,                     // wip を みんなに見せるか（posted は常に見える）
  img: 画像id,
  hidden: bool,                    // 管理者が隠した
  deleted: bool,                   // 管理者が消した（子がいる時は系譜を保つため印だけ残し、絵は消す）
  agree: 1,                        // 同意した文面の版
  at, updatedAt, postedAt
}
```

### コメント（comment）

```
{ id, workId, account, name, anonymous, admin: bool, body, replyTo: '' | コメントid,
  hiddenByAuthor: bool, hidden: bool, at, ip }
```

- `replyTo` を付けられるのは、その案の本人（account が同じ）と管理者だけ。
- 本人が隠すと `hiddenByAuthor`（みんなには見えない・管理者には「本人が隠した」として見える）。

### なじんでる（cheer）: `{ workId, account, at }`（自分の案には付けられない）

### 提案（proposal）

```
{ id, account, name, anonymous, kind, body, img: '' | 画像id,
  status: 'new' | 'later' | 'declined' | 'tasked' | 'withdrawn', reply: '', replyAt, taskId: '', at, editedAt, imgAt（ラフ絵を変えた時刻。返事の絵より新しければ管理人はラフ絵に描き足す）, withdrawnAt, ip }
```

- 本人はお題になる前（`tasked` 以外）なら直せる・取り下げられる。見送り（`declined`）を直すと `new` に戻る。
- 取り下げると中身とラフ絵を消して `withdrawn` の印だけ残す（1 日の回数に数えるため。1 日たったら次の提案の時に消える）。本人・管理者の一覧には出ない。
- 下書きはこの端末だけ（localStorage `kobo_propose_draft:<提案id か new>`。書いている間も自動で残し、送ったら消す）。

## API（すべて POST・JSON。/api/kobo/* と /api/admin/kobo/*。sbosvr_net.js が sbosvr_kobo.js へ回す）

画像は `GET /kobo-img/<id>.png`（と `.jpg`）。`Cache-Control: public, max-age=31536000, immutable`（画像は書き換えず、新しい id を作る）。
受け取る JSON は 1MB まで（画像は base64 の `data` 文字列。PNG は 256KB・元イラストは 1MB まで）。

エラーは `{error: 'コード'}`。主なコード: `invalid_token`(401) `no_char` `session`(401) `not_found`(404) `wait`（`waitSeconds` 付き）`too_many` `long` `empty` `bad_png` `bad_size` `bad_palette` `closed`（お題が募集中でない）`agree`（同意なし）`not_allowed`。

### プレイヤー（token は localStorage の `sbo_device_token`）

| URL | 送る | 返す |
| --- | --- | --- |
| `/api/kobo/me` | `{token}` | `{account, name, canPost, admin:false}` |
| `/api/kobo/tasks` | `{token?}` | `{tasks:[taskView + {works:出した数, wip:見せている作業中の数, latestImg, adoptedImg}]}`（hidden は除く。open→judging→done の順、中は新しい順） |
| `/api/kobo/task` | `{id, token?}` | `{task, works:[workView], comments:[commentView], myCheers:[workId]}` works は 出した＋見せている作業中＋自分の作業中（隠し・消しは除く。ただし子がいる消した案は `{id, parent, deleted:true}` だけ） |
| `/api/kobo/save` | `{token, taskId, workId?, parent?, data(PNG base64), note, anonymous, share, submit, agree}` | `{ok, work}` |
| `/api/kobo/delete` | `{token, id}` | `{ok}`（自分の作業中だけ消せる） |
| `/api/kobo/comment` | `{token, workId, body, replyTo?}` | `{ok, comment}` |
| `/api/kobo/hide-comment` | `{token, id, hide}` | `{ok}`（自分の案へのコメントだけ） |
| `/api/kobo/cheer` | `{token, workId, on}` | `{ok, cheers}` |
| `/api/kobo/propose` | `{token, kind, body, data?(PNG base64), anonymous, agree}` | `{ok, proposal}` |
| `/api/kobo/my-proposals` | `{token}` | `{proposals:[...]}`（返事も見える） |
| `/api/kobo/proposal-edit` | `{token, id, kind, body, anonymous, agree, data?(PNG base64), removeImg?}` | `{ok, proposal}`（自分の・お題になる前だけ。`data` で差し替え、`removeImg` で外す、どちらも無ければラフ絵はそのまま。お題になった後は `tasked`） |
| `/api/kobo/proposal-withdraw` | `{token, id}` | `{ok}`（自分の・お題になる前だけ） |

save の決まり:
- `workId` なし → 新しい案を作る（`parent` は同じお題の posted で消されていない案だけ）。`workId` あり → 自分の wip を上書き（posted は上書きできない）。
- `submit:true` で posted にする（`agree:true` が要る。以後変えられない）。
- 制限: 新しい案は 1 アカウント 1 分 1 件・1 日 30 件。wip の上書きは 1 アカウント 20 秒に 1 回。お題が `open` の時だけ。
- 1 アカウントの wip は 1 お題につき 3 つまで。
- コメント: 300 字まで、1 アカウント 1 分 3 件・1 日 50 件。提案: 1000 字、1 日 3 件。

見せ方（View）:
- 匿名の人は `name:''`, `anonymous:true`。`mine:true` は自分のもの。
- workView: `{id, parent, name, anonymous, note, status, share, img, cheers, comments(数), at, updatedAt, postedAt, mine}`
- commentView: `{id, workId, name, anonymous, admin, body, replyTo, at, mine, hiddenByAuthor?}`（本人の案についた隠したコメントは本人には `hiddenByAuthor:true` で見える）

### 管理者（session は /api/admin/login と同じ。Cookie でもよい）

| URL | 送る | 返す |
| --- | --- | --- |
| `/api/admin/kobo/tasks` | `{session}` | `{tasks:[adminTaskView]}`（hidden も含む）＋ `counts:{proposals:新着数, hiddenComments:本人が隠した数}` |
| `/api/admin/kobo/task` | `{session, id}` | `{task, works, comments}`（全部・account 付き） |
| `/api/admin/kobo/task-save` | `{session, id?, title, desc, kind, state, sheet, x,y,w,h, cw,ch, preview, frameMs, palette, colors, sampleColors?, transparent, fresh?, baseData, bgData?, refData?, refType('png'|'jpg'), samplesData?[], proposalId?}` | `{ok, task}`（`fresh` の時は `baseData` 不要で、前の今の絵は外れる。`*Data` は base64。編集時に省いたものは元のまま。`samplesData` を送ると丸ごと入れ替え。提案から作ると提案は `tasked` に） |
| `/api/admin/kobo/task-delete` | `{session, id}` | `{ok}`（案・コメントも消える） |
| `/api/admin/kobo/work` | `{session, id, hidden?, deleted?}` | `{ok}` |
| `/api/admin/kobo/comment` | `{session, workId, body, replyTo?}` | `{ok, comment}`（admin:true、名前は「管理人」） |
| `/api/admin/kobo/comment-update` | `{session, id, hidden?, delete?}` | `{ok}` |
| `/api/admin/kobo/adopt` | `{session, taskId, workId|''}` | `{ok, task}`（'' で採用を取り消し。採用すると state=done・credits を作る） |
| `/api/admin/kobo/proposals` | `{session, status?}` | `{proposals:[...account 付き]}` |
| `/api/admin/kobo/proposal-update` | `{session, id, status?, reply?, replyData?(PNG base64), replyImg?('' で外す)}` | `{ok, proposal}`（`replyData` は管理人が描き足した返事の絵 → `replyImg`） |
| `/api/admin/kobo/task-image` | `{session, taskId, slot:'base'|'sample', index?, data}` | `{ok, task}`（お題の今の絵・見本を直す。大きさ・パレットはお題と同じ。見本の色 sampleColors は作り直す） |
| `/api/admin/kobo/fix` | `{session, taskId, parent, data, note}` | `{ok, work}`（作監修正：元の案の子に `admin:true`・名前「管理人」の出した案を作る。協力者には入らない） |

管理人が描く時は、描く画面を `/kobo-draw?admin=fix&task=&from=案id` ／ `admin=base&task=` ／ `admin=sample&task=&n=` ／ `admin=proposal&id=提案id` で開く（管理画面のログイン Cookie で API を呼ぶ）。

credits の作り方: 採用した案から parent をたどった全部の案の人（同じ人は 1 回。役は採用した案の人が `draw`、ほかは `base`）＋提案の人（`proposal`）。名前はそれぞれの案・提案で選んだ表示（匿名なら `anonymous:true`, name は ''）。

## 画面

- `/kobo`（web/account/kobo.html・kobo.js）… お題の一覧 → お題のページ（元イラスト・見本・系譜・案の拡大・コメント・なじんでる）。ログインしなくても見られる
- `/kobo-draw`（web/account/kobo-draw.html・kobo-draw.js）… 描く画面（`?task=ID&from=WORKID|base|blank&work=WIPID`）。PC とスマホ
- `/kobo-propose`… 提案（kobo.html の中のパネルでもよい）
- 管理ページ `/admin` の「ドット絵工房」タブ（web/account/admin-kobo.js）
- 入口: ゲームのページのフッター・「おかえりなさい」カード（ご意見箱の隣）

## サーバー側の補足（実装で決めたこと）

- 画像は 1 枚ごとに新しい id（書き換えない）。作業中を上書きすると前の絵は消える。案・お題・提案のラフ絵は PNG 256KB まで（ラフ絵は 512×512 まで・パレットでも RGBA でもよい）、元イラストは 1MB まで。
- 受け取る JSON はプレイヤーの API が 1MB まで、管理者の API は 4MB まで（元イラストの base64 が入るため）。
- 案の PNG の確認で、お題の `colors` に無い番号を使っていた時も `bad_palette`。お題の基準・背景・見本はパレット PNG（8bit・colortype 3）として正しければよい（パレット・大きさは比べない）。
- 管理者の task-save で、`bgData`・`refData` に空文字 `''` を送ると外す（省くと元のまま）。`transparent` は `colors` に自動で足される。`frameMs` は 30〜10000。w,h は 512 まで。
- コメントは `anonymous` を付けて送ると匿名になる（省くと名前を出す）。管理者のコメントは 1000 字まで。コメントを管理者が消すと、その返事も消える。
- `counts.hiddenComments` は「本人が隠していて、管理者がまだ `hidden` にしていないコメント」の数。
- 作業中が 3 つ埋まっていても `submit:true` で新しく出すことはできる。wip が上限の時は `{error:'too_many', max:3}`。制限の `wait` には `waitSeconds`（コメントが 1 分 3 件の時は 60）が付く。
- 案を管理者が消すと、その案だけを元にしていた消し済みの親も一緒にきれいにする。採用中の案を消すと採用は外れ、お題は `judging` に戻る。採用の取り消し（`workId:''`）も `judging` に戻る。
- credits には、消された親の案の人も含める。
- お題を消すと、そこから作った提案は `later` に戻り `taskId` が空になる。
- 一覧・案の数は `works`/`wip` が出した数・見せている作業中の数（隠し・消しは除く）。
- 制限は `Module['sboKoboLimits']` で変えられる（確認用）。

## 確認のしかた

wasm をビルドしなくても、Node だけで API と画面を確かめられる（Node 18 以上）。

```
node tools/kobo/stub-server.js --port 8790 --data /tmp/kobo-stub [--fast]
```

- `server/node/sbosvr_kobo.js` をそのまま読み込む簡易サーバー。保存は `--data` の下の `SBODATA/`。`--port 0` なら空いているポート（起動時に `listening http://127.0.0.1:ポート` を表示）。
- 端末トークン（`localStorage` の `sbo_device_token` に入れる）: `tokA`＝アリス、`tokB`＝ボブ、`tokC`＝キャラ無し。それ以外は無効。
- 管理者は session `'admin'`、または Cookie（`/admin` を開くと自動でログイン済みになる）。
- `--fast` を付けると案の保存・コメントの間隔の制限をなくす（1 日あたりの上限は残る）。画面の確認には付けると楽。
- ページ: `/kobo`・`/kobo-draw`・`/admin`・`/dot`・そのほか `web/account`・`web/changes`・`web/shell` の中のファイル。`/admin-grp/<名前>.png` と `/admin-grp/index.json`（`{"files":[...]}`）は `SboGrpData/grp` から返す。未知の `/api/*` は 200 で `{}`。

```
node tools/kobo/test-api.js
```

- stub-server を一時フォルダで 2 つ（制限なし・制限あり）起動し、お題の作成から採用・PNG の確認・制限までを `fetch` で一通り試す。全部通れば終了コード 0。
