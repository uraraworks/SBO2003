#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ピンボールの盤の絵（差し替え絵）を描くためのテンプレート PNG を作る。

  python3 tools/pinball/make_art_template.py [-o 出力フォルダ]

  - 盤の寸法・当たり判定は common/Pinball.h から（tools/pinball/pinball_dump.cpp をコンパイルして書き出させる）。
    台の形を変えても、このスクリプトを流し直せばガイドが追従する。
  - 部品の絵の並びは SboCli/src/SboCliPinball.h の PBART_* から読む（ゲーム本体と同じ値）。
  - 必要なもの：Python 3 + Pillow（pip install pillow）、C++ コンパイラ（g++ / clang++。環境変数 CXX でも指定できる）。
    コンパイラが無い時は、別の機械で書き出した JSON を --dump-json で渡す。

出力（既定は ./out/pinball-art）：
  pinball_bg_guide.png        背景 160×448 に当たり判定のガイド（薄い色）を重ねたもの（描く時の下敷き。ゲームには使わない）
  pinball_bg_guide_x3.png     同じものを 3 倍に拡大（人が眺める用）
  pinball_bg_empty.png        背景の空版（透明＝黒のインデックスカラー PNG。この上に描いて pinball_bg.png にする）
  pinball_parts_guide.png     部品シート 128×96 に枠・番号・中心・当たり判定のガイドを重ねたもの
  pinball_parts_guide_x6.png  同じものを 6 倍に拡大
  pinball_parts_empty.png     部品シートの空版（同上）
  layout.json                 寸法の一覧（Codex などに読ませる用）
