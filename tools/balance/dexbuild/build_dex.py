# データ図鑑（装備・特技・敵・職業・能力アップ道具）とバランス健診の中身を作る
# 使い方: PROD=<fetch.py で落としたフォルダ> python3 build_dex.py <出力 dex.json>
import sys, os, statistics as stt
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from load import *

OUT = sys.argv[1]

# ---- 段階（マップ帳の「順路」と同じ分け方）
STAGES = [
    ('S1', 1, '段階1 ソラン', '始まりの大陸ソラン', [0, 1, 2, 10, 54, 38, 39, 18, 13, 21, 6]),
    ('S2', 2, '段階2 東フェルガナ', '古の大陸フェルガナ（東）', [14, 16, 45, 57, 11, 12, 80, 17, 15, 19, 20]),
    ('S3a', 3, '段階3 ガルムーン', '砂漠の大陸ガルムーン', [43, 44, 42, 46, 47, 48, 49, 51, 52, 77, 69, 50]),
    ('S3b', 3, '段階3 西フェルガナ', '古の大陸フェルガナ（西）', [33, 35, 34, 53, 86, 87]),
    ('S4', 4, '段階4 ノースフレア', '雪の大陸ノースフレア', [55, 56, 59, 61, 58, 60, 62, 63, 64, 65, 68, 100]),
    ('S5', 5, '段階5 ロウェル', '風と草原の大陸ロウェル', [75, 76, 74, 90]),
    ('S6', 6, '段階6 ロウェルの奥', '山・湿地・風の通り道・ジャングル', [78, 79, 103, 104, 105, 89, 91, 95, 97]),
    ('FES', 9, 'お祭り用', '魔王城・ラストダンジョンなど（昔のお祭り用）', [81, 82, 83, 108, 109, 110, 111, 112, 113, 102]),
]
TEST_MAPS = {37, 41, 66, 67, 92, 93, 7, 8, 71, 72, 73, 84, 94, 98, 99, 101, 106, 114, 115}
map_stage = {}
for key, order, label, title, ids in STAGES:
    for i in ids: map_stage[i] = (key, order)
STAGE_ORDER = {s[0]: s[1] for s in STAGES}
STAGE_LABEL = {s[0]: s[2] for s in STAGES}

def mshort(i):
    return M[i]['szName'].strip() or '%03d' % i

# ---- 入手経路（段階つき）
def item_routes(it):
    """[(種類, 文字列, 段階順)]"""
    r = []
    for m in sorted(shop_src.get(it, ())):
        st = map_stage.get(m)
        r.append(('店', mshort(m), st[1] if st else (99 if m in TEST_MAPS else 50), m in TEST_MAPS))
    for t, s in ST.items():
        if not enemy_maps.get(t) or not s['byDropProbability']: continue
        if it in s['waItem']:
            sts = [map_stage[m][1] for m in enemy_maps[t] if m in map_stage]
            r.append(('ドロップ', s['szName'], min(sts) if sts else 99, not sts))
    for nm in recipe_src.get(it, ()):
        r.append(('合成', nm, 50, False))
    for nm in quest_src.get(it, ()):
        r.append(('依頼', nm, 50, False))
    for m in sorted(give_src.get(it, ())):
        st = map_stage.get(m)
        r.append(('もらう', mshort(m), st[1] if st else 50, m in TEST_MAPS))
    if not r:
        for m in sorted(shop_noclerk.get(it, ())):
            st = map_stage.get(m)
            r.append(('店員のいない店', mshort(m), st[1] if st else 50, m in TEST_MAPS))
    return r

def earliest(routes, kinds=None):
    xs = [o for k, _, o, test in routes if not test and (kinds is None or k in kinds)]
    return min(xs) if xs else None

def route_text(routes):
    g = collections.OrderedDict()
    for k, n, o, test in routes:
        g.setdefault(k, []).append(n + ('（テスト用）' if test else ''))
    return [{'k': k, 'v': '、'.join(dict.fromkeys(v))} for k, v in g.items()]

JOBS = [(j['id'], j['szName']) for j in J]
JSHORT = {0: '旅', 1: '戦', 2: '武', 3: '僧', 4: '魔', 5: '商', 6: '召', 7: '吟'}

