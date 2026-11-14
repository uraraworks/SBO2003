#!/bin/bash
# ===========================================================================
# SBO2003 クライアントのブラウザ版をビルドする
#
#   使い方: tools/web/build.sh [debug|release]   （省略時は release）
#   出力先: out/web/dist/sbocli.html（+ .js / .wasm / .data）
#
# 前提: Emscripten（emsdk）が使えること。~/emsdk があれば自動で読み込む。
#
# 流れ
#   1. ソース（UTF-8）を前処理して out/web/src へ置く
#      （文字列リテラルを SJIS の 8 進エスケープにする。実行時の文字コードは SJIS のまま）
#   2. リソース DLL の .rc からリソース表を作る
#   3. 並列にコンパイルしてリンクする
# ===========================================================================
set -euo pipefail

MODE="${1:-release}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/out/web"
SRC="$OUT/src"
OBJ="$OUT/obj-$MODE"
DIST="$OUT/dist"
GEN="$OUT/gen"

if ! command -v em++ >/dev/null 2>&1; then
	if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
		# shellcheck disable=SC1091
		source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
	else
		echo "[build] em++ が見つかりません。emsdk を用意してください" >&2
		exit 1
	fi
fi

mkdir -p "$OBJ" "$DIST" "$GEN"

echo "[build] 1/3 前処理"
python3 "$ROOT/tools/web/src_prepass.py" -I SboCli "$ROOT" "$SRC" SboCli common SboSoundData SboGrpData >/dev/null

echo "[build] 2/3 リソース表"
python3 "$ROOT/tools/web/gen_resources.py" "$ROOT" "$GEN"

# ---------------------------------------------------------------------------
# コンパイルするソース
#   ゲーム側は tools/web/sources.txt に載っているもの。それに互換層と
#   Web 版の差し替え実装を足す
# ---------------------------------------------------------------------------
# macOS 標準の bash 3.2 には mapfile が無いので while read で読む
GAME_SRCS=()
while IFS= read -r line; do
	case "$line" in
		''|'#'*) ;;
		*) GAME_SRCS+=("$line") ;;
	esac
done < <(tr -d '\r' < "$ROOT/tools/web/sources.txt")
WEB_SRCS=(
	"$ROOT/platform/w32_core.cpp"
	"$ROOT/platform/w32_gdi.cpp"
	"$ROOT/platform/w32_ctrl.cpp"
	"$ROOT/platform/w32_dlg.cpp"
	"$ROOT/platform/w32_mfc.cpp"
	"$ROOT/platform/w32_sjis.cpp"
	"$ROOT/platform/w32_file.cpp"
	"$ROOT/web/w32_html.cpp"
	"$ROOT/web/w32_perf.cpp"
	"$ROOT/web/web_main.cpp"
	"$ROOT/web/replace/CDXAudio_web.cpp"
	"$ROOT/web/replace/LibMusicLoader_web.cpp"
	"$ROOT/web/replace/MacAddr_web.cpp"
	"$ROOT/web/replace/CUraraSockTCP_web.cpp"
	"$ROOT/web/replace/SboCliMsgLogWindow_web.cpp"
	"$ROOT/web/replace/SboCliWebAccount_web.cpp"
	"$GEN/w32_resources.cpp"
)

PORTS=(-sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_LIBPNG=1 -sUSE_ZLIB=1)
if [ "$MODE" = "debug" ]; then
	OPT=(-O0 -g)
else
	OPT=(-O2)
fi
CXXFLAGS=(-std=gnu++17 -fms-extensions -fno-strict-aliasing -w "${OPT[@]}" "${PORTS[@]}"
	-I "$ROOT/platform/include"
	-I "$SRC" -I "$SRC/SboCli" -I "$SRC/SboCli/src" -I "$SRC/common")
GAME_FLAGS=(-include "$SRC/SboCli/StdAfx.h")

echo "[build] 3/3 コンパイル（${#GAME_SRCS[@]} + ${#WEB_SRCS[@]} ファイル）"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
OBJS=()
compile_list=()
for s in "${GAME_SRCS[@]}"; do
	o="$OBJ/$(echo "$s" | tr '/' '_').o"
	OBJS+=("$o")
	compile_list+=("$SRC/$s|$o|game")
