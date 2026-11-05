#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
サーバー（SboSvr）をネイティブ（Mac/Linux）でビルドするための前処理。

  1. SboSvr と common を src_prepass.py で <出力先>/src へ書き出す
     （文字列リテラルを SJIS の 8 進エスケープにする。中身が同じなら書き直さない）
  2. --sources を付けた時は tools/server/sources.txt に載っているソースの一覧を
     <出力先>/sources.cmake に書き出す

使い方: prepare.py <リポジトリルート> <出力先> [--sources]
"""

import os
import subprocess
import sys



def list_sources(root):
	"""tools/server/sources.txt に載っている .cpp を、リポジトリルートからの相対パスで返す"""
	out = []
	with open(os.path.join(root, 'tools', 'server', 'sources.txt'), encoding='utf-8') as f:
		for line in f:
			line = line.strip()
			if line and not line.startswith('#'):
				out.append(line)
	return out


def main():
	args = [a for a in sys.argv[1:] if not a.startswith('--')]
	if len(args) != 2:
		print(__doc__)
		return 1
	root, dst = os.path.abspath(args[0]), os.path.abspath(args[1])
	src = os.path.join(dst, 'src')

	subprocess.check_call([sys.executable, os.path.join(root, 'tools', 'web', 'src_prepass.py'),
		'-I', 'SboSvr', '-I', 'SboSvr/src', root, src, 'SboSvr', 'common'], stdout=subprocess.DEVNULL)

	if '--sources' in sys.argv:
		lines = ['# prepare.py が生成（手で編集しない）', 'set(SBOSVR_GAME_SOURCES']
		lines += ['\t"%s/%s"' % (src.replace('\\', '/'), p) for p in list_sources(root)]
		lines.append(')')
		text = '\n'.join(lines) + '\n'
		path = os.path.join(dst, 'sources.cmake')
		old = open(path, encoding='utf-8').read() if os.path.exists(path) else None
		if old != text:
			with open(path, 'w', encoding='utf-8') as f:
				f.write(text)
	return 0


if __name__ == '__main__':
	sys.exit(main())
