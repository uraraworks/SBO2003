# Sbo2003 本番（SBOP2 と同じ Windows Server の VPS）

GitHub Actions の「production-publish」を手動実行すると、配布 zip を作って Windows で動作を確かめたあと、
**SSH で VPS へ送って入れ替えるところまで** 自動で行います（MangaViewer と同じく、VPS の OpenSSH に鍵でログインします）。
ステージング（`deploy/staging`、GCP・main への push で自動デプロイ）はそのまま残ります。
公開 URL は `https://sbo2003.urara-works.jp/`（SBOP2 の `sbop2.urara-works.jp` と同じく専用のサブドメイン）の想定です。

```
ブラウザ ──https──▶ IIS（VPS、80/443・TLS。SBOP2 と同じ IIS に Sbo2003 用のサイトを足す）
                     └─ Sbo2003 のホスト名のサイト（iis\web.config）
                          └─ すべて ─▶ 127.0.0.1:18090（ブラウザ版の配布と WebSocket /ws）
                                         Sbo2003 サーバー（node\node.exe server\SboSvr.js）
                                         タスクスケジューラ「SBO2003_SboSvr」が起動時に start.ps1 を動かす
```

- サーバーは wasm なので、Windows でもビルドし直さずに同梱の Node.js でそのまま動きます。VPS に Node.js を入れる必要はありません。
- VPS が再起動してもタスクが起動時にサーバーを立ち上げます。サーバーが落ちた時は `start.ps1` が 10 秒後に起動し直します。
- 止める時は必ず `stop.ps1` を使います。`data\stop.request` を置くとサーバーがデータを保存してから終わります
  （Windows ではタスクから起こしたプロセスに Ctrl+C を送れず、`Stop-Process` だと保存されないため）。
- サーバーはパスがルート（`/`）にある前提です（WebSocket は `/ws`）。IIS のサブアプリではなく、専用のホスト名のサイトにします。
- Windows 版クライアント用の TCP（`SboSvr.ini` の `PortNo=2004`）も待ち受けますが、ファイアウォールでは開けません（ブラウザ版だけ）。
  SBOP2 のポート（2007・18080・18081）とはぶつかりません。

## デプロイ（Actions から）

Actions の「production-publish」を手動実行します（「VPS へデプロイする」のチェックを外すと zip を作るだけ）。
`v*` のタグを push した時と、関係するファイルを変える PR の時は、zip を作って確かめるだけでデプロイはしません。
zip は実行結果の Artifacts（`Sbo2003_Server_yyyyMMdd_HHmmss`）にも残ります。

Actions の中では次を行います。

1. ステージングと同じ `deploy/staging/Dockerfile` のビルド段（emsdk）で、サーバーとブラウザ版をビルドする
2. `deploy/production/make-zip.sh` で zip にまとめる（Windows 用の Node.js を公式から取り、ハッシュを確かめて同梱）
3. Windows のランナーで zip を展開し、`start.ps1` で起動 → `/health` とページを確認 → `stop.ps1` で保存して止まることを確かめる。
   続けて `remote-deploy.ps1` で初回の導入・入れ替え・1 つ前の版への戻し・データの入れ替えを一通り確かめる
4. `vps-deploy.sh` で zip を VPS の `C:\SBO2003\upload\` へ scp し、SSH で `remote-deploy.ps1` を動かす。
   再起動の予告 → `stop.ps1` で保存して止める → 今の版を `prev\` に退避 → 展開 → 起動 → `/health` を確認、の順です。
   初回はタスク `SBO2003_SboSvr` の登録と、`sbo2003.env` の作成（雛形から）もここで行います

### 再起動の予告

いきなり切断されて驚かないよう、`remote-deploy.ps1` はサーバーを止める前に、ゲームの全員へシステムメッセージ
（時報と同じ水色の文字。メッセージログにも残る）で知らせます。

```
SYSTEM:【お知らせ】サーバーを更新します。
SYSTEM:数分後に再起動するため、いったん切断されます。
  …（50 秒後）