done
for s in "${WEB_SRCS[@]}"; do
	o="$OBJ/web_$(basename "$s").o"
	OBJS+=("$o")
	case "$s" in
		*/replace/*) compile_list+=("$s|$o|game") ;;
		*)           compile_list+=("$s|$o|web") ;;
	esac
done

compile_one() {
	local src="${1%%|*}" rest="${1#*|}"
	local obj="${rest%%|*}" kind="${rest#*|}"
	# ソースかヘッダが新しい（または無くなった）ならコンパイルし直す（依存は .d で見る）
	if [ -f "$obj" ] && [ -f "$obj.d" ]; then
		local newer
		newer="$(sed 's/\\$//' "$obj.d" | tr ' ' '\n' | grep -v ':$' | grep -v '^$' | while read -r f; do { [ ! -e "$f" ] || [ "$f" -nt "$obj" ]; } && echo x && break; done || true)"
		[ -z "$newer" ] && return 0
	fi
	local extra=()
	[ "$kind" = "game" ] && extra=("${GAME_FLAGS[@]}")
	em++ "${CXXFLAGS[@]}" ${extra[@]+"${extra[@]}"} -MMD -MF "$obj.d" -c "$src" -o "$obj"
}
fail=0
# macOS 標準の bash 3.2 には wait -n が無いので、JOBS 個ずつまとめて待つ
pids=()
for item in "${compile_list[@]}"; do
	compile_one "$item" &
	pids+=($!)
	if [ "${#pids[@]}" -ge "$JOBS" ]; then
		for p in "${pids[@]}"; do
			wait "$p" || fail=1
		done
		pids=()
	fi
done
for p in "${pids[@]+"${pids[@]}"}"; do
	wait "$p" || fail=1
done
if [ "$fail" -ne 0 ]; then
	echo "[build] コンパイルに失敗しました" >&2
	exit 1
fi

echo "[build] リンク"
# SBO_LDFLAGS: 計測用にリンクへ足すオプション（例: SBO_LDFLAGS=--profiling-funcs で関数名を残す）
# --use-preload-cache: .data をブラウザの IndexedDB にも保存し、中身が同じ間は次からそちらを使う
PRELOAD=(--preload-file "$ROOT/web/font@/font")
while IFS= read -r line; do
	[ -n "$line" ] && PRELOAD+=(--preload-file "$line")
done < "$GEN/preload.txt"

em++ "${OPT[@]}" ${SBO_LDFLAGS:-} "${PORTS[@]}" "${OBJS[@]}" -o "$DIST/sbocli.html" \
	--shell-file "$ROOT/web/shell/sbocli.html" \
	"${PRELOAD[@]}" --use-preload-cache \
	-lidbfs.js -lwebsocket.js \
	-sGLOBAL_BASE=1048576 \
	-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB -sSTACK_SIZE=1MB \
	-sEXPORTED_RUNTIME_METHODS=FS,UTF8ToString,stringToNewUTF8,addRunDependency,removeRunDependency \
	-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8,$UTF8ToString' \
	-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_W32JsOnKey,_W32JsOnMouse,_W32JsOnActivate,_W32JsOnButton

# フッターに反映した日時とコミットを書き込む（SBOP2 と同じく、どの版が動いているか分かるように）。
# Docker の中など git が使えない所では、環境変数 SBO2003_BUILD_COMMIT で渡す。
BUILD_COMMIT="${SBO2003_BUILD_COMMIT:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || true)}"
BUILD_COMMIT="${BUILD_COMMIT:-unknown}"
BUILD_DATE="${SBO2003_BUILD_DATE:-$(TZ=Asia/Tokyo date '+%Y-%m-%d %H:%M')}"
# （macOS の sed は -i の書き方が違うので、一時ファイルを経由する）
sed "s/@@SBO_BUILD_DATE@@/$BUILD_DATE/g; s/@@SBO_BUILD_COMMIT@@/$BUILD_COMMIT/g" "$DIST/sbocli.html" > "$DIST/sbocli.html.tmp"
mv "$DIST/sbocli.html.tmp" "$DIST/sbocli.html"
echo "[build] 版: $BUILD_DATE $BUILD_COMMIT"

# BGM（MIDI）用の JS シンセとサウンドフォントは、ページから後で読み込むので .data に入れずに置く
mkdir -p "$DIST/bgm"
cp "$ROOT/web/bgm/sbo_bgm.js" \
	"$ROOT/web/vendor/spessasynth/spessasynth.js" \
	"$ROOT/web/vendor/spessasynth/spessasynth_processor.min.js" \
	"$ROOT/web/soundfont/GeneralUserGS.sf3" \
	"$DIST/bgm/"

# 音声ファイル(.m4a)の BGM（非公開側の SboSoundData/Audio/。無ければ何もしない）
if compgen -G "$ROOT/SboSoundData/Audio/*.m4a" >/dev/null; then
	mkdir -p "$DIST/bgm/audio"
	cp "$ROOT"/SboSoundData/Audio/*.m4a "$DIST/bgm/audio/"
	echo "[build] 音声 BGM: $(ls "$DIST"/bgm/audio/*.m4a | wc -l) 曲"
else
	echo "[build] SboSoundData/Audio に .m4a が無いので、音声 BGM は入りません"
fi

# 管理ページ（/admin。キャラ検索・キャラ名の変更・復旧の承認・マップ資料・パーツ図鑑・配置案。サーバーの /api/admin/* を使う）
# ドット絵エディタ（/dot。dot.html と dot-*.js。手元のフォルダの PNG を編集する）
# キャラ名の変更ページ（/rename。プレイヤー用）
# ご意見箱（/feedback。プレイヤー用。サーバーの /api/feedback/* を使う）
# ドット絵工房（/kobo・/kobo-draw。プレイヤー用。管理ページのタブは admin-kobo.js。サーバーの /api/kobo/*・/api/admin/kobo/* を使う）
# recover-admin.html は前の URL を /admin#recover へ送るだけ
cp "$ROOT/web/account/admin.html" \
	"$ROOT/web/account/admin-mapdoc.js" \
	"$ROOT/web/account/admin-design.js" \
	"$ROOT/web/account/admin-mapview.js" \
	"$ROOT/web/account/admin-recipe.js" \
	"$ROOT/web/account/admin-kobo.js" \
	"$ROOT"/web/account/dot.html \
	"$ROOT"/web/account/dot-*.js \
	"$ROOT/web/account/rename.html" \
	"$ROOT/web/account/feedback.html" \
	"$ROOT/web/account/recover-admin.html" \
	"$DIST/"
# ドット絵工房のページ（kobo.html・kobo.js・kobo-draw.html・kobo-draw.js ほか kobo*）
for f in "$ROOT"/web/account/kobo*.html "$ROOT"/web/account/kobo*.js; do
	[ -e "$f" ] && cp "$f" "$DIST/"
done

# 管理ページが読むゲームの画像（SboGrpData/grp の PNG を全部。日本語名のものも）
#   ・「マップ資料」で全体図を描く（map0[1-5]・char[01][0-9]。admin-mapdoc.js が読む）
#   ・「ドット絵工房」でお題の元の画像を選ぶ・採用した絵を貼る（admin-kobo.js が index.json の一覧から読む）
mkdir -p "$DIST/admin-grp"
cp "$ROOT"/SboGrpData/grp/*.png "$DIST/admin-grp/"
# 一覧（admin-grp/index.json = {"files":["800.png", ...]}）。名前に " や \ は無い前提で、シェルだけで書く（macOS・Linux 両方で動く）
{
	printf '{"files":['
	first=1
	for f in "$DIST"/admin-grp/*.png; do
		[ -e "$f" ] || continue
		if [ "$first" = 1 ]; then first=0; else printf ','; fi
		printf '"%s"' "$(basename "$f")"
	done
	printf ']}\n'
} > "$DIST/admin-grp/index.json"
# パーツ図鑑の最初の下書き（サーバーに SboWebPartsDex.json が無い時に sbosvr_mapdesign.js が読む）
cp "$ROOT/web/account/parts-dex-seed.json" "$DIST/admin-grp/"

# サーバー状況のページ（/status。サーバーの /api/status を読む）。フッターの版も書き込む
sed "s/@@SBO_BUILD_DATE@@/$BUILD_DATE/g; s/@@SBO_BUILD_COMMIT@@/$BUILD_COMMIT/g" "$ROOT/web/status/status.html" > "$DIST/status.html"

# 更新履歴のページ（/changes。サーバーの /api/changes を読む）と最初の中身（sbosvr_changes.js が取り込む）
sed "s/@@SBO_BUILD_DATE@@/$BUILD_DATE/g; s/@@SBO_BUILD_COMMIT@@/$BUILD_COMMIT/g" "$ROOT/web/changes/changes.html" > "$DIST/changes.html"
cp "$ROOT/web/changes/changes.json" "$DIST/changes-seed.json"

# クレジットのページ（/credits。使わせていただいている BGM・効果音などの一覧。中身は credits.json）
sed "s/@@SBO_BUILD_DATE@@/$BUILD_DATE/g; s/@@SBO_BUILD_COMMIT@@/$BUILD_COMMIT/g" "$ROOT/web/credits/credits.html" > "$DIST/credits.html"
cp "$ROOT/web/credits/credits.json" "$DIST/credits.json"

echo "[build] 完了: $DIST/sbocli.html"
