#!/usr/bin/env python3
"""本体の座標一覧・灯り更新を抽出する独立した計測・検証。"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parent.parent
source = (root / "SboCli/src/SboCliRecvCmd.cpp").read_text()
names = "RequestCharPosInfo FlushCharPosInfo PrepareRecvCharPosInfo RenewCharPosInfo RenewMapDarkHideArea GetMyPosDarkType IsHideAreaCross IsMapHasHideArea IsMapHideArea GetMapLightLevel IsMapDarkFlg IsMapViewMode GetMapViewRange IsSightBlock IsInViewRange ResetMapViewSeen RenewMapViewSeen RenewMapViewLights RenewMapDark RenewMapDarkParts IsLightOtherRoom RenewMapDarkLightFlood GetMapPosLight IsInLightCircle GetCharLightRadius GetMapDarkLevel RenewMapDarkCorner RenewMapDarkWork CheckCharBlock IsSightWall".split()
def extract(text, name):
    match = re.search(r"^(?:static )?(?:void|BOOL|BYTE|int|float) " + name + r"\([^\n]*\)\n\{", text, re.M)
    assert match, name
    end = text.index("\n}\n", match.end()) + 3
    return text[match.start():end]

names += "IsMapHideShowTile GetCharNo GetMapBridgeAxis GetCharBridgeLayerNow GetBridgeLayerPos RenewCharBridgeLayer".split()
functions = [extract(source, name) for name in names]
old = (here / "renew-char-pos-old.inc").read_text()
# 更新時点だけを変える。浮動小数点を含む作り直し本体には変更がないことも検査する。
assert extract(source, "RenewCharPosInfo").replace("\n\ts_bCharPosInfoDirty = FALSE;\n", "", 1) == old.replace("RenewCharPosInfoOld(", "RenewCharPosInfo(", 1)
receivers = []
for name in ("RecvDataProcCHARINFO", "RecvDataProcCHARINFO2", "RecvDataProcCHARPOS"):
    body = extract(source, name)
    assert body.count("RequestCharPosInfo ();") == 1
    receivers += [body, body.replace(name + "(", name + "Old(", 1).replace("RequestCharPosInfo ();", "RenewCharPosInfoOld ();")]

# グローバル状態と定数も本体から取り出す。
state = source[source.index("static int s_nPartsLightX"):source.index("BOOL\tCheckEquUserInfo")]
state += "\nstatic BOOL s_bCharPosInfoDirty = FALSE;\n"
state += "\nstatic BYTE s_abyHideShow[SCRMARTSSIZE];\n"
state += source[source.index("#define VIEW_SEEN_LEVEL"):source.index("/* 今の場所で見える範囲(視界)が効くか")]
state += "\nstatic BOOL s_bLightIgnoreViewRange = FALSE;\n"
state += "\n".join(re.findall(r"^#define LIGHTCIRCLE_[^\n]*", source, re.M))
prototypes = "\n".join(f[:f.index("\n{")] + ";" for f in functions)
with tempfile.TemporaryDirectory(prefix="sbo-renew-") as directory:
    directory = Path(directory)
    (directory / "renew-functions.inc").write_text(prototypes + "\n" + state + "\n" + "\n".join(functions) + "\n" + old)
    main = (root / "SboCli/src/SboCliMain.cpp").read_text()
    delete = extract(main, "DeleteChar").replace("RenewCharPosInfo ();", "if (useOld) RenewCharPosInfoOld (); else RenewCharPosInfo ();")
    trade = (root / "SboCli/src/TypedefCli.h").read_text()
    trade = trade[trade.index("typedef struct _TRADEINFO"):trade.index("} TRADEINFO, *PTRADEINFO;") + len("} TRADEINFO, *PTRADEINFO;")]
    (directory / "renew-delete.inc").write_text(delete)
    (directory / "renew-trade.inc").write_text(trade)
    draw = (root / "SboCli/src/SboCliDrawScreen.cpp").read_text()
    (directory / "renew-draw.inc").write_text("#define MAPLIGHT_MAX 32\n" + extract(draw, "DrawMapLight"))
    (directory / "renew-receivers.inc").write_text("\n".join(receivers))
    # 通信の詰め直し・文字列・CRC も本体の実装を使用する。
    packet = re.sub(r'^#include[^\n]*', '', (root / "common/SboPacket.cpp").read_text(), flags=re.M)
    text_source = (root / "common/SboText.cpp").read_text()
    text_functions = "\n".join(extract(text_source, name) for name in ("Utf8SeqLen", "SboTextCharLen", "SboTextFitBytes", "SboTextCopy"))
    crc = re.sub(r'^#include[^\n]*', '', (root / "common/crc.cpp").read_text(), flags=re.M)
    (directory / "renew-packet.inc").write_text(text_functions + "\n" + packet + "\n" + crc)
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    flags = shlex.split(os.environ.get("CXXFLAGS", "-O2 -fno-strict-aliasing -ffp-contract=off"))
    wasm = any("em++" in part for part in compiler)
    if wasm:
        flags += ["-sALLOW_MEMORY_GROWTH=1", "-sINITIAL_MEMORY=64MB", "-sSTACK_SIZE=1MB"]
    binary = directory / ("renew.js" if wasm else "renew")
    subprocess.run(compiler + ["-std=c++17", "-D__EMSCRIPTEN__"] + flags + ["-I", str(root), "-I", str(directory),
        str(here / "renew-char-pos.cpp"), "-o", str(binary)], check=True)
    subprocess.run((["node"] if wasm else []) + [str(binary)], check=True)
