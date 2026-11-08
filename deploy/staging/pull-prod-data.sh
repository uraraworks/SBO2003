#!/usr/bin/env bash
# 本番 VPS の今のデータを、ステージング（GCP の Docker）に写す（Mac から動かす）。
#
#   deploy/staging/pull-prod-data.sh             丸ごと（プレイヤー・アカウント・預かり所も）
#   deploy/staging/pull-prod-data.sh --npc-only  ゲームの中身だけ（キャラ情報は NPC だけ。ステージングのプレイヤーは消える）
#   どちらも -y を付けると確認を飛ばす
#
# 1. 本番のサーバーは止めずに、C:\SBO2003\data\SBODATA のデータを VPS の一時フォルダへ写して zip にし、手元へ持ってくる
#    （書き込み中に当たらないよう、写す前後でサイズと時刻が同じか確かめる。本番は 30 分ごとに保存するので、最大 30 分前の状態）
#    - 丸ごと: SboData*.dat・SboDataUtf8.txt・Web 用の JSON（SboWeb*.json）
#    - --npc-only: 預かり所と Web 用の JSON は持ってこず、tools/server/make-seed.sh で NPC だけにする
# 2. ステージングのデータ（ボリューム）を、ホストの ~/sbo2003-staging/backup/ に tgz で控える（戻す時はこれ）
# 3. ステージングのサーバーを止めて（SIGTERM で保存してから止まる）、データを入れ替えて起動し直す
#    - ステージングの SboSvr.ini（管理者パスワード）と AI 用キー（SboWebAiKeys.json）はそのまま残す
#    - 丸ごとの時は、変更履歴・しおり（SboWebDataHistory.json）も本番のものになる（データと番号が合うように）
#    - --npc-only の時は、ステージングの変更履歴を控えへ移して空にする（入れ替えた後のデータとは番号が合わないため）
# 4. 手元と VPS の一時ファイルを消す（プレイヤーのデータが入るので残さない。GitHub にも上げない）
#
# 設定は deploy/production/deploy.local.env（move-staging-data.sh と同じ）:
#   STAGING_TARGET   ステージングの SSH 先（ユーザー@ホスト）
#   STAGING_SSH_KEY  ステージング用の秘密鍵（任意。無ければ ssh の既定）
#   VPS_HOST・VPS_USER・VPS_PORT・SSH_KEY は vps-deploy.sh と同じ
# --npc-only はサーバーのビルド（tools/server/build.sh）が要る。
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
MODE=full
YES=0
for a in "$@"; do
	case "$a" in
		--npc-only) MODE=npc ;;
		-y|--yes) YES=1 ;;
		-h|--help) sed -n '2,24p' "$0"; exit 0 ;;
		*) echo "知らない引数です: $a" >&2; exit 2 ;;
	esac
done

if [ -f "$ROOT/deploy/production/deploy.local.env" ]; then
	# shellcheck disable=SC1091
	. "$ROOT/deploy/production/deploy.local.env"
fi
: "${STAGING_TARGET:?STAGING_TARGET（ステージングの ユーザー@ホスト）を deploy/production/deploy.local.env に書いてください}"
: "${VPS_HOST:?VPS_HOST を deploy/production/deploy.local.env に書いてください}"
: "${VPS_USER:?VPS_USER を deploy/production/deploy.local.env に書いてください}"
STAGING_SSH=(-o BatchMode=yes)
if [ -n "${STAGING_SSH_KEY:-}" ]; then STAGING_SSH+=(-i "${STAGING_SSH_KEY/#\~/$HOME}"); fi
VPS_PORT="${VPS_PORT:-22}"
SSH_KEY="${SSH_KEY:-$HOME/.ssh/urara_vps_deploy}"
SSH_KEY="${SSH_KEY/#\~/$HOME}"
VPS_ROOT="${VPS_ROOT:-C:/SBO2003}"
WIN_ROOT="${VPS_ROOT//\//\\}"
TARGET="$VPS_USER@$VPS_HOST"
SSH_OPTS=(-i "$SSH_KEY" -p "$VPS_PORT" -o BatchMode=yes -o ServerAliveInterval=30)
SCP_OPTS=(-i "$SSH_KEY" -P "$VPS_PORT" -o BatchMode=yes)
case "$WIN_ROOT" in *" "*) echo "VPS_ROOT に空白は使えません: $VPS_ROOT" >&2; exit 2 ;; esac

