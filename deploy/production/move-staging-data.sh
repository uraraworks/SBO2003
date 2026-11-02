#!/usr/bin/env bash
# ステージング（GCP の Docker）のデータを本番 VPS へ持っていく（Mac から 1 回だけ動かす想定）。
#
#   deploy/production/move-staging-data.sh
#
# 1. ステージングのサーバーを止めて（SIGTERM でデータを保存してから止まる）、ボリュームの
#    SBODATA/ と SboSvr.ini を tgz にして手元に持ってくる。終わったらステージングを起動し直す
# 2. vps-deploy.sh --data で本番に送り、本番のサーバーを止めて今のデータを backup\ に控えてから置き換え、起動し直す
# 3. 手元の tgz を消す（プレイヤーのデータが入っているので残さない。GitHub にも上げない）
#
# 設定は deploy/production/deploy.local.env（deploy.env.example をコピー）に書く:
#   STAGING_TARGET   ステージングの SSH 先（ユーザー@ホスト）
#   STAGING_SSH_KEY  ステージング用の秘密鍵（任意。無ければ ssh の既定）
#   VPS_HOST・VPS_USER・VPS_PORT・SSH_KEY は vps-deploy.sh と同じ
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -f "$HERE/deploy.local.env" ]; then
	# shellcheck disable=SC1091
	. "$HERE/deploy.local.env"
fi
: "${STAGING_TARGET:?STAGING_TARGET（ステージングの ユーザー@ホスト）を指定してください}"
STAGING_SSH=(-o BatchMode=yes)
if [ -n "${STAGING_SSH_KEY:-}" ]; then STAGING_SSH+=(-i "${STAGING_SSH_KEY/#\~/$HOME}"); fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
TGZ="$WORK/sbo2003-data-$(date +%Y%m%d_%H%M%S).tgz"

echo "==> ステージングを止めてデータを取り出します（少しの間ステージングに入れなくなります）"
# 標準出力には tgz だけを流す（docker compose の表示は標準エラーへ）
ssh "${STAGING_SSH[@]}" "$STAGING_TARGET" 'set -e
cd ~/sbo2003-staging/deploy/staging
docker compose stop sbosvr >&2
rc=0
docker run --rm -v sbo2003-staging_data:/d ubuntu tar czf - -C /d SBODATA SboSvr.ini || rc=$?
docker compose start sbosvr >&2
exit $rc' > "$TGZ"
tar tzf "$TGZ" | grep -q '^SBODATA/' || { echo "取り出したデータに SBODATA/ がありません" >&2; exit 1; }
echo "   $(du -h "$TGZ" | cut -f1)、$(tar tzf "$TGZ" | grep -c -v '/$') ファイル"

echo "==> 本番 VPS へ送って入れ替えます"
"$HERE/vps-deploy.sh" --data "$TGZ"
echo "==> 完了（手元の一時ファイルは消しました）"
