#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
リソースDLL（SboGrpData / SboSoundData）の .rc を読み、Web版で使うリソース表を作る。

Web版には DLL が無いので、FindResource / LoadResource / LoadString は
この表を引いて、preload したファイル（/res/<モジュール名>/...）を読む。

SboCli.rc からはダイアログとメニューの定義を読み、CreateDialog / LoadMenu 用の表にする。

出力:
  <出力先>/w32_resources.cpp   リソース表（C++）
  <出力先>/preload.txt         em++ に渡す --preload-file の一覧（1 行 1 つ、"元パス@配置先"）

使い方: gen_resources.py <リポジトリルート> <出力先>
"""
import os
import re
import sys
import unicodedata

# モジュール名(LoadLibrary に渡される名前) → (.rc, resource.h, 含めるリソース種別)
MODULES = [
	('SboGrpData.dll',   'SboGrpData/SboGrpData.rc',     'SboGrpData/resource.h',   None),
	('SboSoundData.dll', 'SboSoundData/SboSoundData.rc', 'SboSoundData/resource.h', None),
]

# ダイアログ・メニューを読む .rc（モジュール名, .rc, resource.h）
DIALOG_RCS = [
	('SboCli.exe', 'SboCli/SboCli.rc', 'SboCli/resource.h'),
]

# 実ファイルとして preload しないもの（今は無し。MIDI もページ側の JS シンセで鳴らすので preload する）
SKIP_EXT = ()

RES_RE = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*|\d+)\s+([A-Za-z_][A-Za-z0-9_]*)\s+(?:(?:MOVEABLE|PURE|DISCARDABLE|PRELOAD|LOADONCALL|FIXED|IMPURE)\s+)*"([^"]+)"')
STR_RE = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*|\d+)\s*,?\s*"((?:[^"]|"")*)"')
DEFINE_RE = re.compile(r'^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(\d+)')


def c_bytes(b):
	"""バイト列を C の文字列リテラルにする（8 進エスケープ）"""
	out = []
	for x in b:
		if 0x20 <= x < 0x7F and x not in (0x22, 0x5C, 0x3F):
			out.append(chr(x))
		else:
			out.append('\\%03o' % x)
	return '"' + ''.join(out) + '"'


def find_file(base, rel):
	"""Windows と同じく大文字小文字を区別せず、Unicode の正規化(NFC/NFD)の違いも無視してファイルを探す"""
	cur = base
	for part in rel.split('/'):
		if part in ('', '.'):
			continue
		key = unicodedata.normalize('NFC', part).lower()
		try:
			names = os.listdir(cur)
		except OSError:
			return None
		hit = [n for n in names if unicodedata.normalize('NFC', n).lower() == key]
		if not hit:
			return None
		cur = os.path.join(cur, hit[0])
	return cur


def load_defines(path):
	defs = {}
	with open(path, encoding='utf-8', errors='replace') as f:
		for line in f:
			m = DEFINE_RE.match(line)
			if m:
				defs[m.group(1)] = int(m.group(2))
	return defs


# ---------------------------------------------------------------------------
# ダイアログ・メニュー
# ---------------------------------------------------------------------------
# rc.exe がコントロールの種類ごとに付ける既定のクラスとスタイル
CTRL_DEFAULTS = {
	'LTEXT':           ('STATIC',    'SS_LEFT | WS_GROUP',               True),
	'RTEXT':           ('STATIC',    'SS_RIGHT | WS_GROUP',              True),
	'CTEXT':           ('STATIC',    'SS_CENTER | WS_GROUP',             True),
	'PUSHBUTTON':      ('BUTTON',    'BS_PUSHBUTTON | WS_TABSTOP',       True),
	'DEFPUSHBUTTON':   ('BUTTON',    'BS_DEFPUSHBUTTON | WS_TABSTOP',    True),
	'CHECKBOX':        ('BUTTON',    'BS_CHECKBOX | WS_TABSTOP',         True),
	'AUTOCHECKBOX':    ('BUTTON',    'BS_AUTOCHECKBOX | WS_TABSTOP',     True),
	'RADIOBUTTON':     ('BUTTON',    'BS_RADIOBUTTON',                   True),
	'AUTORADIOBUTTON': ('BUTTON',    'BS_AUTORADIOBUTTON',               True),
	'GROUPBOX':        ('BUTTON',    'BS_GROUPBOX',                      True),
	'EDITTEXT':        ('EDIT',      'ES_LEFT | WS_BORDER | WS_TABSTOP', False),
	'COMBOBOX':        ('COMBOBOX',  'CBS_SIMPLE | WS_TABSTOP',          False),
	'LISTBOX':         ('LISTBOX',   'LBS_NOTIFY | WS_BORDER',           False),
	'SCROLLBAR':       ('SCROLLBAR', 'SBS_HORZ',                         False),
}
DLG_KEYWORDS = set(CTRL_DEFAULTS) | {'CONTROL', 'ICON'}


def split_args(text):
	"""カンマで区切る（"..." の中のカンマは区切らない）"""
	args, cur, quote = [], '', False
	i = 0
	while i < len(text):
		c = text[i]
		if c == '"':
			if quote and i + 1 < len(text) and text[i + 1] == '"':
				cur += '""'
				i += 2
				continue
			quote = not quote
			cur += c
		elif c == ',' and not quote:
			args.append(cur.strip())
			cur = ''
		else:
			cur += c
		i += 1
	if cur.strip():
		args.append(cur.strip())
	return args


def rc_string(tok):
	return tok[1:-1].replace('""', '"') if tok.startswith('"') else tok


def rc_style(expr, defs):
	"""スタイル式（A | B | NOT C）を C の式にする。(足すもの, 外すもの) を返す"""
	add, rem = [], []
	if not expr:
		return add, rem
	toks = [t.strip() for t in expr.split('|')]
	for t in toks:
		if not t:
			continue
		neg = False
		if t.startswith('NOT '):
			neg = True
			t = t[4:].strip()
		if re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', t) or re.match(r'^(0x[0-9A-Fa-f]+|\d+)L?$', t):
			(rem if neg else add).append(t)
	return add, rem


def style_expr(base, add, rem):
	parts = [base] if base else []
	parts += add
	e = ' | '.join(parts) if parts else '0'
	if rem:
		e = '((%s) & ~(%s))' % (e, ' | '.join(rem))
	return '(DWORD)(%s)' % e


def rc_id(tok, defs):
	tok = tok.strip()
	if re.match(r'^-?\d+$', tok):
		return int(tok)
	if tok in ('IDC_STATIC',):
		return -1
	if tok in ('IDOK',):
		return 1
	if tok in ('IDCANCEL',):
		return 2
	return defs.get(tok, -1)


def parse_dialogs(rc_path, defs):
	"""DIALOG / DIALOGEX と MENU を読む"""
	with open(rc_path, encoding='utf-8', errors='replace') as f:
		lines = [l.rstrip('\r\n') for l in f]
	dialogs, menus = [], []
	i = 0
	head_re = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*|\d+)\s+(DIALOGEX|DIALOG|MENU|MENUEX)\b(.*)$')
	while i < len(lines):
		m = head_re.match(lines[i])
		if not m:
			i += 1
			continue
		name, kind, rest = m.group(1), m.group(2), m.group(3)
		i += 1
		if kind.startswith('MENU'):
			items, depth = [], 0
			while i < len(lines):
				s = lines[i].strip()
				i += 1
				if s == 'BEGIN' or s == '{':
					depth += 1
					continue
				if s == 'END' or s == '}':
					depth -= 1
					if depth == 0:
						break
					items.append((depth, 'END', 0, ''))
					continue
				if s.startswith('POPUP'):
					args = split_args(s[5:])
					items.append((depth, 'POPUP', 0, rc_string(args[0])))
				elif s.startswith('MENUITEM'):
					body = s[8:].strip()
					if body.startswith('SEPARATOR'):
						items.append((depth, 'SEP', 0, ''))
					else:
						args = split_args(body)
						items.append((depth, 'ITEM', rc_id(args[1], defs) if len(args) > 1 else 0, rc_string(args[0])))
			menus.append((name, defs.get(name, 0) if not name.isdigit() else int(name), items))
			continue
		# ダイアログの見出し
		nums = [x.strip() for x in rest.replace('DISCARDABLE', '').split(',')]
		nums = [int(x) for x in nums if re.match(r'^-?\d+$', x)]
		x, y, cx, cy = (nums + [0, 0, 0, 0])[:4]
		style, exstyle, caption, font = '', '', '', 9
		while i < len(lines) and lines[i].strip() not in ('BEGIN', '{'):
			s = lines[i].strip()
			i += 1
			while s.endswith('|') and i < len(lines):
				s += ' ' + lines[i].strip()
				i += 1
			if s.startswith('STYLE'):
				style = s[5:].strip()
			elif s.startswith('EXSTYLE'):
				exstyle = s[7:].strip()
			elif s.startswith('CAPTION'):
				caption = rc_string(s[7:].strip())
			elif s.startswith('FONT'):
				a = split_args(s[4:])
				font = int(a[0]) if a and a[0].isdigit() else 9
		i += 1
		# 本体（複数行にまたがる文はつなげる）
		stmts = []
		while i < len(lines):
			s = lines[i].strip()
			i += 1
			if s in ('END', '}'):
				break
			if not s:
				continue
			word = s.split()[0].rstrip(',')
			if word in DLG_KEYWORDS or not stmts:
				stmts.append(s)
			else:
				stmts[-1] += ' ' + s
		ctrls = []
		for st in stmts:
			word = st.split()[0]
			args = split_args(st[len(word):])
			if word == 'ICON':
				continue
			if word == 'CONTROL':
				# CONTROL text, id, class, style, x, y, cx, cy [, exstyle]
				if len(args) < 8:
					continue
				text, cid, cls = rc_string(args[0]), rc_id(args[1], defs), rc_string(args[2])
				add, rem = rc_style(args[3], defs)
				pos = [int(v) for v in args[4:8]]
				ex = rc_style(args[8], defs)[0] if len(args) > 8 else []
				ctrls.append((cls, text, cid, pos, style_expr('WS_CHILD | WS_VISIBLE', add, rem), style_expr('', ex, [])))
				continue
			cls, base, has_text = CTRL_DEFAULTS[word]
			if has_text:
				text, args = rc_string(args[0]), args[1:]
			else:
				text = ''
			if len(args) < 5:
				continue
			cid = rc_id(args[0], defs)
			pos = [int(v) for v in args[1:5]]
			add, rem = rc_style(args[5], defs) if len(args) > 5 else ([], [])
			ex = rc_style(args[6], defs)[0] if len(args) > 6 else []
			ctrls.append((cls, text, cid, pos, style_expr('WS_CHILD | WS_VISIBLE | ' + base, add, rem), style_expr('', ex, [])))
		sadd, srem = rc_style(style, defs)
		dialogs.append((name, defs.get(name, 0) if not name.isdigit() else int(name),
			(x, y, cx, cy), style_expr('', sadd, srem), style_expr('', rc_style(exstyle, defs)[0], []),
			caption, font, ctrls))
	return dialogs, menus


def write_dialogs(f, dialogs, menus):
	f.write('#include "win32compat.h"\n#include "commctrl.h"\n\n')
	for n, (name, did, rc, style, exstyle, caption, font, ctrls) in enumerate(dialogs):
		f.write('static const W32DlgItem s_aDlgItem%d[] = {\n' % n)
		for cls, text, cid, pos, cstyle, cex in ctrls:
			f.write('\t{%s, %s, %d, %d, %d, %d, %d, %s, %s},\n' % (
				c_bytes(cls.encode()), c_bytes(text.encode('cp932', errors='replace')), cid,
				pos[0], pos[1], pos[2], pos[3], cstyle, cex))
		f.write('\t{NULL, NULL, 0, 0, 0, 0, 0, 0, 0}\n};\n')
	f.write('\nconst W32DlgEntry g_aW32DlgEntry[] = {\n')
	for n, (name, did, rc, style, exstyle, caption, font, ctrls) in enumerate(dialogs):
		f.write('\t{%s, %d, %d, %d, %d, %d, %s, %s, %s, %d, s_aDlgItem%d},\n' % (
			c_bytes(name.encode()), did, rc[0], rc[1], rc[2], rc[3], style, exstyle,
			c_bytes(caption.encode('cp932', errors='replace')), font, n))
	f.write('\t{NULL, 0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL}\n};\n\n')
	kinds = {'POPUP': 'W32MENU_POPUP', 'ITEM': 'W32MENU_ITEM', 'SEP': 'W32MENU_SEPARATOR', 'END': 'W32MENU_END'}
	for n, (name, mid, items) in enumerate(menus):
		f.write('static const W32MenuTplItem s_aMenuItem%d[] = {\n' % n)
		for depth, kind, iid, text in items:
			f.write('\t{%s, %d, %s},\n' % (kinds[kind], iid, c_bytes(text.encode('cp932', errors='replace'))))
		f.write('\t{W32MENU_END, 0, NULL}\n};\n')
	f.write('\nconst W32MenuEntry g_aW32MenuEntry[] = {\n')
	for n, (name, mid, items) in enumerate(menus):
		f.write('\t{%s, %d, s_aMenuItem%d},\n' % (c_bytes(name.encode()), mid, n))
	f.write('\t{NULL, 0, NULL}\n};\n')


def main():
	root = os.path.abspath(sys.argv[1])
	out_dir = os.path.abspath(sys.argv[2])
	os.makedirs(out_dir, exist_ok=True)

	res_rows = []
	str_rows = []
	missing_midi = []
	preload = []
	for module, rc, header, _types in MODULES:
		defs = load_defines(os.path.join(root, header))
		rc_path = os.path.join(root, rc)
		rc_dir = os.path.dirname(rc_path)
		mod_base = os.path.splitext(module)[0]
		in_string = False
		with open(rc_path, encoding='utf-8', errors='replace') as f:
			for line in f:
				s = line.strip()
				if s.startswith('STRINGTABLE'):
					in_string = True
					continue
				if in_string:
					if s == 'END':
						in_string = False
						continue
					m = STR_RE.match(line)
					if m:
						key = m.group(1)
						sid = int(key) if key.isdigit() else defs.get(key)
						if sid is not None:
							text = m.group(2).replace('""', '"')
							str_rows.append((module, sid, text.encode('cp932', errors='replace')))
					continue
				m = RES_RE.match(line)
				if not m:
					continue
				name, rtype, fname = m.group(1), m.group(2).upper(), m.group(3)
				if rtype in ('TEXTINCLUDE', 'ICON', 'CURSOR', 'DIALOG', 'MENU', 'ACCELERATORS', 'VERSIONINFO'):
					continue
				rel = fname.replace('\\\\', '/').replace('\\', '/')
				src = find_file(rc_dir, rel)
				if src is None:
					# MIDI は再配布できないので公開リポジトリには無い。無ければ BGM 無しで作る（最後にまとめて知らせる）
					if rel.lower().startswith('midi/'):
						missing_midi.append(rel)
						continue
					print('[gen-resources] 警告: ファイルがありません: %s' % os.path.join(rc_dir, rel), file=sys.stderr)
					continue
				if name.isdigit():
					rid, rname = int(name), None
				elif name in defs:
					rid, rname = defs[name], None
				else:
					rid, rname = 0, name
				dest = '/res/%s/%s' % (mod_base, rel)
				skip = rel.lower().endswith(SKIP_EXT)
				res_rows.append((module, rtype, rid, rname, dest, skip))
				if not skip:
					preload.append('%s@%s' % (src, dest))

	dialogs, menus = [], []
	for module, rc, header in DIALOG_RCS:
		d, m = parse_dialogs(os.path.join(root, rc), load_defines(os.path.join(root, header)))
		dialogs += d
		menus += m

	with open(os.path.join(out_dir, 'w32_resources.cpp'), 'w', encoding='utf-8') as f:
		f.write('/* gen_resources.py が生成したファイル。手で編集しないこと */\n')
		f.write('#include "w32_resources.h"\n\n')
		f.write('const W32ResEntry g_aW32ResEntry[] = {\n')
		for module, rtype, rid, rname, dest, skip in res_rows:
			f.write('\t{%s, %s, %d, %s, %s, %d},\n' % (
				c_bytes(module.encode()), c_bytes(rtype.encode()), rid,
				c_bytes(rname.encode()) if rname else 'NULL',
				c_bytes(dest.encode('utf-8')), 1 if skip else 0))
		f.write('\t{NULL, NULL, 0, NULL, NULL, 0}\n};\n\n')
		f.write('const W32StrEntry g_aW32StrEntry[] = {\n')
		for module, sid, text in str_rows:
			f.write('\t{%s, %d, %s},\n' % (c_bytes(module.encode()), sid, c_bytes(text)))
		f.write('\t{NULL, 0, NULL}\n};\n\n')
		write_dialogs(f, dialogs, menus)

	# preload は同じディレクトリをまとめずにファイル単位で渡す（日本語名もそのまま通る）
	with open(os.path.join(out_dir, 'preload.txt'), 'w', encoding='utf-8') as f:
		for p in preload:
			f.write(p + '\n')

	print('[gen-resources] リソース %d 件、文字列 %d 件、ダイアログ %d 件、メニュー %d 件、preload %d ファイル' % (
		len(res_rows), len(str_rows), len(dialogs), len(menus), len(preload)))
	if missing_midi:
		print('[gen-resources] MIDI が %d 曲ありません（SboSoundData/MIDI）。その曲の BGM は鳴りません' % len(missing_midi))
	return 0


if __name__ == '__main__':
	sys.exit(main())