# ================= 装備 =================
SLOT = {3: '武器', 4: '鎧', 5: '盾', 6: '兜', 17: '小手', 18: '靴', 19: '首飾り', 20: '指輪', 21: '指輪'}
ATK = {0: '直接', 1: '飛び道具', 2: 'なぎ払い', 3: '貫通'}
SEX = {0: '', 1: '男性のみ', 2: '女性のみ'}
equips = []
for i in I.values():
    t = i['byType']
    if t not in SLOT: continue
    d = i['detail']
    jobs = EU.get(d.get('wEqu'), {}).get('jobIds', [])
    val = i['byData']
    eff = val - d.get('byMin', 0) / 2 if t == 3 else val
    kind = '守備力'
    if t == 3: kind = '攻撃力'
    elif t in (19, 20, 21): kind = {0: '効果なし', 1: '守備力', 2: '魔法防御'}.get(d.get('byEquType', 0), '?')
    rt = item_routes(i['id'])
    equips.append({'id': i['id'], 'name': i['szName'], 'slot': SLOT[t], 'type': t, 'val': val, 'eff': round(eff, 1),
                   'min': d.get('byMin', 0) if t == 3 else None, 'atk': ATK.get(d.get('byAtackType', 0)) if t == 3 else None,
                   'range': d.get('byData1', 0) if t == 3 and d.get('byAtackType') == 1 else None,
                   'kind': kind, 'gold': i['dwGold'], 'sex': SEX.get(i['bySexDistinction'], ''), 'jobs': jobs,
                   'routes': route_text(rt), 'stage': earliest(rt), 'shopStage': earliest(rt, {'店'}), 'flags': []})

def jobs_txt(jobs):
    return '全職業' if len(jobs) == 8 else '・'.join(JSHORT[j] for j in jobs)

# 段階ごと・職業ごと・部位ごとの「店で買える一番いい物」
def shop_best(job, slot, stage_order, exclude=None):
    best = None
    for e in equips:
        if e['slot'] != slot or job not in e['jobs'] or e['shopStage'] is None or e['shopStage'] > stage_order: continue
        if e['kind'] not in ('攻撃力', '守備力'): continue
        if e is exclude: continue
        if best is None or e['eff'] > best['eff']: best = e
    return best

for e in equips:
    if not e['routes']:
        e['flags'].append(('none', '入手手段なし', 'お店・ドロップ・合成・依頼のどこにも出てきません'))
    elif e['stage'] is None:
        e['flags'].append(('none', 'テスト用でだけ', 'テスト用マップでしか手に入りません'))
    elif e['stage'] >= 9:
        e['flags'].append(('info', 'お祭りでだけ', 'お祭り用マップの敵だけが落とします'))
    if e['sex']:
        e['flags'].append(('info', e['sex'], '性別で装備できない人がいます'))
    if e['kind'] not in ('攻撃力', '守備力') or e['stage'] is None: continue
    # 強すぎ: 手に入る段階の店の最強より大きく上
    st = min(e['stage'], 6)
    refs = [shop_best(j, e['slot'], st, exclude=e) for j in e['jobs']]
    refs = [r for r in refs if r]
    if refs and e['shopStage'] != e['stage']:
        ref = max(refs, key=lambda r: r['eff'])
        if e['eff'] >= ref['eff'] * 1.4 and e['eff'] - ref['eff'] >= 4:
            e['flags'].append(('strong', '飛び抜けて強い', '%sで手に入るのに、そこまでの店の一番（%s %s）の約%.1f倍' %
                               ('段階%d' % st, ref['name'], ref['val'], e['eff'] / max(ref['eff'], 1))))
    # 上位互換（店売りどうし）: 同じ部位で、同じか広い職業・同じか前の段階・安い・強い物がある
    if e['shopStage'] is not None:
        for o in equips:
            if o is e or o['slot'] != e['slot'] or o['shopStage'] is None or o['kind'] != e['kind']: continue
            if o['atk'] != e['atk']: continue
            if set(e['jobs']) <= set(o['jobs']) and o['shopStage'] <= e['shopStage'] and o['gold'] < e['gold'] and o['eff'] > e['eff'] and (o['sex'] == '' or o['sex'] == e['sex']):
                e['flags'].append(('weak', '買う意味が薄い', '%s（%s・%dG）の方が安くて強い' % (o['name'], o['val'], o['gold'])))
                break

