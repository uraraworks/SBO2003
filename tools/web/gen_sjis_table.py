#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CP932(Shift-JIS) → Unicode の変換表を C++ のソースとして書き出す。
Web版の互換レイヤが、SJIS の文字列を描画するときに使う。

使い方: gen_sjis_table.py > platform/sjis_table.inc
"""
import sys

LEADS = list(range(0x81, 0xA0)) + list(range(0xE0, 0xFD))

out = sys.stdout
out.write('/* gen_sjis_table.py が生成したファイル。手で編集しないこと */\n')
out.write('/* CP932 の 2 バイト文字 → Unicode。添字は [リードバイトの番号][トレイルバイト - 0x40] */\n')
out.write('static const unsigned short s_awSjisToUcs[%d][189] = {\n' % len(LEADS))
for lead in LEADS:
	row = []
	for trail in range(0x40, 0xFD):
		try:
			ch = bytes([lead, trail]).decode('cp932')
			row.append(ord(ch) if ord(ch) <= 0xFFFF else 0)
		except UnicodeDecodeError:
			row.append(0)
	out.write('\t{' + ','.join('0x%04X' % v for v in row) + '},\n')
out.write('};\n')
