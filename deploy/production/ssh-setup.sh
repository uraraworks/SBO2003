#!/usr/bin/env bash
# Actions で VPS へ SSH するための準備（production-publish の announce・deploy・cancel-notice から呼ぶ）。
#   環境変数 SSH_KEY（秘密鍵。そのままの形でも、base64 で1行にした形でもよい）と KNOWN_HOSTS を ~/.ssh に置く。
#   鍵は ~/.ssh/id_vps になる。
set -euo pipefail
install -m 700 -d ~/.ssh
# CR と行末の空白は取り除く
key=$(printf '%s\n' "$SSH_KEY" | tr -d '\r' | sed 's/[[:space:]]*$//')
case "$key" in
	-----BEGIN*) printf '%s\n' "$key" > ~/.ssh/id_vps ;;
	*) printf '%s' "$key" | tr -d ' \n' | base64 -d > ~/.ssh/id_vps ;;
esac
chmod 600 ~/.ssh/id_vps
ssh-keygen -y -f ~/.ssh/id_vps > /dev/null
printf '%s\n' "$KNOWN_HOSTS" > ~/.ssh/known_hosts
