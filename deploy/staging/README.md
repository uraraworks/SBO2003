# Sbo2003 ステージング（SBOP2 のステージングに相乗り）

SBOP2 のステージング用ホスト（Google Cloud の e2-micro）に、Sbo2003 のサーバー（Node.js 版）とブラウザ版を同居させる一式です。
80/443 と証明書は SBOP2 の Caddy が受け持ち、Sbo2003 は自分のサイト定義を置くだけです。

```
ブラウザ ──https──▶ SBOP2 の Caddy（sbop2-staging-caddy-1、80/443・TLS）
                     ├─ SBOP2 のドメイン      ─▶ SBOP2 の SboSvr（いままでどおり）
                     └─ Sbo2003 のドメイン（~/caddy-sites/sbo2003.caddy）
                          ├─ /ws        ─▶ sbo2003-staging:8080（ゲームの WebSocket。Basic 認証なし）
                          └─ それ以外   ─▶ sbo2003-staging:8080（ブラウザ版。Basic 認証あり）
                                              Sbo2003 サーバー（Docker、Node.js 版）
```

- Caddy と Sbo2003 のコンテナは共有ネットワーク `staging-shared` でつながります（SBOP2 の `deploy/staging/README.md` の「他プロジェクトとの相乗り」）。
- Sbo2003 のコンテナはポートを外に出しません。Windows 版クライアント用の TCP 2004 も使えません（ブラウザ版だけ）。
- ブラウザ版は https で開くと自動で `wss://<同じホスト>/ws` につなぐので、SBOP2 のような wss シムは要りません。
- Basic 認証はサイト全体にかけ、`/ws` だけ外します（iPhone の Safari は WebSocket に Basic 認証を付けないため。ここはゲームのログインで守られます）。
- 遊んだデータ（`SBODATA/` と `SboSvr.ini`）は Docker の名前付きボリューム `sbo2003-staging_data` に入ります。イメージを作り直しても消えません。
- e2-micro に合わせて、メモリ 160MB・CPU 0.2 コアの上限を付けています（`compose.yaml`）。
  サーバーは誰もつながっていない時は長めに休むので、普段の CPU は 1 コアの 3〜4% ほどです。

## ファイル

| ファイル | 役割 |
|---|---|
| `Dockerfile` | サーバー（wasm）とブラウザ版を emsdk のイメージでビルドし、Node.js のイメージに入れる |
| `entrypoint.sh` | 初回にデータをボリュームへコピーし、管理者パスワードを設定してサーバーを起動する |
| `compose.yaml` | Sbo2003 サーバーのコンテナ（`staging-shared` につなぐ） |
| `sbo2003.caddy.template` | Caddy のサイト定義の雛形（`deploy.sh` が値を埋めて `~/caddy-sites/` に置く） |
| `staging.env.example` | ドメイン・Basic 認証・管理者パスワードの設定の雛形 |
| `deploy.sh` | ホストでイメージを入れ替え、サイト定義を置いて Caddy に読み直させる |
| `../../.github/workflows/staging-deploy.yml` | main への push で自動デプロイ |

## 初回のセットアップ

SBOP2 のステージングがすでに動いている前提です（Caddy・Docker・スワップは SBOP2 の手順で用意済み）。

### 1. ドメインを向ける

Sbo2003 用のホスト名（例: `sbo2003-staging.example.com`）の A レコードを、SBOP2 のステージングと同じ静的 IP に向けます。
証明書は SBOP2 の Caddy が Let's Encrypt から自動で取ります。

SBOP2 のステージングを sslip.io（例: `203-0-113-10.sslip.io`）で開いているなら、DNS の作業は要りません。
頭に名前を付けた `sbo2003.203-0-113-10.sslip.io` も同じ IP を返すので、それを `SBO2003_STAGING_DOMAIN` に書くだけで使えます。

### 2. 設定ファイルを置く

ホストに SSH で入って作ります。

