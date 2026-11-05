# ===========================================================================
# SBO2003 サーバー（SboSvr）をビルドする（Emscripten で wasm にして Node.js で動かす）
#
#     cmake --preset mac-debug && cmake --build --preset mac-debug
#     出力: out/server/SboSvr.js（+ .wasm）  起動: node out/server/SboSvr.js --data <フォルダ>
#
# wasm32 はポインタが 4 バイトなので、構造体の大きさが Windows 版と同じになる。
#
# Windows 版のソースはそのまま使い、platform/ の Win32 互換層と
# server/ の差し替え実装（ソケット・画面なし起動）でビルドする。
# ブラウザ版クライアントは tools/web/build.sh でビルドする（こちらは使わない）。
# ===========================================================================
cmake_minimum_required(VERSION 3.16)
project(Sbo2003 CXX)

if(NOT EMSCRIPTEN)
	message(FATAL_ERROR "Emscripten でビルドしてください（cmake --preset mac-debug）")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ---------------------------------------------------------------------------
# 前処理（UTF-8 のソース → 文字列リテラルを SJIS にしたソース）
#   設定時に一度流してソース一覧を作り、ビルドのたびにも流し直す
#   （中身が変わったファイルだけ書き直すので、変更の無いものは再コンパイルされない）
# ---------------------------------------------------------------------------
set(SBO_GEN "${CMAKE_BINARY_DIR}/sbosvr")
set(SBO_SRC "${SBO_GEN}/src")
execute_process(
	COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/server/prepare.py"
		"${CMAKE_SOURCE_DIR}" "${SBO_GEN}" --sources
	RESULT_VARIABLE SBO_PREPARE_RESULT)
if(NOT SBO_PREPARE_RESULT EQUAL 0)
	message(FATAL_ERROR "前処理（tools/server/prepare.py）に失敗しました")
endif()
include("${SBO_GEN}/sources.cmake")

# ソースが増減した時に設定し直す
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/tools/server/sources.txt"
	"${CMAKE_SOURCE_DIR}/tools/server/prepare.py")

add_custom_target(sbosvr_prepare
	COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/server/prepare.py"
		"${CMAKE_SOURCE_DIR}" "${SBO_GEN}"
	COMMENT "前処理（SJIS 化）"
	VERBATIM)

# ---------------------------------------------------------------------------
# 実行ファイル
# ---------------------------------------------------------------------------
add_executable(SboSvr
	${SBOSVR_GAME_SOURCES}
	platform/w32_core.cpp
	platform/w32_gdi.cpp
	platform/w32_ctrl.cpp
	platform/w32_dlg.cpp
	platform/w32_sjis.cpp
	platform/w32_file.cpp
	server/CUraraSockTCP_svr.cpp
	server/w32_headless.cpp
	server/SvrStub.cpp
	server/web_account_svr.cpp
	server/web_status_svr.cpp
	server/web_data_svr.cpp
	server/server_main.cpp)
add_dependencies(SboSvr sbosvr_prepare)

target_include_directories(SboSvr PRIVATE
	"${CMAKE_SOURCE_DIR}/platform/include"
	"${SBO_SRC}"
	"${SBO_SRC}/SboSvr"
	"${SBO_SRC}/SboSvr/src"
	"${SBO_SRC}/common")
target_compile_options(SboSvr PRIVATE
	-fms-extensions -fno-strict-aliasing -w)
# 画面なしで動かす。実行ファイル名は GetModuleFileName が返す名前
target_compile_definitions(SboSvr PRIVATE W32_HEADLESS W32_EXE_NAME="SboSvr.exe")

set(SBO_PREJS "${CMAKE_SOURCE_DIR}/server/node/sbosvr_net.js")
set(SBO_PREJS_ACCOUNT "${CMAKE_SOURCE_DIR}/server/node/sbosvr_account.js")
set(SBO_PREJS_STATUS "${CMAKE_SOURCE_DIR}/server/node/sbosvr_status.js")
set(SBO_PREJS_FEEDBACK "${CMAKE_SOURCE_DIR}/server/node/sbosvr_feedback.js")
set(SBO_PREJS_DATA "${CMAKE_SOURCE_DIR}/server/node/sbosvr_data.js")
set(SBO_PREJS_MAPDESIGN "${CMAKE_SOURCE_DIR}/server/node/sbosvr_mapdesign.js")
target_compile_options(SboSvr PRIVATE -sUSE_ZLIB=1)
target_link_options(SboSvr PRIVATE
	-sUSE_ZLIB=1
	-sENVIRONMENT=node
	-sNODERAWFS=1
	-sEXIT_RUNTIME=1
	-sALLOW_MEMORY_GROWTH=1
	-sINITIAL_MEMORY=64MB
	-sSTACK_SIZE=4MB
	-sGLOBAL_BASE=1048576
	-sEXPORTED_FUNCTIONS=_main,_malloc,_free
	"-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToUTF8,$UTF8ToString,$stringToNewUTF8"
	"--pre-js=${SBO_PREJS}"
	"--pre-js=${SBO_PREJS_ACCOUNT}"
	"--pre-js=${SBO_PREJS_STATUS}"
	"--pre-js=${SBO_PREJS_FEEDBACK}"
	"--pre-js=${SBO_PREJS_DATA}"
	"--pre-js=${SBO_PREJS_MAPDESIGN}")
set_property(TARGET SboSvr APPEND PROPERTY LINK_DEPENDS "${SBO_PREJS}" "${SBO_PREJS_ACCOUNT}" "${SBO_PREJS_STATUS}" "${SBO_PREJS_FEEDBACK}" "${SBO_PREJS_DATA}" "${SBO_PREJS_MAPDESIGN}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# C++ のソースでブレークポイントを使えるよう DWARF を残す（VSCode の WebAssembly DWARF Debugging 拡張で見られる）
	target_link_options(SboSvr PRIVATE -g -sASSERTIONS=1)
endif()
set_target_properties(SboSvr PROPERTIES SUFFIX ".js")