if [ "$MODE" = npc ] && [ ! -f "$ROOT/out/server/SboSvr.js" ]; then
	echo "==> サーバーがまだビルドされていないので、先にビルドします（tools/server/build.sh）"
	"$ROOT/tools/server/build.sh"
fi

if [ "$YES" != 1 ]; then
	if [ "$MODE" = full ]; then
		echo "ステージングのデータを、本番の今のデータ（プレイヤー・アカウントも）で丸ごと入れ替えます。"
	else
		echo "ステージングのゲームの中身を本番の今のデータにします（キャラは NPC だけ。ステージングのプレイヤーは消えます）。"
	fi
	echo "今のステージングのデータは、ホストの ~/sbo2003-staging/backup/ に控えます。"
	printf '続けますか？ [y/N] '
	read -r ans
	case "$ans" in y|Y|yes) ;; *) echo "やめました"; exit 0 ;; esac
fi

# vps-deploy.sh と同じ呼び方（UTF-16LE + base64 の -EncodedCommand で、cmd でも PowerShell でも同じに動く）
remote_cmd() {
	ssh "${SSH_OPTS[@]}" "$TARGET" "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $(printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\n')"
}

WORK="$(mktemp -d)"
# プレイヤーのデータが一時的に入るので、終わったら必ず消す
trap 'rm -rf "$WORK"' EXIT
STAMP="$(date +%Y%m%d_%H%M%S)"
REMOTE_DIR="$WIN_ROOT\\upload\\stg-$STAMP"
REMOTE_ZIP="$WIN_ROOT\\upload\\stg-$STAMP.zip"
if [ "$MODE" = full ]; then
	# 本番の AI 用キーは持ってこない（ステージングのキーを残す）
	PS_FILTER="(\$_.Name -cmatch '^SboData[A-Za-z]+[0-9]*\.dat\$') -or \$_.Name -ceq 'SboDataUtf8.txt' -or (\$_.Name -cmatch '^SboWeb[A-Za-z]+\.json\$' -and \$_.Name -cne 'SboWebAiKeys.json')"
else
	PS_FILTER="(\$_.Name -cmatch '^SboData[A-Za-z]+[0-9]*\.dat\$' -and \$_.Name -ne 'SboDataLeaveInfo.dat') -or \$_.Name -ceq 'SboDataUtf8.txt'"
fi

echo "==> 本番のデータを写します（本番のサーバーは止めません）"
remote_cmd "
\$ErrorActionPreference = 'Stop'
\$src = '$WIN_ROOT\\data\\SBODATA'
\$dst = '$REMOTE_DIR'
function Files { Get-ChildItem -Path \$src -File | Where-Object { $PS_FILTER } }
function Snap { Files | Sort-Object Name | ForEach-Object { '{0}|{1}|{2}' -f \$_.Name, \$_.Length, \$_.LastWriteTimeUtc.Ticks } }
\$ok = \$false
for (\$i = 1; \$i -le 5; \$i++) {
	if (Test-Path \$dst) { Remove-Item -Recurse -Force \$dst }
	New-Item -ItemType Directory -Force -Path \$dst | Out-Null
	\$before = (Snap) -join ';'
	Files | Copy-Item -Destination \$dst
	\$after = (Snap) -join ';'
	if (\$before -eq \$after) { \$ok = \$true; break }
	Write-Output \"保存中だったのでやり直します（\$i/5）\"
	Start-Sleep -Seconds 10
}
if (-not \$ok) { throw '保存が続いていて写せませんでした。少し待ってからやり直してください' }
Write-Output ('   {0} ファイル' -f (Get-ChildItem \$dst).Count)
Compress-Archive -Path (Join-Path \$dst '*') -DestinationPath '$REMOTE_ZIP' -Force
Remove-Item -Recurse -Force \$dst
"