詳しくは docs/pinball-art.md。
"""
import argparse
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

try:
	from PIL import Image, ImageDraw, ImageFont
except ImportError:
	sys.exit('Pillow が必要です: pip install pillow')

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))


def load_layout(json_path):
	"""盤の寸法を読む。json_path が無ければ pinball_dump.cpp をコンパイルして実行する"""
	if json_path:
		with open(json_path, encoding='utf-8') as f:
			return json.load(f)
	cxx = os.environ.get('CXX') or shutil.which('g++') or shutil.which('clang++') or shutil.which('c++')
	if not cxx:
		sys.exit('C++ コンパイラ（g++ / clang++）が見つかりません。--dump-json で書き出した JSON を渡すか、CXX を指定してください')
	with tempfile.TemporaryDirectory() as tmp:
		exe = os.path.join(tmp, 'pinball_dump.exe' if os.name == 'nt' else 'pinball_dump')
		src = os.path.join(ROOT, 'tools', 'pinball', 'pinball_dump.cpp')
		subprocess.check_call([cxx, '-std=c++11', '-I' + ROOT, src, '-o', exe])
		return json.loads(subprocess.check_output([exe]).decode('utf-8'))


def load_parts_layout():
	"""SboCliPinball.h の PBART_* を読む（部品シートの並び。ゲーム本体と同じ値）"""
	vals = {}
	with open(os.path.join(ROOT, 'SboCli', 'src', 'SboCliPinball.h'), encoding='utf-8') as f:
		for line in f:
			m = re.match(r'#define\s+(PBART_[A-Z_]+)\s+\((\d+)\)', line)
			if m:
				vals[m.group(1)] = int(m.group(2))
	return vals


def font(size):
	try:
		return ImageFont.load_default(size=size)
	except TypeError:
		return ImageFont.load_default()


class Guide:
	"""拡大率 S で、ドット座標（小数可）のまま図形を描く薄い色の重ね描き"""
	def __init__(self, w, h, scale, base):
		self.s = scale
		self.base = Image.new('RGBA', (w * scale, h * scale), base + (255,))
		self.lay = Image.new('RGBA', self.base.size, (0, 0, 0, 0))
		self.d = ImageDraw.Draw(self.lay)

	def p(self, x, y):
		return (x * self.s, y * self.s)

	def line(self, x1, y1, x2, y2, col, w=1):
		self.d.line([self.p(x1, y1), self.p(x2, y2)], fill=col, width=max(1, int(w * (1 if self.s == 1 else self.s / 2))))

	def circle(self, cx, cy, r, col, w=1, fill=None):
		a = self.p(cx - r, cy - r)
		b = self.p(cx + r, cy + r)
		self.d.ellipse([a, (b[0] - 1, b[1] - 1)], outline=col, fill=fill, width=max(1, int(w * (1 if self.s == 1 else self.s / 2))))

	def arc(self, cx, cy, r, a0, a1, col, w=1):
		pts = [self.p(cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a))) for a in range(a0, a1 + 1, 3)]
		self.d.line(pts, fill=col, width=max(1, int(w * (1 if self.s == 1 else self.s / 2))))

	def rect(self, x, y, w, h, col, wd=1):
		a = self.p(x, y)
		b = self.p(x + w, y + h)
		self.d.rectangle([a, (b[0] - 1, b[1] - 1)], outline=col, width=max(1, int(wd * (1 if self.s == 1 else self.s / 2))))

	def text(self, x, y, t, col, size=7):
		self.d.text(self.p(x, y), t, fill=col, font=font(size * (1 if self.s == 1 else max(1, self.s // 2))))

	def capsule(self, x1, y1, x2, y2, r, col):
		"""太さのある線分（フリッパー・ターゲット）の輪郭"""
		dx, dy = x2 - x1, y2 - y1
		n = math.hypot(dx, dy) or 1
		nx, ny = -dy / n * r, dx / n * r
		self.line(x1 + nx, y1 + ny, x2 + nx, y2 + ny, col)
		self.line(x1 - nx, y1 - ny, x2 - nx, y2 - ny, col)
		ang = math.degrees(math.atan2(dy, dx))
		self.arc(x2, y2, r, int(ang - 90), int(ang + 90), col)
		self.arc(x1, y1, r, int(ang + 90), int(ang + 270), col)

	def finish(self):
		return Image.alpha_composite(self.base, self.lay).convert('RGB')


C_WALL = (110, 220, 255, 200)		# 壁・アーチ（球が触れる面）
C_BUMP = (255, 190, 80, 220)		# バンパー
C_TGT = (255, 240, 90, 220)			# ターゲット
C_FLIP = (255, 130, 220, 220)		# フリッパー
C_DRAIN = (255, 90, 90, 220)		# 穴
C_START = (110, 255, 130, 220)		# 打ち出し待ちの球
C_GRID = (255, 255, 255, 28)
C_GRID4 = (255, 255, 255, 60)
C_TEXT = (255, 255, 255, 200)


def empty_indexed(w, h):
	"""空版：0 番（黒＝透明）で塗った 8 ビットのインデックスカラー PNG。ゲームが読める形（256 色まで）"""
	im = Image.new('P', (w, h), 0)
	pal = [0] * 768
	im.putpalette(pal)
	return im


def draw_bg_guide(L, S):
	W, H = L['W'], L['H']
	g = Guide(W, H, S, (58, 60, 72))
	# 16 ドット格子（64 ごとに濃く）
	for x in range(0, W + 1, 16):
		g.line(x, 0, x, H, C_GRID4 if x % 64 == 0 else C_GRID, 1)
	for y in range(0, H + 1, 16):
		g.line(0, y, W, y, C_GRID4 if y % 64 == 0 else C_GRID, 1)
	# 見える窓（上端の窓と、下端の窓。盤が窓より高いと窓が上下に動く）
	g.line(0, L['VIEW_H'], W, L['VIEW_H'], (255, 255, 255, 110), 1)
	g.line(0, L['YOFS'], W, L['YOFS'], (255, 255, 255, 110), 1)
	g.text(2, L['VIEW_H'] - 9, 'y=%d window(top) end' % L['VIEW_H'], C_TEXT)
	g.text(2, L['YOFS'] + 1, 'y=%d window(bottom) start' % L['YOFS'], C_TEXT)
	# 壁（球が触れる面）とアーチ
	for x1, y1, x2, y2 in L['walls']:
		g.line(x1, y1, x2, y2, C_WALL, 1)
	a = L['arch']
	g.arc(a['cx'], a['cy'], a['r'], 180, 360, C_WALL, 1)
	# バンパー（当たり判定の円。セル 32×32 も薄く）
	for i, (bx, by) in enumerate(L['bumpers']):
		g.circle(bx, by, L['BUMPER_R'], C_BUMP, 1)
		g.line(bx - 3, by, bx + 3, by, C_BUMP, 1)
		g.line(bx, by - 3, bx, by + 3, C_BUMP, 1)
		g.text(bx + 10, by - 4, 'B%d' % i, C_BUMP)
	# ターゲット（線分＋太さ）
	for x1, y1, x2, y2, n in L['targets']:
		g.capsule(x1, y1, x2, y2, L['TARGET_R'], C_TGT)
		g.text(x2 + 2, y1 - 4, 'T%d' % n, C_TGT)
	# フリッパー（休み・上がり。左右）と軸
	pv = L['pivot']
	for side, px in (('L', pv['lx']), ('R', pv['rx'])):
		sign = 1 if side == 'L' else -1
		for deg in (L['FLIP_REST_DEG'], L['FLIP_UP_DEG']):
			tx = px + sign * L['FLIP_LEN'] * math.cos(math.radians(deg))
			ty = pv['y'] + L['FLIP_LEN'] * math.sin(math.radians(deg))
			g.capsule(px, pv['y'], tx, ty, L['FLIP_R'], C_FLIP)
		g.circle(px, pv['y'], 1.5, C_FLIP, 1)
	# 穴（これより下に球の中心が行くと落ちる）・レーン・打ち出し待ちの球
	g.line(0, L['drain_y'], W, L['drain_y'], C_DRAIN, 1)
	g.text(2, L['drain_y'] - 9, 'drain', C_DRAIN)
	g.line(L['lane_left'], 0, L['lane_left'], H, (110, 255, 130, 60), 1)
	g.circle(L['start']['x'], L['start']['y'], L['BALL_R'], C_START, 1)
	g.text(L['lane_left'] + 2, L['start']['y'] - 14, 'ball', C_START)
	return g.finish()


def draw_parts_guide(P, L, S):
	W, H = P['PBART_PARTS_W'], P['PBART_PARTS_H']
	g = Guide(W, H, S, (58, 60, 72))
	for x in range(0, W + 1, 16):
		g.line(x, 0, x, H, C_GRID4 if x % 32 == 0 else C_GRID, 1)
	for y in range(0, H + 1, 16):
		g.line(0, y, W, y, C_GRID4 if y % 32 == 0 else C_GRID, 1)

	def cell(n, x, y, size, label, h=None):
		h = h or size
		g.rect(x, y, size, h, (255, 255, 255, 130), 1)
		g.text(x + 1, y + 1, str(n), C_TEXT, 7)
		return x + size / 2.0, y + h / 2.0

	# 1 ボール
	cx, cy = cell(1, P['PBART_BALL_X'], P['PBART_BALL_Y'], P['PBART_BALL_S'], 'ball')
	g.circle(cx, cy, L['BALL_R'], C_START, 1)
	# 2〜4 ターゲット（立つ・倒れる・光る）。中心に線分＋太さ
	for n, x in ((2, P['PBART_TARGET_UP_X']), (3, P['PBART_TARGET_DOWN_X']), (4, P['PBART_TARGET_HIT_X'])):
		cx, cy = cell(n, x, P['PBART_TARGET_Y'], P['PBART_TARGET_W'], 'target', P['PBART_TARGET_H'])
		t = L['targets'][0]
		half = abs(t[2] - t[0]) / 2.0		# 当たり判定の線分の半分の長さ（太さ TARGET_R の丸い端が両側に付く）
		g.capsule(cx - half, cy, cx + half, cy, L['TARGET_R'], C_TGT)
	# 5・6 バンパー（ふつう・光る）
	for n, x in ((5, P['PBART_BUMPER_X']), (6, P['PBART_BUMPER_HIT_X'])):
		cx, cy = cell(n, x, P['PBART_BUMPER_Y'], P['PBART_BUMPER_S'], 'bumper')
		g.circle(cx, cy, L['BUMPER_R'], C_BUMP, 1)
		g.line(cx - 3, cy, cx + 3, cy, C_BUMP, 1)
		g.line(cx, cy - 3, cx, cy + 3, C_BUMP, 1)
	# 7・8 左フリッパー（下がり・上がり）。軸の位置に印、その姿の当たり判定（カプセル）
	for n, x, deg in ((7, P['PBART_FLIP_REST_X'], L['FLIP_REST_DEG']), (8, P['PBART_FLIP_UP_X'], L['FLIP_UP_DEG'])):
		cell(n, x, P['PBART_FLIP_Y'], P['PBART_FLIP_S'], 'flipper')
		px, py = x + P['PBART_FLIP_PX'], P['PBART_FLIP_Y'] + P['PBART_FLIP_PY']
		tx = px + L['FLIP_LEN'] * math.cos(math.radians(deg))
		ty = py + L['FLIP_LEN'] * math.sin(math.radians(deg))
		g.capsule(px, py, tx, ty, L['FLIP_R'], C_FLIP)
		g.circle(px, py, 1.5, C_FLIP, 1)
	# 9・10 打ち出しのバネ（頭・コイル）。↓で溜める間、頭と球がレーンの床の下の「穴」へ沈み、コイルが縮む（見た目だけ）
	cx, cy = cell(9, P['PBART_SPRING_X'], P['PBART_SPRING_HEAD_Y'], P['PBART_SPRING_HEAD_W'], 'head', P['PBART_SPRING_HEAD_H'])
	g.line(P['PBART_SPRING_X'], P['PBART_SPRING_HEAD_Y'], P['PBART_SPRING_X'] + P['PBART_SPRING_HEAD_W'], P['PBART_SPRING_HEAD_Y'], C_START, 1)
	g.text(P['PBART_SPRING_X'] - 15, P['PBART_SPRING_HEAD_Y'] - 1, 'ball', C_START, 6)
	cell(10, P['PBART_SPRING_X'], P['PBART_SPRING_COIL_Y'], P['PBART_SPRING_COIL_W'], 'coil', P['PBART_SPRING_COIL_H'])
	g.text(P['PBART_SPRING_X'] - 12, P['PBART_SPRING_COIL_Y'] + 6, 'stretch', C_TEXT, 6)
	return g.finish()


def main():
	ap = argparse.ArgumentParser(description='ピンボールの盤の絵のテンプレートを作る')
	ap.add_argument('-o', '--out', default=os.path.join(ROOT, 'out', 'pinball-art'), help='出力フォルダ（既定: out/pinball-art）')
	ap.add_argument('--dump-json', help='pinball_dump が書き出した JSON（コンパイラが無い時）')
	a = ap.parse_args()
	L = load_layout(a.dump_json)
	P = load_parts_layout()
	os.makedirs(a.out, exist_ok=True)
	out = lambda n: os.path.join(a.out, n)

	draw_bg_guide(L, 1).save(out('pinball_bg_guide.png'))
	draw_bg_guide(L, 3).save(out('pinball_bg_guide_x3.png'))
	empty_indexed(L['W'], L['H']).save(out('pinball_bg_empty.png'))
	draw_parts_guide(P, L, 1).save(out('pinball_parts_guide.png'))
	draw_parts_guide(P, L, 6).save(out('pinball_parts_guide_x6.png'))
	empty_indexed(P['PBART_PARTS_W'], P['PBART_PARTS_H']).save(out('pinball_parts_empty.png'))
	with open(out('layout.json'), 'w', encoding='utf-8') as f:
		json.dump({'board': L, 'parts_sheet': P}, f, ensure_ascii=False, indent=1)
	print('出力:', a.out)
	for n in sorted(os.listdir(a.out)):
		print(' ', n)


if __name__ == '__main__':
	main()