# 職業ごとの武器の選択肢の少なさ
weapon_count = {j: sum(1 for e in equips if e['slot'] == '武器' and j in e['jobs'] and e['shopStage'] is not None) for j, _ in JOBS}

# ================= 特技 =================
TT = {0: '自分', 1: '前方', 2: '周囲', 3: '画面内', 4: '前方横一列', 5: '選択', 6: '選択(自分以外)'}
ATTR = {0: '', 1: '火', 2: '水', 3: '風', 4: '土', 5: '光', 6: '闇'}
book = collections.defaultdict(list)
for i in I.values():
    if i['byType'] == 14: book[i['detail'].get('byID')].append(i)
child_of = set()
for m in MG.values():
    if m['byType'] in (4, 7) and m['wData1'] in MG: child_of.add(m['wData1'])

def rng(a, b):
    a, b = min(a, b), max(a, b)
    return str(a) if a == b else '%d〜%d' % (a, b)

def effect(m, depth=0):
    """(説明文, 平均ダメージ or 回復量, 攻撃力に足す形か)"""
    t = m['byType']; at = ATTR.get(m['byAttr'], '')
    at = '（%s）' % at if at else ''
    if t in (1, 9):
        return '%sの魔法ダメージ%s' % (rng(m['wData1'], m['wData2']), at), (m['wData1'] + m['wData2']) / 2, False
    if t == 6:
        return '攻撃力＋%sのダメージ%s' % (rng(m['wData1'], m['wData2']), at), (m['wData1'] + m['wData2']) / 2, True
    if t == 10:
        return '残りHPの%d％のダメージ' % m['wData1'], None, False
    if t == 2:
        return 'HPを%s回復' % rng(m['wData1'], m['wData2']), (m['wData1'] + m['wData2']) / 2, False
    if t == 3: return '暗い所を明るくする', None, False
    if t == 5: return '記録した町へ戻る', None, False
    if t == 8: return '倒れた仲間を生き返らせる（HP%d％で）' % m['wData1'], None, False
    if t == 11: return '%d秒、攻撃の間隔が%d％短くなる' % (m['wData2'], m['wData1']), None, False
    if t == 12: return '毒（%d秒、2秒ごとに%d）' % (m['wData2'], m['wData1']), None, False
    if t == 13: return '眠らせる（最大%d秒）' % m['wData1'], None, False
    if t in (14, 15): return '%d秒、%sが%d％上がる' % (m['wData2'], '攻撃力' if t == 14 else '守備力', m['wData1']), None, False
    if t in (4, 7) and depth < 4 and m['wData1'] in MG:
        c = MG[m['wData1']]
        s, v, a = effect(c, depth + 1)
        head = 'その場に出す' if t == 7 else ('前へ飛ばす' if m['wData4'] else '出す')
        return '%s→当たると%s' % (head, s), v, a
    return '（効果なし）', None, False

ENEMY_ONLY_MANA = 34463
magics = []
enemy_use = collections.defaultdict(set)
for t, s in ST.items():
    if enemy_maps.get(t):
        for x in s['abyMagic']:
            if x: enemy_use[x].add(s['szName'])
for m in MG.values():
    if m['id'] == 0: continue
    bk = book.get(m['id'], [])
    jobs = MU.get(m['byUserInfo'], {}).get('jobIds', [])
    txt, val, isat = effect(m)
    routes = []
    for b in bk:
        rt = item_routes(b['id'])
        routes.append({'book': b['szName'], 'gold': b['dwGold'], 'routes': route_text(rt), 'stage': earliest(rt)})
    kind = 'player'
    if m['wMana'] == ENEMY_ONLY_MANA: kind = 'enemy'
    elif m['id'] in child_of: kind = 'child'
    elif not bk and ('テスト' in m['szName'] or 'お祝い' in m['szName']): kind = 'test'
    elif not bk and enemy_use.get(m['id']): kind = 'enemy'
    elif not bk and m['byUserInfo'] == 0: kind = 'child'
    st = min([r['stage'] for r in routes if r['stage'] is not None], default=None)
    rec = {'id': m['id'], 'name': m['szName'], 'jobs': jobs, 'jobName': MU.get(m['byUserInfo'], {}).get('szName', ''),
           'mana': m['wMana'] if m['wMana'] != ENEMY_ONLY_MANA else None, 'time': m['byTime'],
           'target': TT.get(m['byTargetType'], '?'), 'all': bool(m['byTargetCount']), 'attr': ATTR.get(m['byAttr'], ''),
           'effect': txt, 'avg': val, 'isAT': isat, 'books': routes, 'stage': st, 'kind': kind,
           'enemies': sorted(enemy_use.get(m['id'], [])), 'explain': m['szExplain'], 'flags': [],
           'dmgType': m['byType']}
    if val and rec['mana']:
        rec['perMana'] = round(val / rec['mana'], 1)
    magics.append(rec)