```bash
mkdir -p ~/sbo2003-staging/deploy/staging
cd ~/sbo2003-staging/deploy/staging
# staging.env.example の中身をコピーして staging.env を作り、値を埋める
docker run --rm caddy:2 caddy hash-password --plaintext 'Basic 認証のパスワード'
```

- `staging.env` はそのまま（raw で）読むので、ハッシュの `$` はそのまま書いて大丈夫です。
- `SBO2003_ADMIN_PASSWORD` はゲームの管理者アカウント（`administrator`）のパスワードです。元データでは空なので、必ず決めてください。

### 3. GitHub の設定

Sbo2003 リポジトリの Settings → Secrets and variables → Actions に登録します。
SBOP2 と同じホスト・同じユーザーなので、SBOP2 に登録した値と同じものを使えます
（デプロイ専用の鍵を分けたい時は、SBOP2 の README の手順で作ってホストに登録します）。

| 種類 | 名前 | 値 |
|---|---|---|
| Variable | `SBO2003_STAGING_HOST` | ホスト名か IP（SBOP2 の `SBOP2_STAGING_HOST` と同じ） |
| Variable | `SBO2003_STAGING_USER` | SSH ユーザー（docker グループに入っていること） |
| Secret | `SBO2003_STAGING_SSH_KEY` | SSH 秘密鍵（`base64 -w0 <鍵ファイル>` の1行がおすすめ） |
| Secret | `SBO2003_STAGING_KNOWN_HOSTS` | `ssh-keyscan <ホスト>` の出力 |

`SBO2003_STAGING_HOST` が未設定の間、ワークフローは何もしません。

### 4. 初回デプロイ

Actions の「staging-deploy」を手動実行します（以後は main への push で自動）。
イメージ作成（サーバーとブラウザ版のビルド）→ 転送 → `deploy.sh --image`（読み込み・起動・`/health` 確認・サイト定義の反映）の順に進みます。
ホストには `~/sbo2003-staging/deploy/staging` だけが置かれます。

終わったら `https://<Sbo2003 のドメイン>/` を開き、Basic 認証のあとにログイン画面が出れば完了です。
アカウントは最初のログインで作られます（`SboSvr.ini` の `FreeAcount=1`）。

## 運用

```bash
cd ~/sbo2003-staging/deploy/staging
docker compose ps
docker compose logs -f sbosvr          # サーバーのログ
docker compose restart sbosvr          # 再起動（SIGTERM でデータを保存してから止まる）
docker run --rm -v sbo2003-staging_data:/d -v "$PWD":/o ubuntu tar czf /o/sbo2003-data-backup.tgz -C /d .   # データのバックアップ
```

データを最初からやり直す時は、止めてからボリュームを消して起動し直します（元に戻せないので、先にバックアップを取ってください）。

```bash
docker compose down && docker volume rm sbo2003-staging_data && docker compose up -d
```

## 注意

- SBOP2 の Caddy を作り直した時（SBOP2 の `docker compose up -d --force-recreate` など）も、`~/caddy-sites/sbo2003.caddy` はホストに残るので
  Sbo2003 のサイトはそのまま使えます。Sbo2003 のコンテナを作り直す必要もありません（別のネットワーク名前空間のため）。
- サイト定義が壊れていて Caddy が読み直せなかった時は、`deploy.sh` が前のサイト定義に戻してエラーで止まります（SBOP2 側は止まりません）。
- 通信量: ブラウザ版の初回読み込みは圧縮後で約 8MB です（2回目以降は送り直しません。サーバーがページに各ファイルの版（中身のハッシュ）を書き込み、`?v=版` 付きの URL をブラウザに1年覚えさせるので、リロードではページ本体以外は問い合わせも起きません。ゲームデータ（.data）はさらにブラウザの IndexedDB にも保存します。ビルドし直して中身が変われば版が変わり、その分だけ読み直されます）。
  無料枠の外向き通信（月 1GB 程度）は SBOP2 と合わせて数えられます。
