#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
UTF-8 のソースを em++(clang) でコンパイルできる形に整えて別ディレクトリへ書き出す前処理ツール。

もとは文字列リテラルを Shift-JIS(CP932) に置き換えるためのツールだったが、
ゲーム内部の文字コードを UTF-8 にした（UTF-8 対応の段階 3）ので、今は文字列はそのまま通す。
Windows(VC6) 由来のソースをそのまま通すための手直しだけをする。

変換ルール:
  - コメント・文字列     : そのまま（UTF-8 のまま）
  - #include "..."       : '\\' を '/' に直し、大文字小文字の違いを実在するパスに合わせる
  - コード中の全角空白   : タブにする
  - 行数は変えないので、コンパイルエラーの行番号は元ファイルと一致する
    （先頭に #line を入れて、エラー表示のファイル名も元ファイルにする）

使い方:
  src_prepass.py [-I <インクルードディレクトリ>]... <リポジトリルート> <出力先> <ディレクトリ>...
  例: src_prepass.py -I SboCli . out/web/src SboCli common SboSoundData
"""

import os
import re
import sys

EXTS = ('.cpp', '.c', '.h', '.hpp', '.inl')


def resolve_include(base_dir, inc):
	"""大文字小文字を区別しないで include パスを解決する。見つからなければ None"""
	parts = inc.replace('\\', '/').split('/')
	cur = base_dir
	out = []
	for p in parts:
		if p in ('', '.'):
			out.append(p)
			continue
		if p == '..':
			cur = os.path.dirname(cur)
			out.append(p)
			continue
		try:
			names = os.listdir(cur)
		except OSError:
			return None
		if p in names:
			hit = p
		else:
			low = [n for n in names if n.lower() == p.lower()]
			if not low:
				return None
			hit = low[0]
		out.append(hit)
		cur = os.path.join(cur, hit)
	return '/'.join(out)


INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*)"([^"]+)"')


INCLUDE_DIRS = []


def fix_include_line(line, src_dir):
	m = INCLUDE_RE.match(line)
	if not m:
		return line
	inc = m.group(2)
	fixed = resolve_include(src_dir, inc)
	if fixed is None:
		# インクルードパス側で見つかる場合は、そのディレクトリ基準で大文字小文字を合わせる
		for d in INCLUDE_DIRS:
			hit = resolve_include(d, inc)
			if hit is not None:
				fixed = hit
				break
	if fixed is None:
		# 親フォルダ基準なら見つかる場合（元の環境では通っていた相対パスのずれ）は、そのパスに直す
		cur = src_dir
		prefix = ''
		for _ in range(3):
			cur = os.path.dirname(cur)
			prefix += '../'
			hit = resolve_include(cur, inc)
			if hit is not None:
				fixed = prefix + hit
				break
	if fixed is None:
		# 見つからない場合は区切りだけ直して include パス検索に任せる
		fixed = inc.replace('\\', '/')
	return m.group(1) + '"' + fixed + '"' + line[m.end():]


def convert(text, src_dir):
	"""デコード済みテキストを変換する"""
	out = []
	i = 0
	n = len(text)
	state = 'code'
	line_start = True
	while i < n:
		c = text[i]
		if state == 'code':
			if line_start:
				# 行頭の #include を処理
				j = text.find('\n', i)
				if j < 0:
					j = n
				line = text[i:j]
				if INCLUDE_RE.match(line):
					out.append(fix_include_line(line, src_dir))
					i = j
					line_start = False
					continue
			line_start = False
			if c == '/' and i + 1 < n and text[i + 1] == '/':
				state = 'line_comment'
				out.append('//')
				i += 2
				continue
			if c == '/' and i + 1 < n and text[i + 1] == '*':
				state = 'block_comment'
				out.append('/*')
				i += 2
				continue
			if c == '"' or c == "'":
				quote = c
				out.append(c)
				i += 1
				while i < n:
					d = text[i]
					if d == '\\' and i + 1 < n:
						out.append(d)
						e = text[i + 1]
						out.append(e)
						i += 2
						continue
					if d == quote or d == '\n':
						out.append(d)
						i += 1
						break
					out.append(d)
					i += 1
				continue
			if c == '\n':
				line_start = True
			# コード中の全角空白はタブと同じ扱いにする
			out.append('\t' if c == '　' else c)
			i += 1
			continue
		if state == 'line_comment':
			if c == '\n':
				state = 'code'
				line_start = True
				out.append(c)
			elif c == '\\' and i + 1 < n and text[i + 1] in '\r\n':
				# 行コメント末尾の \ は行継続になるので消す
				pass
			else:
				out.append(c)
			i += 1
			continue
		if state == 'block_comment':
			if c == '*' and i + 1 < n and text[i + 1] == '/':
				state = 'code'
				out.append('*/')
				i += 2
				continue
			out.append(c)
			i += 1
			continue
	return ''.join(out)


def process_file(src, dst, rel):
	with open(src, 'rb') as f:
		raw = f.read()
	try:
		text = raw.decode('utf-8')
	except UnicodeDecodeError:
		# 古い SJIS のファイルが残っていた場合も読めるようにしておく
		text = raw.decode('cp932', errors='replace')
		print('[src-prepass] 警告: UTF-8 ではないファイルです(CP932 として読みました): ' + rel, file=sys.stderr)
	if text.startswith('﻿'):
		text = text[1:]
	text = text.replace('\r\n', '\n')
	body = convert(text, os.path.dirname(src))
	# エラー表示のファイル名を元ファイルにする（行番号は変わらない）
	header = '#line 1 "%s"\n' % os.path.abspath(src).replace('\\', '/')
	data = (header + body).encode('utf-8')
	if os.path.exists(dst):
		with open(dst, 'rb') as f:
			if f.read() == data:
				return False
	os.makedirs(os.path.dirname(dst), exist_ok=True)
	with open(dst, 'wb') as f:
		f.write(data)
	return True


def main():
	args = sys.argv[1:]
	inc_dirs = []
	while len(args) >= 2 and args[0] == '-I':
		inc_dirs.append(args[1])
		args = args[2:]
	if len(args) < 3:
		print(__doc__)
		return 2
	root = os.path.abspath(args[0])
	out_root = os.path.abspath(args[1])
	INCLUDE_DIRS[:] = [os.path.join(root, d) for d in inc_dirs]
	changed = 0
	total = 0
	for d in args[2:]:
		for dirpath, _dirs, files in os.walk(os.path.join(root, d)):
			for name in files:
				if not name.lower().endswith(EXTS):
					continue
				src = os.path.join(dirpath, name)
				rel = os.path.relpath(src, root)
				dst = os.path.join(out_root, rel)
				total += 1
				if process_file(src, dst, rel):
					changed += 1
	print('[src-prepass] %d ファイル中 %d ファイルを更新しました' % (total, changed))
	return 0


if __name__ == '__main__':
	sys.exit(main())