pm = [x for x in magics if x['kind'] == 'player']
for x in pm:
    if not x['books']:
        x['flags'].append(('none', '書がない', '覚える方法（特技の書）がありません'))
    elif x['stage'] is None:
        x['flags'].append(('none', '書の入手手段なし', '書がお店にもドロップにもありません'))
    # 攻撃特技の上位互換（同じ職業・同じ種類・単体/全体が同じ）
    if x['avg'] and x['dmgType'] in (1, 4, 6, 7) and x['books']:
        g = min(b['gold'] for b in x['books'])
        for o in pm:
            if o is x or not o['avg'] or not o['books'] or o['isAT'] != x['isAT'] or o['all'] != x['all'] or set(o['jobs']) != set(x['jobs']): continue
            if 'HPを' in o['effect'] or 'HPを' in x['effect']: continue
            og = min(b['gold'] for b in o['books'])
            if o['avg'] > x['avg'] and (o['mana'] or 0) <= (x['mana'] or 0) and og <= g:
                x['flags'].append(('weak', '上位互換あり', '%s（%s・Mana%d・書%dG）の方が強くて軽い' % (o['name'], o['effect'].split('→')[-1], o['mana'], og)))
                break
# 手書きのメモ（数字を見た気づき）
HAND_MAGIC = {
    22: ('strong', '1発が重い', '1体150〜180でMana10。段階4の町で買え、同じ頃の雑魚（HP30〜100）はほぼ一撃。魔刃剣より強くて安い'),
    33: ('weak', '威力がほぼ無い', '攻撃力＋0〜2。横一列に当たるが、ふつうに殴るのとほとんど変わらずMana5を使う'),
    29: ('info', '序盤の主力', '攻撃力＋10〜30でMana5。始まりの町で500G'),
    4: ('info', '旅人の明かり', '暗いマップで使う'),
    36: ('weak', '1回が小さい?', '当たると3〜6（光）。書40000G・Mana10なのに、同じ頃の聖光輪は80〜90。残る間に何度も当たるならその分増える（要確認）'),
    76: ('weak', '重いわりに不安定', 'Mana20で1〜50。書40000G'),
    41: ('info', '続けて当たる型', 'その場に残って2〜4ずつ当たる。数字は1回分'),
}
for x in magics:
    if x['id'] in HAND_MAGIC: x['flags'].append(HAND_MAGIC[x['id']])
buff_none = not any(m['byType'] in (11, 14, 15) for m in MG.values())

