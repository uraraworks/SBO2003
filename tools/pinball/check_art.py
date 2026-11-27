#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ピンボールの差し替え絵（pinball_bg.png・pinball_parts.png）が、ゲームの読める形か調べる。

  python3 tools/pinball/check_art.py <pinball_bg.png> <pinball_parts.png> [--preview 出力.png]

  - 形式（インデックスカラー・256 色まで）・大きさ・色数・黒（透明）の使い方を調べる
  - --preview：絵に当たり判定のガイドを重ねた画像（背景は 3 倍）を出す。位置合わせの確認用
終了コード 0=問題なし、1=直すところがある。
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_art_template as T
from PIL import Image


def check(path, w, h, label, ng):
	if not os.path.exists(path):
		ng.append('%s: ファイルがありません: %s' % (label, path))
		return None
	im = Image.open(path)
	if im.format != 'PNG':
		ng.append('%s: PNG ではありません' % label)
	if im.mode != 'P':
		ng.append('%s: インデックスカラー（パレット）の PNG ではありません（今: %s）。RGB・RGBA はゲームが読めず、図形で描くことになります。'
			'Photoshop なら「モード→インデックスカラー」、Pillow なら im.convert("RGB").quantize(colors=64) で変換してください' % (label, im.mode))
		return None
	if 'transparency' in im.info:
		print('%s: 注意: PNG の透明情報（tRNS）は使いません。透明は黒 (0,0,0) です' % label)
	if im.size != (w, h):
		ng.append('%s: 大きさが違います（今 %dx%d、必要 %dx%d）' % (label, im.size[0], im.size[1], w, h))
	rgb = im.convert('RGB')
	cols = rgb.getcolors(maxcolors=1 << 20)
	n = len(cols)
	black = sum(c for c, v in cols if v == (0, 0, 0))
	print('%s: %dx%d  使っている色 %d  透明（黒）のドット %d / %d (%.0f%%)' % (label, rgb.size[0], rgb.size[1], n, black, rgb.size[0] * rgb.size[1], 100.0 * black / (rgb.size[0] * rgb.size[1])))
	if n > 64:
		print('%s: 注意: 色が多めです（%d 色）。SBO らしさのため、背景は 48 色・部品は 32 色くらいまでを目安に絞ってください' % (label, n))
	# ほぼ黒（縁取りのつもりで (1,1,1) など）は、透明とは別の色として描かれる（問題ではないが、気づきやすいように知らせる）
	near = sum(c for c, v in cols if v != (0, 0, 0) and max(v) <= 7)
	if near:
		print('%s: 注意: ほぼ黒（各値 7 以下）のドットが %d 個あります。縁取りは (16,16,32) くらいの暗い色にすると、黒＝透明と見分けがつきます' % (label, near))
	return rgb


def main():
	ap = argparse.ArgumentParser(description='ピンボールの差し替え絵を調べる')
	ap.add_argument('bg')
	ap.add_argument('parts')
	ap.add_argument('--preview', help='ガイドを重ねたプレビュー PNG の出力先')
	ap.add_argument('--dump-json')
	a = ap.parse_args()
	L = T.load_layout(a.dump_json)
	P = T.load_parts_layout()
	ng = []
	bg = check(a.bg, L['W'], L['H'], '背景', ng)
	parts = check(a.parts, P['PBART_PARTS_W'], P['PBART_PARTS_H'], '部品', ng)
	if parts is not None and not ng:
		# バネの絵（頭・コイル）が空なら、バネは図形（線のジグザグ）で描かれる
		def used(x, y, w, h):
			return any(parts.getpixel((i, j)) != (0, 0, 0) for j in range(y, y + h) for i in range(x, x + w))
		if not (used(P['PBART_SPRING_X'], P['PBART_SPRING_HEAD_Y'], P['PBART_SPRING_HEAD_W'], P['PBART_SPRING_HEAD_H']) and
				used(P['PBART_SPRING_X'], P['PBART_SPRING_COIL_Y'], P['PBART_SPRING_COIL_W'], P['PBART_SPRING_COIL_H'])):
			print('部品: 注意: バネの絵（9 頭・10 コイル）が空です。打ち出しのバネは図形で描かれます')
	if a.preview and bg is not None and not ng:
		# 下地の上に絵（黒＝透明は下地のまま）を載せ、当たり判定のガイドの線だけをその上に重ねる（3 倍）
		size = (bg.width * 3, bg.height * 3)
		out = Image.new('RGB', size, (58, 60, 72))
		art = bg.resize(size, Image.NEAREST)
		out.paste(art, (0, 0), art.convert('L').point(lambda v: 255 if v else 0))
		guide = T.draw_bg_guide(L, 3)
		plain = T.Guide(L['W'], L['H'], 3, (58, 60, 72)).finish()
		gp, pp, op = guide.load(), plain.load(), out.load()
		for y in range(size[1]):
			for x in range(size[0]):
				if gp[x, y] != pp[x, y]:
					op[x, y] = gp[x, y]
		out.save(a.preview)
		print('プレビュー:', a.preview)
	for m in ng:
		print('NG:', m)
	if not ng:
		print('OK: ゲームが読める形です')
	return 1 if ng else 0


if __name__ == '__main__':
	sys.exit(main())
