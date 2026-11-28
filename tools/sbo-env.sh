#!/bin/bash
# ===========================================================================
# ビルドスクリプト共通の環境づくり（source して使う）
#
#   Mac / Linux では何もしません（従来どおり python3 を使うだけ）。
#   Windows（Git Bash）の時だけ、次の 2 つを直します。
#     1. python3 が Microsoft Store のスタブだと動かない → 本物の Python を探して PATH の先頭に置く
#     2. フォルダのパスが長いと em++ のコマンドが長すぎて
#        "Argument list too long" になる → 短いパス（既定 C:\s）から自分を呼び直す
#
#   使い方（ROOT を決めたあとで）:
#     . "$ROOT/tools/sbo-env.sh"
#     sbo_env_init "tools/web/build.sh" "$@"
# ===========================================================================

sbo_is_windows() {
	case "$(uname -s 2>/dev/null)" in
		MINGW*|MSYS*|CYGWIN*) return 0 ;;
		*) return 1 ;;
	esac
}

# python3 が動くか（Store のスタブは動かないので、実際に実行して確かめる）
sbo_python3_works() {
	command -v python3 >/dev/null 2>&1 && python3 -c 'import sys' >/dev/null 2>&1
}

# python3 が使えない Windows で、本物の Python を python3 として呼べるようにする
sbo_fix_python() {
	sbo_python3_works && return 0
	local shim="$ROOT/out/.win-shim" real=""
	mkdir -p "$shim"
	local c
	for c in python python3.exe py; do
		command -v "$c" >/dev/null 2>&1 || continue
		if [ "$c" = "py" ]; then
			py -3 -c 'import sys' >/dev/null 2>&1 || continue
			printf '#!/bin/sh\nexec py -3 "$@"\n' > "$shim/python3"
		else
			"$c" -c 'import sys' >/dev/null 2>&1 || continue
			real="$(command -v "$c")"
			printf '#!/bin/sh\nexec "%s" "$@"\n' "$real" > "$shim/python3"
		fi
		chmod +x "$shim/python3"
		PATH="$shim:$PATH"
		export PATH
		return 0
	done
	echo "[env] 動く Python が見つかりません。Python 3 を入れてください（python3 は Store のスタブです）" >&2
	return 1
}

# パスが長い時、短いパス（ジャンクション）から同じスクリプトを呼び直す
#   $1: ROOT からのスクリプトの場所  $2...: スクリプトの引数
sbo_short_root() {
	local script="$1"
	shift
	[ -n "${SBO_SHORT_ROOT_DONE:-}" ] && return 0
	[ "${#ROOT}" -le 16 ] && return 0
	command -v cygpath >/dev/null 2>&1 || return 0
	local link="${SBO_SHORT_ROOT:-/c/s}"
	if [ ! -e "$link" ]; then
		cmd //c mklink //J "$(cygpath -w "$link")" "$(cygpath -w "$ROOT")" >/dev/null 2>&1 || {
			echo "[env] 短いパス $link を作れなかったので、そのまま続けます" >&2
			return 0
		}
	fi
	if [ "$link/$script" -ef "$ROOT/$script" ]; then
		export SBO_SHORT_ROOT_DONE=1
		echo "[env] パスが長いので $link から実行します"
		exec bash "$link/$script" "$@"
	fi
	echo "[env] $link が別の場所を指しているので、短いパスは使いません（SBO_SHORT_ROOT で変えられます）" >&2
	return 0
}

sbo_env_init() {
	sbo_is_windows || return 0
	sbo_short_root "$@"
	sbo_fix_python
}