# ================= 敵 =================
NT = {11: '反撃型', 12: '追いかける', 13: '追いかけて攻撃', 24: '宝箱の敵', 15: '爆弾', 19: '爆弾ゲーム'}
enemies = []
for t, s in ST.items():
    mp = sorted(enemy_maps.get(t, ()))
    sts = sorted({map_stage[m][0] for m in mp if m in map_stage}, key=lambda k: STAGE_ORDER[k])
    test = bool(mp) and all(m in TEST_MAPS for m in mp)
    rec = {'id': t, 'name': s['szName'], 'maps': [{'id': m, 'name': mshort(m)} for m in mp], 'stages': sts,
           'stage': min([STAGE_ORDER[k] for k in sts], default=None), 'count': enemy_count.get(t, 0),
           'hp': s['wMaxHP'], 'mp': s['wMaxMana'], 'pw': s['byPW'], 'dex': s['byDEX'], 'st': s['byST'], 'it': s['byIT'],
           'df': s['byST'] // 2, 'dfm': s['wDFMagic'], 'exp': s['dwExp'], 'gold': s['dwGold'], 'drop': s['byDropProbability'],
           'items': [{'id': x, 'name': I[x]['szName']} for x in s['waItem'] if x],
           'magic': [MG[x]['szName'] for x in s['abyMagic'] if x], 'type': NT.get(s['byNpcType'], str(s['byNpcType'])),
           'resist': s['byStatusResist'], 'attr': {k: s[f] for k, f in [('火', 'chAttrFire'), ('水', 'chAttrWater'), ('風', 'chAttrWind'), ('土', 'chAttrEarth'), ('光', 'chAttrLight'), ('闇', 'chAttrDark')] if s[f]},
           'flags': [], 'kind': 'normal'}
    if not mp: rec['kind'] = 'unused'
    elif test: rec['kind'] = 'test'
    elif rec['exp'] == 0: rec['kind'] = 'test'
    elif rec['hp'] >= 900 or rec['exp'] >= 400: rec['kind'] = 'boss'
    enemies.append(rec)

# 段階ごとの雑魚の中央値と比べる
def zako(stage_key):
    return [e for e in enemies if e['kind'] == 'normal' and stage_key in e['stages']]
for e in enemies:
    if e['kind'] == 'unused':
        e['flags'].append(('none', 'どこにも出ない', '発生地点が無いテンプレートです')); continue
    if e['kind'] == 'test': continue
    if e['hp']:
        ratio = e['exp'] / e['hp']
        e['ratio'] = round(ratio, 2)
    for sk in e['stages'][:1]:
        z = [x for x in zako(sk) if x is not e and x['hp']]
        if len(z) < 2: continue
        med = stt.median(x['exp'] / x['hp'] for x in z)
        if e['kind'] == 'normal' and ratio >= med * 2.5:
            e['flags'].append(('strong', '割のいい獲物', 'HPあたりの経験値が%sの雑魚の約%.1f倍' % (STAGE_LABEL[sk], ratio / med)))
        if e['kind'] == 'normal' and ratio <= med * 0.3:
            e['flags'].append(('weak', '割に合わない', 'HPあたりの経験値が%sの雑魚の約%.1f倍しかない' % (STAGE_LABEL[sk], ratio / med)))
    if e['pw'] == 0 and e['hp']:
        e['flags'].append(('info', '殴ってこない', '力0（特技だけで戦う）'))
    if e['dfm'] >= 100:
        e['flags'].append(('info', '魔法が効かない', '魔法防御%d' % e['dfm']))
# 強い装備を落とす敵
strong_eq = {x['id']: x for x in equips if any(f[0] == 'strong' for f in x['flags'])}
for e in enemies:
    for it in e['items']:
        if it['id'] in strong_eq and e['drop']:
            e['flags'].append(('strong', '強い装備を落とす', '%s（%d）を%d%%の当たりから落とす' % (it['name'], strong_eq[it['id']]['val'], e['drop'])))
HAND_ENEMY = {
    68: ('strong', 'HPが低すぎ?', 'HP150なのに経験値1000（周りのラストダンジョンの敵はHP1000〜7000）。入力ミスの疑い'),
    11: ('info', '序盤なのにHP200', 'ぬめぬめ（もぐら達の隠れ家）はHP200・素早さ20で、同じマップの洞窟もぐら（HP40）の5倍。経験値は11'),
    44: ('info', '逃げ回る宝物役?', '素早さ・体力200、魔法防御255。HP200で経験値100・ドロップ20%'),
    5: ('info', '序盤のボス', 'プヨンの巣の主。HP300・経験値100・ドロップ23%で種を色々落とす'),
}
for e in enemies:
    if e['id'] in HAND_ENEMY: e['flags'].append(HAND_ENEMY[e['id']])

