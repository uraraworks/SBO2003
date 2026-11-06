#!/usr/bin/env bash
# 本番 VPS のゲームデータから、非公開リポジトリ（SBO2003-private）の Release/SBODATA を作り直す（Mac から動かす）。
#
#   deploy/production/pull-prod-seed.sh [出力先]
#     省略時は、非公開リポジトリのクローンの Release/SBODATA（探し方は tools/server/find-data.sh：
#     このリポジトリと同じ階層の SBO2003-private か、環境変数 SBO2003_PRIVATE_DIR）
#
# マップ・NPC・アイテムなどの本物のデータはネタバレになるので、公開リポジトリには置かない
# （公開側の Release/SBODATA は 10×10 の草原だけ）。ローカルでサーバーを試す時（tools/server/run.sh・
# 各スレッドの確認）は、非公開リポジトリの Release/SBODATA を使うので、そこを本番の今のデータで更新する。
#
# 1. 本番のサーバーは止めずに、C:\SBO2003\data\SBODATA の SboData*.dat（名前が英数字だけのもの）を VPS の一時フォルダへ写して
#    zip にし、手元へ持ってくる（書き込み中に当たらないよう、写す前後でサイズと時刻が同じか確かめる）。
#    本番は 30 分ごとに保存するので、最大 30 分前の状態になる。
#    預かり所（SboDataLeaveInfo.dat）と Web 用の JSON（アカウント・ご意見・AI の鍵など）は持ってこない
# 2. tools/server/make-seed.sh で、キャラ情報を NPC だけにした初期データを作る（プレイヤーは入らない）
# 3. 出力先の SboData*.dat をそっくり入れ替える。手元と VPS の一時ファイルは消す
#
# サーバー（make-seed.sh が使う）がまだビルドされていなければ、先に tools/server/build.sh を動かす。
# 入れ替えたファイルは、出力先のリポジトリ（ふつうは SBO2003-private）で git add まで行う（コミットと push は手で）。
# 接続先は vps-deploy.sh と同じ（deploy/production/deploy.local.env の VPS_HOST・VPS_USER・VPS_PORT・SSH_KEY）。
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
if [ -n "${1:-}" ]; then
	OUT="$1"
elif PRIV="$("$ROOT/tools/server/find-data.sh" --repo)"; then
	OUT="$PRIV/Release/SBODATA"
else
	echo "非公開リポジトリ（SBO2003-private）のクローンが見つかりません。" >&2
	echo "このリポジトリと同じ階層にクローンするか、SBO2003_PRIVATE_DIR を設定してください" >&2
	exit 1
fi
# 公開リポジトリの Release/SBODATA（草原だけ）に本物のデータを入れてしまわないように
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/$(basename "$OUT")" in
	"$ROOT/Release/SBODATA") echo "公開リポジトリの Release/SBODATA には本物のデータを置けません（ネタバレ防止）" >&2; exit 1 ;;
esac
echo "==> 出力先: $OUT"
if [ -f "$HERE/deploy.local.env" ]; then
	# shellcheck disable=SC1091
	. "$HERE/deploy.local.env"
fi

: "${VPS_HOST:?VPS_HOST を指定してください（deploy/production/deploy.local.env）}"
: "${VPS_USER:?VPS_USER を指定してください（deploy/production/deploy.local.env）}"
VPS_PORT="${VPS_PORT:-22}"
SSH_KEY="${SSH_KEY:-$HOME/.ssh/urara_vps_deploy}"
SSH_KEY="${SSH_KEY/#\~/$HOME}"
VPS_ROOT="${VPS_ROOT:-C:/SBO2003}"
WIN_ROOT="${VPS_ROOT//\//\\}"
TARGET="$VPS_USER@$VPS_HOST"
SSH_OPTS=(-i "$SSH_KEY" -p "$VPS_PORT" -o BatchMode=yes -o ServerAliveInterval=30)
SCP_OPTS=(-i "$SSH_KEY" -P "$VPS_PORT" -o BatchMode=yes)
case "$WIN_ROOT" in *" "*) echo "VPS_ROOT に空白は使えません: $VPS_ROOT" >&2; exit 2 ;; esac

if [ ! -f "$ROOT/out/server/SboSvr.js" ]; then
	echo "==> サーバーがまだビルドされていないので、先にビルドします（tools/server/build.sh）"
	"$ROOT/tools/server/build.sh"
	[ -f "$ROOT/out/server/SboSvr.js" ] || { echo "ビルドできませんでした（emsdk は docs/vscode-mac.md の手順で用意）" >&2; exit 1; }
fi

# vps-deploy.sh と同じ呼び方（UTF-16LE + base64 の -EncodedCommand で、cmd でも PowerShell でも同じに動く）
remote_cmd() {
	ssh "${SSH_OPTS[@]}" "$TARGET" "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $(printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\n')"
}

WORK="$(mktemp -d)"
# プレイヤーのデータ（キャラ情報）が一時的に入るので、終わったら必ず消す
trap 'rm -rf "$WORK"' EXIT
STAMP="$(date +%Y%m%d_%H%M%S)"
REMOTE_DIR="$WIN_ROOT\\upload\\seed-$STAMP"
REMOTE_ZIP="$WIN_ROOT\\upload\\seed-$STAMP.zip"

