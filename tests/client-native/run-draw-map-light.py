#!/usr/bin/env python3
"""本体の描画部分を毎回抽出し、旧実装と比較する。ゲームのビルドには入れない。"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parent.parent
source = (root / "SboCli/src/SboCliDrawScreen.cpp").read_text()
function = source.index("static void DrawMapLight(const char *pMapDark)\n{")
# 事前計算を含め、光源収集直後から関数末尾まで取り出す。
start = source.index("\tfloat afR2[", function)
end = source.index("\n}\n", start)
new = source[start:end]
old = (here / "draw-map-light-old.inc").read_text()
with tempfile.TemporaryDirectory(prefix="sbo-light-") as directory:
    directory = Path(directory)
    functions = []
    for name, body in (("Old", old), ("New", new)):
        functions.append("void Scene::" + name + "(std::vector<DWORD> &buffer) const\n{\n" + """
	int i, x, y, nCX, nCY, nSX, nSY;
	float afVisX[DRAW_PARTS_X * 2 + 3];
	BYTE *pBits = reinterpret_cast<BYTE *>(buffer.data());
	const Character *pMyChar = &character;
	const char *pMapDark = dark.data();
""" + body + "\n}\n")
    (directory / "draw-map-light-functions.inc").write_text("\n".join(functions))
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    # Wasm の通常の演算と同じく、積和を融合しない。fast-math は使わない。
    flags = shlex.split(os.environ.get("CXXFLAGS", "-O2 -ffp-contract=off"))
    wasm = any("em++" in part for part in compiler)
    binary = directory / ("draw-map-light.js" if wasm else "draw-map-light")
    subprocess.run(compiler + ["-std=c++17"] + flags + ["-I", str(directory),
        str(here / "draw-map-light.cpp"), "-o", str(binary)], check=True)
    subprocess.run((["node"] if wasm else []) + [str(binary)], check=True)