# ================= 職業 =================
STATS = [('byPW', '力'), ('byDEX', '素早さ'), ('byST', '体力'), ('byIT', '賢さ'), ('byLK', '運'), ('wMAXHP', 'HP'), ('wMAXMANA', 'Mana')]
jobs = []
for j in J:
    lv = j['levelUp']; flags = []
    rows = []
    for n, l in enumerate(lv, 1):
        rows.append({'lv': n, 'exp': l['dwExp'], **{k: l[k] for k, _ in STATS}})
    for n in range(1, len(lv)):
        p, c = lv[n - 1], lv[n]
        for k, nm in STATS:
            if c[k] < p[k]:
                flags.append(('typo', 'Lv%dの%sが下がる' % (n + 1, nm), 'Lv%d %d → Lv%d %d（入力ミスの疑い）' % (n, p[k], n + 1, c[k])))
        if n >= 1 and c['dwExp'] <= p['dwExp'] and n > 0:
            flags.append(('typo', 'Lv%dの経験値が前と同じ' % (n + 1), 'Lv%d・Lv%dとも %d' % (n, n + 1, c['dwExp'])))
        if n >= 2 and p['dwExp'] > 0 and c['dwExp'] / p['dwExp'] >= 3.5:
            flags.append(('info', 'Lv%dへの経験値が急に増える' % (n + 1), '%d → %d（%.1f倍。ほかの所は約2倍）' % (p['dwExp'], c['dwExp'], c['dwExp'] / p['dwExp'])))
    jobs.append({'id': j['id'], 'name': j['szName'], 'rows': rows, 'flags': flags, 'weapons': weapon_count[j['id']]})

# ================= 能力アップ道具 =================
SEED = {0: '力', 1: '素早さ', 2: '体力（スタミナ）', 3: '賢さ', 4: '運', 5: 'HP（生命）', 6: 'Mana（魔力）', 7: '力', 8: '素早さ', 9: '体力', 10: '賢さ', 11: '運', 12: 'HP', 13: 'Mana'}
SEED_KEY = {0: 'byPW', 1: 'byDEX', 2: 'byST', 3: 'byIT', 4: 'byLK', 5: 'wMAXHP', 6: 'wMAXMANA'}
seeds = []
for i in I.values():
    if i['byType'] != 7: continue
    d = i['detail']; bt = d.get('byType', 0)
    rt = item_routes(i['id'])
    rec = {'id': i['id'], 'name': i['szName'], 'stat': SEED.get(bt, '?'), 'down': bt >= 7, 'max': d.get('wMax', 0),
           'gold': i['dwGold'], 'routes': route_text(rt), 'stage': earliest(rt), 'flags': []}
    if not rt: rec['flags'].append(('none', '入手手段なし', ''))
    if bt in SEED_KEY and not rec['down']:
        caps = {jn: J[jid]['levelUp'][-1][SEED_KEY[bt]] for jid, jn in JOBS}
        over = [jn for jn, c in caps.items() if c < rec['max']]
        under = [jn for jn, c in caps.items() if c >= rec['max']]
        rec['caps'] = caps
        if over and rec['max'] >= 2 * min(caps.values()) and bt in (5, 6):
            weakest = min(caps, key=caps.get)
            rec['flags'].append(('strong', '低い職業を大きく押し上げる', '%sのLv15上限%dに対して%dまで上がる' % (weakest, caps[weakest], rec['max'])))
        rec['useful15'] = over
        # 何レベルから全職業で効かなくなるか
        for n in range(15):
            if all(J[jid]['levelUp'][n][SEED_KEY[bt]] >= rec['max'] for jid, _ in JOBS):
                rec['deadLv'] = n + 1; break
    seeds.append(rec)
seeds.sort(key=lambda x: (x['down'], x['stat'], x['max']))
stat_tiers = collections.defaultdict(list)
for s in seeds:
    if not s['down']: stat_tiers[s['stat']].append(s['max'])

# ================= バランス健診 =================
REF_JOB = 0  # 目安レベルは旅人の表で出す
def lv_for(exp_med, job=REF_JOB, kills=40):
    lu = J[job]['levelUp']
    for n in range(len(lu) - 1):
        gap = lu[n + 1]['dwExp'] - lu[n]['dwExp']
        if gap > kills * exp_med: return n + 1
    return len(lu)