SYSTEM:まもなく再起動します。数分たったら、もう一度ログインしてください。
```

- 待つ秒数は手動実行の入力「notice_sec」（既定 60、0 で予告しない）。Mac から動かす時は環境変数 `NOTICE_SEC`。
- ゲームに入っている人がいない時（`/health` の `onlinePlayers` が 0）は、知らせるだけで待たずに止めます。
- データの入れ替え（`--data`）と戻す（`--rollback`）の時は「サーバーのメンテナンスを行います。」になります。
- 仕組み: `data\notice.request` に UTF-8 で 1 行 1 メッセージを書いて置くと、サーバーが読んで全員に流し、ファイルを消します
  （`start.ps1` が環境変数 `SBO2003_NOTICE_FILE` で場所を渡す。`server/node/sbosvr_net.js`）。
  手で知らせたい時も、このファイルを置けば流れます。

### 必要な設定（Settings → Secrets and variables → Actions）

| 種類 | 名前 | 中身 |
|---|---|---|
| Variable | `SBO2003_VPS_HOST` | VPS のホスト名か IP（未設定の間はデプロイしない） |
| Variable | `SBO2003_VPS_USER` | SSH ユーザー（Administrators に入っていること。MangaViewer と同じでよい） |
| Variable | `SBO2003_VPS_PORT` | SSH のポート（22 なら不要） |
| Variable | `SBO2003_VPS_HEALTH_URL` | 外から確かめる URL（任意。`https://sbo2003.urara-works.jp/health`） |
| Variable | `SBO2003_VPS_IIS_HOST` | IIS のサイトのホスト名（未設定なら `sbo2003.urara-works.jp`。デプロイのたびに `setup-iis.ps1` でサイトを確かめ、足りないものだけ足す） |
| Secret | `SBO2003_VPS_SSH_KEY` | SSH 秘密鍵（`base64 -i ~/.ssh/urara_vps_deploy` の1行がおすすめ） |
| Secret | `SBO2003_VPS_KNOWN_HOSTS` | `ssh-keyscan -p <ポート> <ホスト>` の出力 |

VPS 事業者のパケットフィルタで SSH（22）を特定の IP だけに絞っている時は、GitHub Actions からは届きません。
その時は Mac から `deploy/production/vps-deploy.sh <zip>` を動かします（`deploy.env.example` を `deploy.local.env` にコピーして設定）。

### 戻す

```bash
deploy/production/vps-deploy.sh --rollback    # Mac から。1 つ前の版（prev\）に戻す。data\ はそのまま
```

VPS で直接戻す時は `C:\SBO2003\remote-deploy.ps1 -Rollback` です。

手元（Mac・Linux）で作る時は、`tools/server/build.sh release` と `tools/web/build.sh release` のあとに:

```bash
deploy/production/make-zip.sh out/server-release out/web/dist publish
```

## 初回のセットアップ（VPS で 1 回だけ）

SBOP2 のサイトで IIS・URL Rewrite・Application Request Routing（ARR）・WebSocket プロトコルは用意済みの前提です。
SSH（OpenSSH Server と鍵）は MangaViewer のデプロイで用意済みの前提です。

SSH でデプロイする時の手順:

1. DNS で `sbo2003.urara-works.jp` を VPS に向ける
2. 上の設定をしてから Actions の「production-publish」を手動実行する（`C:\SBO2003` への展開・タスク登録・起動まで行われる）
3. VPS の `C:\SBO2003\sbo2003.env` に `SBO2003_ADMIN_PASSWORD` を書き、`.\stop.ps1` → `Start-ScheduledTask -TaskName SBO2003_SboSvr` で起動し直す
4. 管理者の PowerShell で IIS のサイトを作る（http と、証明書があれば https も付ける）
   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\setup-iis.ps1 -HostName sbo2003.urara-works.jp
   ```
   証明書が無いと言われたら、SBOP2 のサイトと同じやり方で `sbo2003.urara-works.jp` の証明書を取って、もう一度実行します
5. `https://sbo2003.urara-works.jp/` を開いてログイン画面が出れば完了です

手で展開する時の手順:

1. **展開する**: zip を `C:\SBO2003` に展開します（別の場所でもよい。以下は `C:\SBO2003` で書きます）。
2. **設定ファイル**: `sbo2003.env.example` を `sbo2003.env` にコピーし、`SBO2003_ADMIN_PASSWORD`（ゲームの管理者 `administrator` のパスワード）を決めて書きます。
3. **タスクを登録**: 管理者の PowerShell で
   ```powershell
   cd C:\SBO2003
   powershell -NoProfile -ExecutionPolicy Bypass -File .\install-task.ps1
   Start-ScheduledTask -TaskName SBO2003_SboSvr
   ```
   `log\start.log` に「サーバーを起動しました」が出て、`curl.exe http://127.0.0.1:18090/health` が `ok` を返せば動いています。
   初回は `seed\` から `data\` に元データがコピーされます。
4. **IIS のサイトを作る**: IIS マネージャーで新しいサイトを追加します。
   - 物理パス: `C:\SBO2003\iis`（`web.config` が入っています）
   - バインド: Sbo2003 用のホスト名（DNS をこの VPS に向けておく）で http と https。証明書は SBOP2 と同じやり方で取ります
   - ARR のプロキシが有効になっていること（サーバーノードの「Application Request Routing Cache」→「Server Proxy Settings」→「Enable proxy」）。SBOP2 で有効にしてあればそのままで大丈夫です
5. **確かめる**: `https://<Sbo2003 のホスト名>/` を開いてログイン画面が出れば完了です。アカウントは最初のログインで作られます（`FreeAcount=1`）。

### 手元やステージングのデータを持っていく時

