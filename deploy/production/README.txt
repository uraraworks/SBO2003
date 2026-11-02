# Sbo2003 サーバー配布パッケージ（本番 VPS 用）

ビルド日時: @@STAMP@@（コミット @@COMMIT@@）
同梱の Node.js: @@NODE_VERSION@@（node\node.exe）

## 中身
- server\          サーバー本体（SboSvr.js / SboSvr.wasm）
- web\             ブラウザ版クライアント（サーバーが配る）
- seed\            初回起動時に data\ へコピーする元データ（SBODATA\ と SboSvr.ini）
- node\            Windows 用の Node.js
- iis\web.config   IIS サイトの設定（127.0.0.1:18090 へ逆プロキシ）
- start.ps1        サーバーを起動し続ける（タスク SBO2003_SboSvr から呼ばれる。落ちたら起動し直す）
- stop.ps1         データを保存してから止める
- install-task.ps1 起動時に自動で立ち上がるタスクを登録する（初回だけ）
- sbo2003.env.example  設定の雛形（sbo2003.env にコピーして使う）

zip に入っていないもの（上書き展開しても消えない）:
- data\            遊んだデータ（SBODATA\ と SboSvr.ini）
- log\             サーバーのログ
- sbo2003.env      管理者パスワードと HTTP ポート

## 再デプロイ手順（初回の設定は済んでいる前提）

管理者の PowerShell で:

    cd C:\SBO2003
    .\stop.ps1                 # データを保存してから止まる（Stop-Process でいきなり止めないこと）
    Expand-Archive -Force <この zip> C:\SBO2003
    Start-ScheduledTask -TaskName SBO2003_SboSvr

初回のセットアップ手順はリポジトリの deploy/production/README.md を見てください。