def dmg(at, df):
    return max(0, at - df // 2)

checks = []
for key, order, label, title, ids in STAGES:
    z = zako(key)
    if not z: continue
    bosses = [e for e in enemies if e['kind'] == 'boss' and key in e['stages']]
    exp_med = stt.median(e['exp'] for e in z)
    hp_med = stt.median(e['hp'] for e in z)
    st_med = stt.median(e['st'] for e in z)
    pw_max = max(e['pw'] for e in z)
    gold_med = stt.median(e['gold'] for e in z)
    lv = lv_for(exp_med)
    so = min(order, 6)
    per = []
    for jid, jn in JOBS:
        cap = J[jid]['levelUp'][lv - 1]
        w = shop_best(jid, '武器', so)
        armor = sum((shop_best(jid, s, so) or {'val': 0})['val'] for s in ('鎧', '盾', '兜', '靴'))
        at = cap['byPW'] + (w['val'] - w['min'] / 2 if w else 0)
        df = cap['byST'] // 2 + armor
        d_out = dmg(int(at), int(st_med) // 2)
        d_in = dmg(pw_max, df)
        # その段階までに書が買える攻撃特技で一番強いもの（1回分）
        bm = None
        for x in magics:
            if x['kind'] != 'player' or jid not in x['jobs'] or not x['avg'] or 'HPを' in x['effect']: continue
            bst = min([b['stage'] for b in x['books'] if b['stage'] is not None], default=None)
            if bst is None or bst > so or (x['mana'] or 0) > cap['wMAXMANA']: continue
            v = x['avg'] + (at if x['isAT'] else 0) - (int(st_med) // 4 if x['isAT'] else 0)
            if bm is None or v > bm[1]: bm = (x['name'], v, x['mana'])
        per.append({'job': jn, 'lv': lv, 'weapon': w['name'] if w else 'なし', 'at': round(at), 'df': df, 'hp': cap['wMAXHP'], 'mana': cap['wMAXMANA'],
                    'magic': bm[0] if bm else None, 'magicHits': round(hp_med / bm[1], 1) if bm and bm[1] > 0 else None, 'magicCasts': (cap['wMAXMANA'] // bm[2]) if bm and bm[2] else None,
                    'hitsToKill': (round(hp_med / d_out, 1) if d_out else None), 'hitsToDie': (round(cap['wMAXHP'] / d_in, 1) if d_in else None)})
    # 新しく買える武器の値段 ÷ 雑魚のお金
    new_w = [e for e in equips if e['slot'] == '武器' and e['shopStage'] == order]
    price = stt.median([e['gold'] for e in new_w]) if new_w else None
    checks.append({'key': key, 'label': label, 'title': title, 'lv': lv, 'expMed': exp_med, 'expRange': [min(e['exp'] for e in z), max(e['exp'] for e in z)],
                   'hpMed': hp_med, 'pwMax': pw_max, 'goldMed': gold_med, 'killsPerLv': round((J[0]['levelUp'][lv]['dwExp'] - J[0]['levelUp'][lv - 1]['dwExp']) / exp_med) if lv < 15 else None,
                   'zako': [e['name'] for e in z], 'boss': [{'name': b['name'], 'hp': b['hp'], 'exp': b['exp']} for b in bosses],
                   'jobs': per, 'newWeaponPrice': price, 'killsForWeapon': round(price / gold_med) if price and gold_med else None})

data = {'at': os.environ.get('SBO_AT', ''), 'jobs': [{'id': j, 'name': n, 'short': JSHORT[j]} for j, n in JOBS],
        'stages': [{'key': s[0], 'order': s[1], 'label': s[2], 'title': s[3]} for s in STAGES],
        'equips': sorted(equips, key=lambda x: (list(SLOT.values()).index(x['slot']), x['val'], x['gold'])),
        'magics': sorted(magics, key=lambda x: (x['kind'] != 'player', x['jobs'][:1] if len(x['jobs']) < 8 else [9], x['stage'] or 99, x['mana'] or 0)),
        'enemies': sorted(enemies, key=lambda x: (x['kind'] in ('test', 'unused'), x['stage'] or 99, x['exp'])),
        'jobTables': jobs, 'seeds': seeds, 'statTiers': stat_tiers, 'checks': checks, 'buffNone': buff_none,
        'weaponCount': {JSHORT[j]: c for j, c in weapon_count.items()}}
json.dump(data, open(OUT, 'w'), ensure_ascii=False, indent=1)
print('equips', len(equips), 'magics', len(magics), 'enemies', len(enemies), 'checks', len(checks))