データは `SBODATA\` フォルダと `SboSvr.ini` の 2 つだけです。サーバーを `stop.ps1` で止めている間に、
`C:\SBO2003\data\SBODATA\` と `C:\SBO2003\data\SboSvr.ini` になるよう置けば、次の起動からそのデータで動きます
（動いている間に置くと、止まる時の保存で上書きされます）。初回の起動より前に置いておけば、`seed\` からのコピーは行われません。

- 手元（Mac）の `tools/server/run.sh` で遊んだデータは `out/server-data/` にあります（`SboSvr.pid` は要りません）。
- 管理者パスワードは起動のたびに `sbo2003.env` の値で上書きされます。

ステージングのデータを本番に持っていく時は、Mac から次を動かします（`deploy.local.env` に `STAGING_TARGET` も書いておく）。

```bash
deploy/production/move-staging-data.sh
```

ステージングを少しの間止めて（保存してから）ボリューム `sbo2003-staging_data` の `SBODATA/` と `SboSvr.ini` を取り出し、
本番のサーバーを止めて今のデータを `C:\SBO2003\backup\` に zip で控えてから置き換え、起動し直します。
プレイヤーのデータ（パスワードを含む）は Mac を通るだけで、GitHub には上げません。手元の一時ファイルも最後に消します。

### 本番のゲームデータでローカル用の初期データを更新する時

ローカルでサーバーを試す時（`tools/server/run.sh`・クラウドの各スレッドでの確認）は `Release/SBODATA/` を使います。
これを本番の今のマップ・アイテム・NPC などにそろえる時は、Mac で次を動かします（サーバーが未ビルドなら先にビルドします）。

```bash
deploy/production/pull-prod-seed.sh
git commit -m "初期データを本番の最新に更新"   # git add はスクリプトが行う
```

本番のサーバーは止めずに `C:\SBO2003\data\SBODATA\` の `SboData*.dat` を写して持ってきて（本番は 30 分ごとに保存するので、最大 30 分前の状態）、
`tools/server/make-seed.sh` でキャラ情報を NPC だけにしてから `Release/SBODATA/` を入れ替えます。
預かり所（`SboDataLeaveInfo.dat`）と Web 用の JSON（アカウント・ご意見・AI の鍵など）は持ってこないので、
プレイヤーのデータは入りません。手元と VPS の一時ファイルも最後に消します。

## 再デプロイ（手で行う時）

```powershell
cd C:\SBO2003
.\stop.ps1                                  # データを保存してから止まる
Expand-Archive -Force <新しい zip> C:\SBO2003
Start-ScheduledTask -TaskName SBO2003_SboSvr
```

`data\`・`log\`・`sbo2003.env` は zip に入っていないので、上書き展開しても残ります。
古い版のファイルを残したくない時は、展開の前に `server\`・`web\` を消してもかまいません。

## 運用

| やりたいこと | コマンド（C:\SBO2003 で） |
|---|---|
| 止める（保存してから） | `.\stop.ps1` |
| 起動する | `Start-ScheduledTask -TaskName SBO2003_SboSvr` |
| 起動・停止の記録 | `Get-Content log\start.log -Tail 20` |
| サーバーのログ | `log\sbosvr-<日時>.out.log`（エラーは `.err.log`。30 日で消える） |
| バックアップ | 止めてから `Compress-Archive data\* backup\sbo2003-data-<日付>.zip` |

`Stop-ScheduledTask` や `Stop-Process` でいきなり止めると、最後の保存のあとのデータが失われます。

## ファイル

| ファイル | 役割 |
|---|---|
| `make-zip.sh` | ビルド結果・元データ・スクリプト・Node.js を zip にまとめる |
| `start.ps1` | 初回のデータコピー、管理者パスワードの設定、サーバーの起動と落ちた時の起動し直し |
| `stop.ps1` | 停止ファイルを置いて、保存して止まるのを待つ |
| `install-task.ps1` | 起動時に自動で立ち上がるタスクを登録する |
| `setup-iis.ps1` | IIS のサイト（http と、証明書があれば https）を作る |
| `remote-deploy.ps1` | VPS で、新しい zip・データへの入れ替えと 1 つ前の版への戻しを行う |
| `vps-deploy.sh` | zip やデータを SSH で VPS に送り、`remote-deploy.ps1` を動かす（Actions からも Mac からも） |
| `move-staging-data.sh` | ステージングのデータを本番へ持っていく（Mac から） |
| `pull-prod-seed.sh` | 本番のゲームデータから `Release/SBODATA/`（ローカル用の初期データ）を作り直す（Mac から） |
| `deploy.env.example` | Mac から動かす時の接続設定の雛形（`deploy.local.env` にコピー） |
| `set-admin-password.js` | `SboSvr.ini` の管理者パスワードを書き換える（ステージングの `entrypoint.sh` と同じ処理） |
| `web.config` | IIS サイトの設定（zip では `iis\web.config`） |
| `sbo2003.env.example` | 設定の雛形 |
| `README.txt` | zip に入れる短い説明 |
| `../../.github/workflows/production-publish.yml` | zip を作って Windows で動作確認する |
