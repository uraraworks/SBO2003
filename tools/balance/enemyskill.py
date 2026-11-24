"""敵の特技を battlesim の「特技の列」にする（simdata2tsv.py と report.py が使う）。
simdata.json の enemies[].skills（dexbuild/prep.py が本番の magic 表から作る 16 枠）を読む。

対応する効果（BattleSim.h / BattleCalc.h と同じ範囲）:
  種別 1 HP減少（魔法）/ 9 HP減少(HP消費・敵では 1 と同じ) / 6 HP減少(攻撃力) / 10 HP減少(率) / 2 HP増加（回復）
  4・7 置く魔法は、子の魔法が上のどれかなら「当たったら子の効果」として 1 回分で数える
  （残って何度も当たる型は 1 回分だけ＝過小評価。発動種別・射程は親のもの。親が「自分」で wData4 が 0 なら当たらない）
対応しない効果（毒・眠り・召喚・加速・攻撃/守備アップ・隠れる・蘇生・明かり・帰還など）は
  「行動だけ使って何も起きない」枠（種別 99）にして、無視した種類として呼び出し側に返す。
属性軽減・連携は入れていない。敵は Mana を使わない。
"""
SKILL_NOOP = 99
TYPE_NAME = {0: '効果なし', 1: 'HP減少', 2: 'HP増加', 3: '明かり', 4: '魔法発生', 5: '帰還', 6: 'HP減少(攻撃力)', 7: '魔法発生(自分)',
             8: '蘇生', 9: 'HP減少(HP消費)', 10: 'HP減少(率)', 11: '加速', 12: '毒', 13: '眠り', 14: '攻撃力アップ',
             15: '守備力アップ', 16: '隠れる'}
DIRECT = (1, 9, 6, 10, 2)


def flatten(m):
    """1 枠の magic 記録 → (種別, d1, d2, 発動種別, 使用対象, 射程, 無視した種類 or None, 注記 or None)"""
    t, tt, area = m['type'], m['tt'], m['area']
    if t in (4, 7):
        # 子をたどって効果のある魔法まで（置く魔法の子が置く魔法でも可）
        c, chain = m.get('child'), []
        while c and c['type'] in (4, 7) and len(chain) < 4:
            chain.append(c); c = c.get('child')
        if not c or c['type'] not in DIRECT:
            return (SKILL_NOOP, 0, 0, tt, 1, area, '置く魔法の子=' + TYPE_NAME.get(c['type'] if c else 0, '?'), None)
        # 飛ぶ魔法（自分の場所から前へ。wData4 が射程）は前方扱い。そうでなく「自分」なら足元に置くだけで当たらない
        if tt == 0 and m['d4']:
            tt = 1
        return (1 if c['type'] == 9 else c['type'], c['d1'], c['d2'], tt, c['tg'], area, None,
                '置く魔法を1回分で数えた')
    if t in DIRECT:
        return (1 if t == 9 else t, m['d1'], m['d2'], tt, m['tg'], area, None, None)
    return (SKILL_NOOP, 0, 0, tt, 1, area, TYPE_NAME.get(t, str(t)), None)


def skill_column(enemy):
    """(列の文字列, 無視した種類のリスト, 注記のリスト)。特技が無ければ '-'"""
    sk = enemy.get('skills') or []
    cells, ign, notes = [], [], []
    for m in sk:
        if not m:
            cells.append('')
            continue
        t, d1, d2, tt, tg, area, ig, note = flatten(m)
        cells.append('%d,%d,%d,%d,%d,%d' % (t, d1, d2, tt, tg, area))
        if ig:
            ign.append(ig)
        if note:
            notes.append(note)
    if not any(cells):
        return '-', ign, notes
    return ';'.join(cells), ign, notes