echo "==> 手元へ持ってきます"
scp "${SCP_OPTS[@]}" "$TARGET:$VPS_ROOT/upload/stg-$STAMP.zip" "$WORK/prod.zip"
remote_cmd "Remove-Item -Force '$REMOTE_ZIP'"
mkdir -p "$WORK/prod"
# unzip は警告だけなら 1 を返すので、それは通す
unzip -q "$WORK/prod.zip" -d "$WORK/prod" || [ $? -eq 1 ]
[ -f "$WORK/prod/SboDataCharInfo.dat" ] || { echo "持ってきたデータに SboDataCharInfo.dat がありません" >&2; exit 1; }
[ -f "$WORK/prod/SboDataUtf8.txt" ] || { echo "持ってきたデータに SboDataUtf8.txt（UTF-8 の目印）がありません。文字化けするのでやめます" >&2; exit 1; }

PUT="$WORK/prod"
if [ "$MODE" = npc ]; then
	echo "==> NPC だけのデータを作ります"
	"$ROOT/tools/server/make-seed.sh" "$WORK/prod" "$WORK/seed" > "$WORK/seed.log" 2>&1 || { cat "$WORK/seed.log" >&2; exit 1; }
	[ -f "$WORK/seed/SboDataCharInfo.dat" ] || { cat "$WORK/seed.log" >&2; echo "NPC だけのデータを作れませんでした" >&2; exit 1; }
	PUT="$WORK/seed"
fi
# 送るファイル（SboData*.dat・目印・丸ごとの時は Web 用の JSON）
( cd "$PUT" && find . -maxdepth 1 -type f \( -name 'SboData*.dat' -o -name 'SboDataUtf8.txt' -o -name 'SboWeb*.json' \) | sed 's|^\./||' | grep -E '^(SboData[A-Za-z]+[0-9]*\.dat|SboDataUtf8\.txt|SboWeb[A-Za-z]+\.json)$' | grep -v '^SboWebAiKeys\.json$' > "$WORK/list.txt" )
echo "   $(wc -l < "$WORK/list.txt" | tr -d ' ') ファイルを送ります"
tar czf "$WORK/put.tgz" -C "$PUT" -T "$WORK/list.txt"

echo "==> ステージングのデータを控えてから入れ替えます（少しの間ステージングに入れなくなります）"
REMOTE_PUT="/tmp/sbo2003-put-$STAMP.tgz"
scp "${STAGING_SSH[@]}" "$WORK/put.tgz" "$STAGING_TARGET:$REMOTE_PUT"
ssh "${STAGING_SSH[@]}" "$STAGING_TARGET" "set -e
trap 'rm -f $REMOTE_PUT' EXIT
cd ~/sbo2003-staging/deploy/staging
mkdir -p ~/sbo2003-staging/backup
docker compose stop sbosvr >&2
rc=0
# 控え（戻す時: 止めてからボリュームを空にして、この tgz を展開する）
docker run --rm -v sbo2003-staging_data:/d -v ~/sbo2003-staging/backup:/b ubuntu \
	tar czf /b/data-before-prod-$STAMP.tgz -C /d . || rc=\$?
if [ \$rc -eq 0 ]; then
	docker run --rm -v sbo2003-staging_data:/d -v $REMOTE_PUT:/put.tgz:ro ubuntu sh -c '
		set -e
		cd /d/SBODATA
		# 本番で消えたマップなどが残らないよう、サーバーが読む名前の .dat は消してから置く
		find . -maxdepth 1 -type f -regextype posix-extended -regex \"\\./SboData[A-Za-z]+[0-9]*\\.dat\" -delete
		if [ \"$MODE\" = npc ]; then
			# 入れ替えた後のデータとは番号が合わないので、変更履歴・しおりは空にする（控えには残っている）
			rm -f SboWebDataHistory.json
		fi
		tar xzf /put.tgz
		chown -R \$(stat -c %u:%g /d) /d/SBODATA
	' || rc=\$?
fi
docker compose start sbosvr >&2
exit \$rc"

echo "==> 完了しました。控え: $STAGING_TARGET:~/sbo2003-staging/backup/data-before-prod-$STAMP.tgz"
echo "   （手元と VPS の一時ファイルは消しました）"