echo "==> 本番のゲームデータを写します（サーバーは止めません）"
remote_cmd "
\$ErrorActionPreference = 'Stop'
\$src = '$WIN_ROOT\\data\\SBODATA'
\$dst = '$REMOTE_DIR'
function Files { Get-ChildItem -Path \$src -Filter 'SboData*.dat' | Where-Object { \$_.Name -cmatch '^SboData[A-Za-z]+[0-9]*\.dat\$' -and \$_.Name -ne 'SboDataLeaveInfo.dat' } }
function Snap { Files | Sort-Object Name | ForEach-Object { '{0}|{1}|{2}' -f \$_.Name, \$_.Length, \$_.LastWriteTimeUtc.Ticks } }
\$ok = \$false
for (\$i = 1; \$i -le 5; \$i++) {
	if (Test-Path \$dst) { Remove-Item -Recurse -Force \$dst }
	New-Item -ItemType Directory -Force -Path (Join-Path \$dst 'SBODATA') | Out-Null
	\$before = (Snap) -join ';'
	Files | Copy-Item -Destination (Join-Path \$dst 'SBODATA')
	\$after = (Snap) -join ';'
	if (\$before -eq \$after) { \$ok = \$true; break }
	Write-Output \"保存中だったのでやり直します（\$i/5）\"
	Start-Sleep -Seconds 10
}
if (-not \$ok) { throw '保存が続いていて写せませんでした。少し待ってからやり直してください' }
Write-Output ('   {0} ファイル' -f (Get-ChildItem (Join-Path \$dst 'SBODATA')).Count)
# フォルダごとだと Windows PowerShell 5.1 は区切りが \\ の zip を作るので、中身だけを入れる
Compress-Archive -Path (Join-Path \$dst 'SBODATA\\*') -DestinationPath '$REMOTE_ZIP' -Force
Remove-Item -Recurse -Force \$dst
"

echo "==> 手元へ持ってきます"
scp "${SCP_OPTS[@]}" "$TARGET:$VPS_ROOT/upload/seed-$STAMP.zip" "$WORK/prod.zip"
remote_cmd "Remove-Item -Force '$REMOTE_ZIP'"
mkdir -p "$WORK/prod/SBODATA"
# unzip は警告だけなら 1 を返すので、それは通す
unzip -q "$WORK/prod.zip" -d "$WORK/prod/SBODATA" || [ $? -eq 1 ]
[ -f "$WORK/prod/SBODATA/SboDataCharInfo.dat" ] || { echo "持ってきたデータに SboDataCharInfo.dat がありません" >&2; exit 1; }

echo "==> NPC だけの初期データを作ります"
"$ROOT/tools/server/make-seed.sh" "$WORK/prod/SBODATA" "$WORK/seed" > "$WORK/seed.log" 2>&1 || { cat "$WORK/seed.log" >&2; exit 1; }
grep -E "読み込|NPC|エラー|失敗" "$WORK/seed.log" || true
[ -f "$WORK/seed/SboDataCharInfo.dat" ] || { cat "$WORK/seed.log" >&2; echo "初期データを作れませんでした" >&2; exit 1; }

echo "==> $OUT を入れ替えます"
mkdir -p "$OUT"
# 本番で消えたマップなどが残らないよう、いったん SboData*.dat を消してから置く
# （「SboDataMapInfo075(広い町バックアップ).dat」のような控えはサーバーが読まないので持ってこず、手元のものを残す）
find "$OUT" -maxdepth 1 -name 'SboData*.dat' | grep -E '/SboData[A-Za-z]+[0-9]*\.dat$' | while IFS= read -r f; do rm -f "$f"; done
find "$WORK/seed" -maxdepth 1 -name 'SboData*.dat' | grep -E '/SboData[A-Za-z]+[0-9]*\.dat$' | while IFS= read -r f; do cp "$f" "$OUT/"; done
echo "   $(find "$OUT" -maxdepth 1 -name 'SboData*.dat' | wc -l | tr -d ' ') ファイル"
if REPO="$(git -C "$OUT" rev-parse --show-toplevel 2>/dev/null)"; then
	# 入れ替えたファイルだけを git に載せる（git add Release/SBODATA だと、Mac が日本語名の控えを
	# 別の文字の並び（濁点が分かれた形）で見せるため、同じ控えがもう 1 つ増えてしまう）
	git -C "$REPO" add -u -- "$OUT"
	find "$OUT" -maxdepth 1 -name 'SboData*.dat' | grep -E '/SboData[A-Za-z]+[0-9]*\.dat$' | while IFS= read -r f; do git -C "$REPO" add -- "$f"; done
	echo "   $REPO の git に載せました（あとは git commit と git push だけ）:"
	git -C "$REPO" diff --cached --stat -- "$OUT" | tail -5
fi
echo "==> 完了（プレイヤーのデータは入っていません。手元・VPS の一時ファイルは消しました）"
echo "   ローカルで遊んだデータ（out/server-data）は残っているので、新しい初期データで試す時は"
echo "   tools/server/stop.sh で止めてから out/server-data を消して tools/server/run.sh を動かしてください"
