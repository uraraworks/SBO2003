#!/bin/bash
# 弓のテストは公開の最小データで実行する。
set -euo pipefail
cd "$(dirname "$0")/../.."
source "${EMSDK_ENV:-$HOME/emsdk/emsdk_env.sh}" >/dev/null 2>&1
cmake --preset mac-release >/dev/null
cmake --build --preset mac-release --target SboSvrTests -j8
BOW_TEST_WORK="$(mktemp -d "${TMPDIR:-/tmp}/sbo-bow-test.XXXXXX")"
trap 'rm -rf "$BOW_TEST_WORK"' EXIT
cp -R Release/SBODATA "$BOW_TEST_WORK/"
cp Release/sbosvr.ini "$BOW_TEST_WORK/"
node out/server-release/SboSvrTests.js "$BOW_TEST_WORK" --bow
