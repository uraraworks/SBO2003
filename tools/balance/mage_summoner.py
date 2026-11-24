#!/usr/bin/env python3
"""魔法使い・召喚士が目標1（雑魚1体を1人で勝率90%以上）を満たさない原因の切り分けと、直す案の比較表を作る
（Markdown を標準出力へ）。データは書き換えない（simdata.json のコピーをメモリ上で変えて battlesim を回すだけ）。

  python3 tools/balance/mage_summoner.py [--simdata simdata.json] [--runs 1000] [--seed 1] [--exe /tmp/battlesim] > 表.md

early_boss.py / north_flare.py の作り方（World / Batch / scen_row）の続き。見る観点:
  (1) 他職（旅人・戦士・僧侶）との要因の比較（HP・守備・攻撃・Mana・特技・装備できる物）と、要因を 1 つずつ他職に入れ替えた時の勝率
  (2) シミュレータで再現できていない役割（遠隔の先撃ち・召喚の複数回ヒット・敵の魔法防御）を入れた時の勝率
  (3) 案（職業表の底上げ・特技の入手前倒し・装備の見直し・何もしない＝1人ソロを想定しない）の勝率・他職との差・強すぎないか
遠隔の先撃ちは common/BattleSim.h の Player.bAtkIsAT の bit1 以上（射程）で有効にする（0・1 なら従来どおり）。
"""
import argparse, copy, json, os, random, re, sys
from statistics import median, mean

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import report as R
import early_boss as EB

WORK = '/mnt/project-files/_work/sbo2003-balance-sim'
JOBS, SHORT, JID = EB.JOBS, EB.SHORT, EB.JID
MS = ['魔法使い', '召喚士']
REF = ['旅人', '戦士', '僧侶']
STAGES = [('S1', 6), ('S2', 8), ('S3a', 9), ('S3b', 10), ('S4', 10), ('S5', 10), ('S6', 12)]
SLABEL = {'S1': '段階1 ソラン(Lv6)', 'S2': '段階2 東フェルガナ(Lv8)', 'S3a': '段階3 ガルムーン(Lv9)', 'S3b': '段階3 西フェルガナ(Lv10)',
          'S4': '段階4 ノースフレア(Lv10)', 'S5': '段階5 ロウェル(Lv10)', 'S6': '段階6 ロウェルの奥(Lv12)'}
SLIM = ['S1', 'S2', 'S3a', 'S3b', 'S4', 'S6']      # 表に出す段階（段階5 は全職合格なので省く）
ZEN = str.maketrans('０１２３４５６７８９', '0123456789')


# ---------------------------------------------------------------- 標準プレイヤー（World.player の拡張版）
def best_equip(d, jid, slot, so, any_job=False):
    c = [e for e in d['equips'] if e['slot'] == slot and ((any_job and (0 in e['jobs'] or 1 in e['jobs'])) or (not any_job and jid in e['jobs'])) and e.get('shopStage') is not None
         and e['shopStage'] <= so and e['kind'] == ('攻撃力' if slot == '武器' else '守備力') and (slot != '武器' or e['atk'] == '直接')]
    return sorted(c, key=lambda e: -e['eff'])[0] if c else None


def magic_range(m):
    """特技名の「前３マス」「選択２マス」から射程（マス）。「周囲１マス」「前１マス」は 1（目の前）"""
    r = re.search(r'(?:前|選択|周囲)(\d)マス', m['name'].translate(ZEN))
    return int(r.group(1)) if r else 1


BUDGET = [None]     # 特技の書を「その段階の雑魚 N 体分のゴールド以下」だけに絞る（None=値段を無視。既定）


def player(w, job, lv, sk, book_so=None, gear_any=False, gear_so=None):
    """World.player と同じ標準プレイヤー＋内訳（装備名・特技名など）。book_so: 特技の書を使える段階の順（既定=段階の順）。
    gear_any: 装備の職業制限を外す（旅人か戦士が装備できる物の中の最強。武道家専用品は含めない）。gear_so: 装備の店の段階の順。"""
    d = w.d
    so = min(w.stage[sk]['order'], 6)
    bso = so if book_so is None else book_so
    gso = so if gear_so is None else gear_so
    jt = next(j for j in d['jobTables'] if j['name'] == job)
    row = jt['rows'][lv - 1]
    jid = JID[job]
    wp = best_equip(d, jid, '武器', gso, gear_any)
    armor = {s: best_equip(d, jid, s, gso, gear_any) for s in ['鎧', '盾', '兜', '靴']}
    hp = w.hp_at(job, lv, row['wMAXHP'])
    mana = row['wMAXMANA']

    gold_budget = None if BUDGET[0] is None else BUDGET[0] * (w.chk[sk].get('goldMed') or 1)

    def ok(m):
        return (m['kind'] == 'player' and jid in m['jobs'] and m.get('avg') and m['mana'] <= mana
                and any(b['stage'] is not None and b['stage'] <= bso and (gold_budget is None or b['gold'] <= gold_budget) for b in m['books']))
    heals = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] == 2], key=lambda m: m['mana'])
    atks = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] != 2 and 'HPを' not in m['effect']], key=lambda m: -m['avg'])
    h = heals[0] if heals else None
    a = atks[0] if atks else None
    arm = {s: (x['val'] if x else 0) for s, x in armor.items()}
    p = dict(lv=lv, hp=hp, mana=mana, at=row['byPW'] + (wp['val'] if wp else 0),
             df=row['byST'] // 2 + sum(arm.values()), dex=row['byDEX'], wmin=wp['min'] if wp else 0,
             hm=h['mana'] if h else 0, ha=int(h['avg']) if h else 0, ht=h['time'] if h else 0,
             am=a['mana'] if a else 0, aa=int(a['avg']) if a else 0, at_=a['time'] if a else 0, aisat=1 if a and a['isAT'] else 0)
    p.update(job=job, pw=row['byPW'], st=row['byST'], weapon=wp['name'] if wp else '-', wval=wp['val'] if wp else 0,
             armor={s: (x['name'], x['val']) if x else ('-', 0) for s, x in armor.items()},
             spell=a['name'].split('（')[0] if a else '-', spell_full=a['name'] if a else '-', spell_dt=a['dmgType'] if a else 0,
             spell_range=magic_range(a) if a else 0, spell_book=a['books'][0] if a else None, heal=h['name'].split('（')[0] if h else '-')
    return p


def zako_list(w, sk, cfg):
    skip = set(cfg.get('boss_override', [])) | set(cfg.get('treasure_override', []))
    return [e for e in w.enemies(sk, 'zako') if e['name'] not in skip]


# ---------------------------------------------------------------- 行の作り（敵の魔法防御・遠隔・召喚の複数回ヒットの補正つき）
_EXP = {}


def exp_normal(at, df):
    """通常攻撃 1 回の期待ダメージ（空振り5%込み。武器の減少値・会心は無視）"""
    k = (at, df)
    if k not in _EXP:
        rng = random.Random(7)
        _EXP[k] = mean(R.py_damage(at, df, rng) for _ in range(400)) * .95
    return _EXP[k]


def adj(p, e, dfm=False, ranged=False, summon=1.0, smart=False):
    """dfm: 敵の魔法防御を攻撃特技（攻撃力＋値型でないもの）に反映（1発ごとに dfm/2 を引く）。
    summon: 召喚（置く魔法 dmgType 7）の 1 回の使用で当たる回数（係数）。dfm を引いた後の 1 発に掛ける。
    ranged: 攻撃特技の射程（前３マスなど）を先撃ちに使う（BattleSim.h の bAtkIsAT bit1 以上）。魔法使い・召喚士にだけ付ける（他職にも付けると、敵の回復との兼ね合いで 0.5 秒の行動のずれだけで勝率が大きく動く例があり、比べにくいため）。
    smart: 特技の 1 発が通常攻撃の期待ダメージより小さい敵には特技を使わない（battlesim は「Manaがあれば特技」固定のため）"""
    p = dict(p)
    if p['am'] and not p['aisat']:
        aa = p['aa']
        if dfm and e.get('dfm'):
            aa -= e['dfm'] // 2
        if p.get('spell_dt') == 7 and summon != 1.0:
            aa *= summon
        if aa <= 0 or (smart and aa < exp_normal(p['at'], e['st'] // 2)):
            p['am'] = p['aa'] = p['at_'] = 0
        else:
            p['aa'] = aa
    if ranged and p['am'] and p.get('spell_range', 1) > 1 and p.get('job') in MS:
        p['aisat'] = p['aisat'] + 2 * p['spell_range']
    return p


def row(p, e, n=1, allies=None, nobuff=False, **kw):
    return R.scen_row(adj(p, e, **kw), e, n, nobuff=nobuff,
                      allies=[adj(a, e, **kw) for a in allies] if allies else None)


class Rows:
    """(職業, 段階, 条件) ごとの雑魚 1 体の勝率を、後でまとめて引く"""

    def __init__(self, d, w, batch, cfg):
        self.d, self.w, self.b, self.cfg = d, w, batch, cfg

    def zako(self, p, sk, n=1, **kw):
        return [(e, self.b.add(row(p, e, n, **kw))) for e in zako_list(self.w, sk, self.cfg)]

    def stat(self, reqs, th=.9):
        ws = [self.b[r]['win'] for _, r in reqs]
        return dict(mean=mean(ws), min=min(ws), bad=sum(1 for x in ws if x < th), n=len(ws), worst=min(reqs, key=lambda x: self.b[x[1]]['win'])[0]['name'])


def pc(x):
    return '%.0f' % (x * 100)


def cell(s, th=.9):
    t = '%s／%s' % (pc(s['mean']), pc(s['min']))
    return '**%s**' % t if s['min'] < th else t


def apply(d0, *fns):
    d = copy.deepcopy(d0)
    for f in fns:
        if f:
            f(d)
    return d


# ---------------------------------------------------------------- 案（simdata のコピーを書き換える関数）
def hp_pct(pct, jobs=MS, lvs=range(1, 16)):
    """職業のLv別最大HPを 旅人・戦士・僧侶の中央値 × pct にする（今の値より下げない）"""
    def f(d):
        tabs = {t['name']: t['rows'] for t in d['jobTables']}
        for j in jobs:
            for lv in lvs:
                ref = median(tabs[r][lv - 1]['wMAXHP'] for r in REF)
                r = tabs[j][lv - 1]
                r['wMAXHP'] = max(r['wMAXHP'], int(round(ref * pct)))
    return f


def job_add(jobs, pw=0, st=0, mana=1.0, lvs=range(1, 16)):
    def f(d):
        for t in d['jobTables']:
            if t['name'] in jobs:
                for lv in lvs:
                    r = t['rows'][lv - 1]
                    r['byPW'] += pw; r['byST'] += st; r['wMAXMANA'] = int(round(r['wMAXMANA'] * mana))
    return f


def book_stage(magic_id, stage):
    def f(d):
        for m in d['magics']:
            if m['id'] == magic_id:
                for b in m['books']:
                    b['stage'] = stage
    return f


def book_price(prices):
    """特技の書の値段（G）を {特技ID: 値段} で置き換える"""
    def f(d):
        for m in d['magics']:
            if m['id'] in prices:
                for b in m['books']:
                    b['gold'] = prices[m['id']]
    return f


def spell_dmg(magic_id, mult=1.0, add=0.0):
    def f(d):
        for m in d['magics']:
            if m['id'] == magic_id:
                m['avg'] = m['avg'] * mult + add
    return f


def equip_jobs(names, jobs):
    def f(d):
        for e in d['equips']:
            if e['name'] in names:
                e['jobs'] = sorted(set(e['jobs']) | {JID[j] for j in jobs})
    return f


def new_weapon(jobs, plus, stage):
    """jobs それぞれの「段階 stage 以下の店売り最強武器」の 1 つ上に 攻撃力+plus の新武器を段階 stage の店に足す（仮）"""
    def f(d):
        new = []
        for j in jobs:
            b = best_equip(d, JID[j], '武器', stage)
            n = dict(b); n['name'] = b['name'] + '＋%d(仮)' % plus; n['val'] = b['val'] + plus; n['eff'] = b['eff'] + plus
            n['jobs'] = [JID[j]]; n['shopStage'] = stage
            new.append(n)
        d['equips'].extend(new)
    return f


def new_armor(jobs, plus, stage, slot='鎧'):
    def f(d):
        new = []
        for j in jobs:
            b = best_equip(d, JID[j], slot, stage)
            n = dict(b); n['name'] = b['name'] + '＋%d(仮)' % plus; n['val'] = b['val'] + plus; n['eff'] = b['eff'] + plus
            n['jobs'] = [JID[j]]; n['shopStage'] = stage
            new.append(n)
        d['equips'].extend(new)
    return f


# ---------------------------------------------------------------- 節1: 要因の比較（事実の表）
def sec_facts(d, w, cfg):
    out = ['## 1. 他職との比較（段階ごとの標準プレイヤー）\n',
           '標準プレイヤー＝段階の目安Lv・その段階までの店売り最強装備（種・結晶・回復道具なし）。「他職」は旅人・戦士・僧侶。守備＝体力÷2＋防具、攻撃＝力＋武器。'
           '「通常1発」は同じ段階の雑魚の守備の中央値に対する通常攻撃の期待ダメージ（空振り込み）。\n']
    rows = []
    for sk, lv in STAGES:
        if sk not in SLIM:
            continue
        zs = zako_list(w, sk, cfg)
        zdf = median(e['st'] // 2 for e in zs)
        pl = {j: player(w, j, lv, sk) for j in REF + MS}
        ref = {k: median(pl[j][k] for j in REF) for k in ('hp', 'df', 'at', 'mana')}
        for j in REF + MS:
            p = pl[j]
            rel = lambda k: '' if j in REF else '（%d%%）' % round(p[k] * 100 / ref[k])
            sp = '%s（Mana%d・%s）' % (p['spell'], p['am'], ('攻撃力+%d' % p['aa']) if p['aisat'] else '%d' % p['aa']) if p['am'] else '—'
            rows.append([SLABEL[sk] if j == REF[0] else '', j, '%d%s' % (p['hp'], rel('hp')), '%d%s' % (p['df'], rel('df')),
                         '%d%s' % (p['at'], rel('at')), '%.0f' % exp_normal(p['at'], zdf), p['mana'], sp])
    out.append(R.table(['段階', '職', 'HP（他職中央値比）', '守備', '攻撃', '通常1発', 'Mana', '攻撃特技（1発）'], rows))
    out.append('')
    # 装備できる物
    out.append('### 装備（その段階の店売りで、その職が装備できる最強。括弧は値）\n')
    rows = []
    for sk, lv in STAGES:
        if sk not in SLIM:
            continue
        for j in REF + MS:
            p = player(w, j, lv, sk)
            pa = player(w, j, lv, sk, gear_any=True)
            ar = p['armor']
            lost_a = pa['wval'] - p['wval']
            lost_d = sum(v for _, v in pa['armor'].values()) - sum(v for _, v in ar.values())
            rows.append([SLABEL[sk] if j == REF[0] else '', j, '%s(%d)' % (p['weapon'], p['wval']),
                         '鎧 %s(%d)／盾 %s(%d)／兜 %s(%d)' % (ar['鎧'][0], ar['鎧'][1], ar['盾'][0], ar['盾'][1], ar['兜'][0], ar['兜'][1]),
                         '武器 -%d・防具 -%d' % (lost_a, lost_d) if (lost_a or lost_d) else '0'])
    out.append(R.table(['段階', '職', '武器', '防具', '旅人・戦士の最強との差（職業制限の損）'], rows))
    out.append('')
    # 特技と書
    out.append('### 魔法使い・召喚士の特技の書（入手段階・値段）\n')
    out.append('「雑魚何体分」＝本の値段÷その段階の雑魚1体の平均ゴールド（goldMed）。標準プレイヤーは値段を無視して「段階まで店に並ぶ物は全部持っている」前提なので、'
               '値段が高い物ほど実際の入手は遅い（シミュレータの限界）。\n')
    rows = []
    for j in MS:
        jid = JID[j]
        for m in sorted([m for m in d['magics'] if m['kind'] == 'player' and jid in m['jobs'] and m.get('avg')],
                        key=lambda m: min([b['stage'] or 99 for b in m['books']] or [99])):
            if not m['books']:
                continue
            b = m['books'][0]
            gm = w.chk.get({1: 'S1', 2: 'S2', 3: 'S3a', 4: 'S4', 5: 'S5', 6: 'S6'}.get(b['stage'], 'S6'), {}).get('goldMed') or 1
            rows.append([j, m['name'].split('（')[0], m['mana'], m['time'], '%g' % m['avg'], '段階%s' % b['stage'], '{:,}G'.format(b['gold']),
                         '%d体' % round(b['gold'] / gm), b['routes'][0]['v'][:24] if b['routes'] else '', m['dmgType']])
    out.append(R.table(['職', '特技', 'Mana', 'time', '平均', '書の段階', '値段', '雑魚何体分', '売っている所', '型'], rows))
    out.append('')
    out.append('型: 1=直接ダメージ（HP減少）、4=置く（飛ばす）→当たる、7=置く→さらに置く→当たる（召喚）。シミュレータは型 4・7 を「1 回の使用で 1 回当たる」として扱う。\n')
    # Mana
    out.append('### Mana 満タンで続けて倒せる雑魚の数（特技だけで倒す場合の目安）\n')
    out.append('同じ段階の雑魚の HP の中央値を、攻撃特技の 1 発（敵の魔法防御を引いた値）で倒すのに要る回数×Mana で、Mana の最大値を割った数（Mana は戦闘中に回復しない。宿・薬・イベントでの回復は別）。'
               '「—」は特技が通常攻撃より弱いので使わない場合。\n')
    rows = []
    for sk, lv in STAGES:
        if sk not in SLIM:
            continue
        zs = zako_list(w, sk, cfg)
        hp = median(e['hp'] for e in zs)
        dfm = median(e.get('dfm', 0) for e in zs)
        zdf = median(e['st'] // 2 for e in zs)
        cells = []
        for j in MS:
            p = player(w, j, lv, sk)
            aa = p['aa'] - dfm // 2
            if not p['am'] or aa < exp_normal(p['at'], zdf):
                cells.append('—（通常攻撃のほうが強い）' if p['am'] else '—')
                continue
            casts = -(-hp // aa)
            cells.append('%d 体（%s・%d 回×Mana%d）' % (p['mana'] // (casts * p['am']), p['spell'], casts, p['am']))
        rows.append([SLABEL[sk], '%d' % hp] + cells)
    out.append(R.table(['段階', '雑魚HPの中央値'] + MS, rows))
    out.append('')
    return out


# ---------------------------------------------------------------- 節2: 要因の入れ替え
def sec_swap(d, w, cfg, batch, a):
    rw = Rows(d, w, batch, cfg)
    reqs = {}
    for j in MS:
        for sk, lv in STAGES:
            if sk not in SLIM:
                continue
            p = player(w, j, lv, sk)
            pa = player(w, j, lv, sk, gear_any=True)
            refp = [player(w, r, lv, sk) for r in REF]
            med = lambda k: int(median(x[k] for x in refp))
            sm = dict(smart=True)
            pb = player(w, j, lv, sk, book_so=min(w.stage[sk]['order'], 6) + 1)
            V = [('現状（goal-check と同じ）', p, {}),
                 ('通常攻撃だけ（Mana切れ後）', p, dict(nobuff=True)),
                 ('特技は強い時だけ使う', p, sm),
                 ('＋Mana 無限', dict(p, mana=9999), sm),
                 ('＋HP→他職中央値', dict(p, hp=med('hp')), sm),
                 ('＋守備→他職中央値', dict(p, df=med('df')), sm),
                 ('＋攻撃(力+武器)→他職中央値', dict(p, at=med('at')), sm),
                 ('＋武器の職業制限なし', dict(p, at=pa['at'], wmin=pa['wmin']), sm),
                 ('＋防具の職業制限なし', dict(p, df=pa['df']), sm),
                 ('＋HP・守備・攻撃 全部→他職中央値', dict(p, hp=med('hp'), df=med('df'), at=med('at')), sm),
                 ('特技の書だけ 1 段階早く（段階+1 の店の書まで）', pb, sm),
                 ('HP・守備・攻撃 全部→他職中央値＋特技の書も 1 段階早く', dict(pb, hp=med('hp'), df=med('df'), at=med('at')), sm)]
            for name, q, kw in V:
                reqs[(j, sk, name)] = [(e, batch.add(row(q, e, 1, **kw))) for e in zako_list(w, sk, cfg)]
    batch.run()
    names = [v for v in dict.fromkeys(k[2] for k in reqs)]
    out = ['## 2. 要因を 1 つずつ他職（旅人・戦士・僧侶の中央値）に入れ替えた時の勝率\n',
           '各セルは「雑魚の平均勝率／最低勝率（%）」（雑魚 1 体を 1 人で。90% 未満の最低値は太字）。1 項目だけ入れ替える。'
           '「通常攻撃だけ」は特技を使わない場合（Mana が切れた後と同じ）。「特技は強い時だけ使う」は、特技 1 発が通常攻撃の期待ダメージより小さい敵には特技を使わない'
           '（battlesim は Mana があれば常に特技を使うため、特技が弱い段階では逆効果になる）。4 行目以降は「特技は強い時だけ使う」を入れた上で 1 項目だけ動かす。乱数のぶれは ±3pt 程度。\n']
    for j in MS:
        out.append('### %s\n' % j)
        rows = []
        for n in names:
            rows.append([n] + [cell(rw.stat(reqs[(j, sk, n)])) for sk in SLIM])
        out.append(R.table(['入れ替える項目'] + [SLABEL[s] for s in SLIM], rows))
        out.append('')
        worst = ['%s: %s' % (SLABEL[sk], rw.stat(reqs[(j, sk, names[0])])['worst']) for sk in SLIM if rw.stat(reqs[(j, sk, names[0])])['min'] < .9]
        out.append('最低勝率の敵: ' + '、'.join(worst) + '\n')
    return out


# ---------------------------------------------------------------- 節3: シミュレータで再現できていない役割
VIEW_A = dict()                                              # goal-check と同じ見方
VIEW_B = dict(smart=True, dfm=True, ranged=True)             # 補正込み（召喚の複数回ヒットは入れない）
VIEW_C = dict(smart=True, dfm=True, ranged=True, summon=4.0)  # 見方B＋召喚が 1 回の使用で 4 回当たると仮定（実機で未確認）


def sec_role(d, w, cfg, batch):
    rw = Rows(d, w, batch, cfg)
    pl = {(j, sk): player(w, j, lv, sk) for j in MS for sk, lv in STAGES if sk in SLIM}
    mult = [1, 2, 3, 4, 6, 8, 12, 16]
    reqs = {}
    for sk in SLIM:
        for name, kw in [('現状（goal-check と同じ）', {}),
                         ('遠隔の先撃ち（火・暗黒弾 などの射程）', dict(ranged=True)),
                         ('敵の魔法防御を反映', dict(dfm=True)),
                         ('特技は強い時だけ使う', dict(smart=True)),
                         ('遠隔＋特技は強い時だけ', dict(ranged=True, smart=True)),
                         ('敵の魔法防御＋特技は強い時だけ', dict(dfm=True, smart=True)),
                         ('3つ全部（補正込み）', dict(ranged=True, smart=True, dfm=True))]:
            reqs[('魔法使い', sk, name)] = rw.zako(pl[('魔法使い', sk)], sk, **kw)
        for m in mult:
            reqs[('召喚士', sk, m)] = rw.zako(pl[('召喚士', sk)], sk, summon=float(m), dfm=True)
        reqs[('召喚士', sk, 'raw')] = rw.zako(pl[('召喚士', sk)], sk)
        reqs[('召喚士', sk, 'dfm')] = rw.zako(pl[('召喚士', sk)], sk, dfm=True)
    batch.run()
    out = ['## 3. シミュレータが再現できていない役割は、結果を不当に低くしているか\n',
           '各セルは「雑魚の平均勝率／最低勝率（%）」（雑魚 1 体を 1 人で。90% 未満の最低値は太字）。\n',
           '### 3-1. 魔法使い（遠隔の高火力）\n',
           '遠隔の先撃ち: 特技の射程内（火＝前3マス。爆炎・爆発は目の前だけ）に敵が入ったら、手前に来る前でも特技を撃つ（ゲージは普通に溜まる。動かない敵には、歩いて近づく間に撃つ）。'
           '敵の魔法防御: 本番は特技のダメージから「魔法防御÷2」を引く（battlesim は引かない）ので、攻撃特技（攻撃力＋値型以外）から引く。\n']
    names = list(dict.fromkeys(k[2] for k in reqs if k[0] == '魔法使い'))
    rows = [[n] + [cell(rw.stat(reqs[('魔法使い', sk, n)])) for sk in SLIM] for n in names]
    out.append(R.table(['見方'] + [SLABEL[s] for s in SLIM], rows))
    out.append('')
    out.append('### 3-2. 召喚士（召喚の特技）\n')
    out.append('召喚（獣召喚・炎鬼召喚・氷女召喚）は「置く→さらに置く→当たる」型で、本番では置いた物が約1秒残り、その間に何度も当たる可能性がある（根拠は資料の付録A）。'
               'battlesim は 1 回の使用で 1 回しか当てない。1 回の使用で当たる回数を ×1〜×16 に変えた時の勝率（敵の魔法防御は反映済み）。\n')
    rows = [['×%d%s' % (m, '（今のシミュレータ）' if m == 1 else '')] + [cell(rw.stat(reqs[('召喚士', sk, m)])) for sk in SLIM] for m in mult]
    rows.insert(0, ['（参考）現状 goal-check と同じ（魔法防御なし）'] + [cell(rw.stat(reqs[('召喚士', sk, 'raw')])) for sk in SLIM])
    out.append(R.table(['1回の使用で当たる回数'] + [SLABEL[s] for s in SLIM], rows))
    out.append('')
    need = []
    for sk in SLIM:
        ok = [m for m in mult if rw.stat(reqs[('召喚士', sk, m)])['min'] >= .9]
        need.append('%s: %s' % (SLABEL[sk], '×%d' % ok[0] if ok else '×16 でも届かない'))
    out.append('目標1（最低 90%）に届く最小の回数: ' + '、'.join(need) + '\n')
    return out, rw, reqs


# ---------------------------------------------------------------- 節4: 案
def plans():
    P = []
    add = lambda key, name, note, *fns: P.append((key, name, note, fns))
    add('0', '0 現状', '', )
    # 職業表
    add('T1', 'T1 HP を 旅人・戦士・僧侶の中央値の 70% に（魔・召。今より下げない）', '案Dの魔法使いは Lv15 で 44 → 61、召喚士は 50 → 61', hp_pct(.7))
    add('T2', 'T2 力 +5（魔・召 Lv1〜15）', '通常攻撃と武器込みの攻撃力が上がる', job_add(MS, pw=5))
    add('T3', 'T3 体力 +8（魔・召 Lv1〜15。守備 +4）', '', job_add(MS, st=8))
    add('T4', 'T4 T1＋力+5＋体力+8', '職業表だけで直す場合の目安', hp_pct(.7), job_add(MS, pw=5, st=8))
    # 特技・書
    add('S1', 'S1 魔: 爆発（Mana7・25〜40・周囲）の書を段階3の店に前倒し', '今は段階4・6000G', book_stage(6, 3))
    add('S2', 'S2 魔: 爆発の書を段階2の店に前倒し', '', book_stage(6, 2))
    add('S3', 'S3 魔: 火の威力 10〜15 → 20〜30', '平均 12.5 → 25', spell_dmg(5, mult=2.0))
    add('S4', 'S4 召: 炎鬼召喚（Mana8・15〜20）の契約書を段階2の店に前倒し', '今は段階3・20000G（フィレーン）', book_stage(83, 2))
    add('S5', 'S5 召: 獣召喚の威力 4〜8 → 12〜24（×3）', '平均 6 → 18', spell_dmg(79, mult=3.0))
    add('S7', 'S7 召: 炎鬼召喚・氷女召喚の威力 ×3（15〜20 → 45〜60、18〜25 → 54〜75）', '召喚が 1 回しか当たらなかった場合の保険', spell_dmg(83, mult=3.0), spell_dmg(86, mult=3.0))
    add('S6', 'S6 魔 S1＋召 S4＋S5', '特技の入手・威力をまとめて', book_stage(6, 3), book_stage(83, 2), spell_dmg(79, mult=3.0))
    # 装備
    add('E1', 'E1 召喚士も僧侶と同じ装備（赤銅の杖・銀のナイフ・皮の盾・木の盾）を装備可', '段階1の武器 2 → 7、盾なし → 1〜2',
        equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']))
    add('E2', 'E2 魔・召も盾・兜（ラウンド・カイト・タワー・黒曜石の盾、チェインフード・鋼の兜・フルヘルム・黒曜石の兜）を装備可', '段階3の守備 +6（盾）+2（兜）',
        equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS))
    add('E3', 'E3 魔・召の杖を 段階2で +6・段階3で +15（新しい杖を店に足す）', '段階3は 28 → 43（旅人の剣と同じ）', new_weapon(MS, 6, 2), new_weapon(MS, 15, 3))
    add('E4', 'E4 魔・召のローブを 段階3で +6（ラピスの胸当て25 → 31）', '', new_armor(MS, 6, 3))
    # 組み合わせ
    add('C1', 'C1 T1＋E1＋E2（HPと装備だけ）', '', hp_pct(.7), equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']),
        equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS))
    add('C2', 'C2 C1＋S1＋S4（特技の入手前倒しも）', '', hp_pct(.7), equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']),
        equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS),
        book_stage(6, 3), book_stage(83, 2))
    add('C3', 'C3 T4＋E1＋E2＋S6（全部）', '強くなりすぎの確認用', hp_pct(.7), job_add(MS, pw=5, st=8), equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']),
        equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS),
        book_stage(6, 3), book_stage(83, 2), spell_dmg(79, mult=3.0))
    ALLEQ = [equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']),
             equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS)]
    add('R1', 'R1 E1＋E2＋S1＋S4＋召喚士だけHPを T1（魔法使いのHPは今の案Dのまま）', '魔法使いの表は触らない', hp_pct(.7, jobs=['召喚士']), *ALLEQ, book_stage(6, 3), book_stage(83, 2))
    add('R2', 'R2 R1＋S7（召喚が 1 回しか当たらない場合の保険。炎鬼召喚の前倒しは外す）', '', hp_pct(.7, jobs=['召喚士']), *ALLEQ, book_stage(6, 3), spell_dmg(83, mult=3.0), spell_dmg(86, mult=3.0))
    return P


def goal_bosses(w, sk, cfg):
    boss_names = set(cfg.get('boss_override', []))
    bs = list(w.enemies(sk, 'boss'))
    have = {e['name'] for e in bs}
    bs += [e for e in w.enemies(sk, 'zako') if e['name'] in boss_names and e['name'] not in have]
    return bs


class PlanEval:
    def __init__(self, d, batch, cfg, kw):
        self.d, self.b, self.cfg, self.kw = d, batch, cfg, kw
        self.w = R.World(d)
        w = self.w
        self.z = {}
        self.pl = {}
        for sk, lv in STAGES:
            for j in JOBS:
                self.pl[(j, sk)] = player(w, j, lv, sk)
        for j in MS + REF:
            for sk in SLIM:
                self.z[(j, sk)] = [(e, batch.add(row(self.pl[(j, sk)], e, 1, **kw))) for e in zako_list(w, sk, cfg)]
        # 副作用: 段階1・2 の雑魚を倒す秒（前衛 旅戦武 の平均を 1 とした比）。強さを見るため全職 1 体・補正なしの見方（kw）
        self.kill = {}
        for j in MS + ['旅人', '戦士', '武道家']:
            for sk in ('S1', 'S2', 'S3a', 'S4'):
                self.kill[(j, sk)] = [(e, batch.add(row(self.pl[(j, sk)], e, 1, **kw))) for e in zako_list(w, sk, cfg)]
        # ボス格: 1人（段階3以降）・2人・3人の代表編成
        self.bsolo, self.bparty = {}, {}
        comps = [(int(n), c) for n, cl in sorted(cfg['parties'].items()) for c in cl]
        for sk, lv in STAGES:
            if sk not in cfg['stages']:
                continue
            for e in goal_bosses(w, sk, cfg):
                if w.stage[sk]['order'] >= cfg.get('boss_solo_check_from_order', 1):
                    for j in MS:
                        self.bsolo[(sk, e['name'], j)] = batch.add(row(self.pl[(j, sk)], e, 1, **kw))
                for n, c in comps:
                    ps = [self.pl[(x, sk)] for x in c]
                    self.bparty[(sk, e['name'], n, tuple(c))] = batch.add(row(ps[0], e, 1, allies=ps[1:], **kw))

    def stat(self, j, sk):
        ws = [self.b[r]['win'] for _, r in self.z[(j, sk)]]
        return min(ws), mean(ws)

    def kill_ratio(self, j, sk):
        def sec(job):
            xs = [self.b[r] for _, r in self.kill[(job, sk)]]
            xs = [x['sec'] for x in xs if x['win'] > .5]
            return median(xs) if xs else None
        front = [sec(x) for x in ('旅人', '戦士', '武道家')]
        s = sec(j)
        return None if s is None or None in front else s / (sum(front) / 3)

    def boss_stats(self):
        solo_over = [(k, self.b[r]['win']) for k, r in self.bsolo.items() if self.b[r]['win'] > .4]
        med = {}
        for (sk, en, n, c), r in self.bparty.items():
            med.setdefault((sk, en, n), []).append(self.b[r]['win'])
        meds = {k: median(v) for k, v in med.items()}
        ms = [self.b[r]['win'] for (sk, en, n, c), r in self.bparty.items() if set(c) & set(MS)]
        return solo_over, meds, ms


def sec_plans(d0, cfg, batch):
    P = plans()
    ev = {}
    for key, name, note, fns in P:
        d = apply(d0, *fns)
        ev[(key, 'A')] = PlanEval(d, batch, cfg, VIEW_A)
        ev[(key, 'B')] = PlanEval(d, batch, cfg, VIEW_B)
        ev[(key, 'C')] = PlanEval(d, batch, cfg, VIEW_C)
    batch.run()
    out = ['## 4. 案の比較\n',
           '案は魔法使い・召喚士だけに効く（他職は変えないので、他職の数字は全案で同じ）。評価は各段階の標準プレイヤーで、雑魚 1 体を 1 人で戦った勝率。'
           '表のセルは「雑魚の最低勝率（%）」で 90% 未満は太字。「外れ」は 6 段階（段階1・2・3a・3b・4・6。段階1 は全案で合格なので表では省く）× 魔・召＝12 組のうち、最低勝率が 90% 未満の数。\n']
    cols = [(j, sk) for j in MS for sk in SLIM if sk != 'S1']
    for view, title in (('A', '4-1. 見方A: 今のシミュレータのまま（goal-check と同じ）'),
                        ('B', '4-2. 見方B: 補正込み（特技は強い時だけ使う＋敵の魔法防御を反映＋遠隔の先撃ち。召喚の複数回ヒットは入れない）'),
                        ('C', '4-3. 見方C: 見方B＋召喚が 1 回の使用で 4 回当たると仮定（実機で未確認。3-2 の ×4）')):
        out.append('### %s\n' % title)
        rows = []
        e0 = ev[('0', view)]
        rows.append(['（参考）旅人・戦士・僧侶の最低'] + [('%s' % pc(min(e0.stat(r, sk)[0] for r in REF))) for j, sk in cols] + ['', ''])
        for key, name, note, fns in P:
            x = ev[(key, view)]
            cells = []
            bad = 0
            for j, sk in cols:
                m = x.stat(j, sk)[0]
                cells.append('**%s**' % pc(m) if m < .9 else pc(m))
            for j in MS:
                for sk in SLIM:
                    bad += 1 if x.stat(j, sk)[0] < .9 else 0
            rows.append([name] + cells + ['%d／%d' % (bad, len(MS) * len(SLIM))])
        head = ['案'] + ['%s %s' % (j[0], SLABEL[sk].split('(')[0]) for j, sk in cols] + ['外れ']
        out.append(R.table(head, rows))
        out.append('')
    out.append('### 4-4. 強くなりすぎないか・他への副作用（見方A）\n')
    out.append('「倒す秒の比」は段階1・2・3a・4 の雑魚を 1 人で倒す秒（勝てた戦の中央値）を、旅人・戦士・武道家の平均を 1 とした比。1 を大きく下回ると前衛より速い＝強すぎ。'
               '「ボス格」は段階3以降。1人>40% は魔・召が 1 人でボス格に 40% 超で勝つ数（目標3は 40% 以下）。2人・3人は代表編成の中央値のうち、段階3〜6 のボス格での最低（目標 70% 以上）。'
               '（2人の中央値は魔・召を変えても動かない＝2人編成の問題は別の原因）。\n')
    rows = []
    for key, name, note, fns in P:
        x = ev[(key, 'A')]
        kr = ['%s %s' % (j[0], ('%.2f' % x.kill_ratio(j, sk)) if x.kill_ratio(j, sk) else '—') for sk in ('S1', 'S2', 'S3a', 'S4') for j in MS]
        solo_over, meds, ms = x.boss_stats()
        m2 = min(v for (sk, en, n), v in meds.items() if n == 2)
        m3 = min(v for (sk, en, n), v in meds.items() if n == 3)
        avg = lambda j: mean(x.stat(j, sk)[1] for sk in SLIM)
        rows.append([name, ' ／ '.join(kr[0:2]), ' ／ '.join(kr[2:4]), ' ／ '.join(kr[4:6]), ' ／ '.join(kr[6:8]), len(solo_over), pc(m2), pc(m3), '%s／%s' % (pc(avg('魔法使い')), pc(avg('召喚士')))])
    x0 = ev[('0', 'A')]
    rows.insert(0, ['（参考）旅人／戦士／僧侶', '1.00（前衛平均）', '1.00（前衛平均）', '1.00（前衛平均）', '1.00（前衛平均）', '', '', '',
                    '%s／%s／%s' % tuple(pc(mean(x0.stat(j, sk)[1] for sk in SLIM)) for j in REF)])
    out.append(R.table(['案', '段階1 倒す秒の比（魔／召）', '段階2 倒す秒の比（魔／召）', '段階3a 倒す秒の比（魔／召）', '段階4 倒す秒の比（魔／召）', 'ボス格 1人>40%', '2人 中央値の最低', '3人 中央値の最低', '雑魚の平均勝率（6段階の平均）'], rows))
    out.append('')
    return out, ev


def sec_pairs(d0, cfg, batch):
    """何もしない案: 1 人で戦う想定をやめて、前衛と組んだ時（2人）に雑魚を倒せるか"""
    w = R.World(d0)
    pl = {(j, sk): player(w, j, lv, sk) for j in JOBS for sk, lv in STAGES}
    pairs = [('旅人', '魔法使い'), ('戦士', '魔法使い'), ('僧侶', '魔法使い'), ('旅人', '召喚士'), ('戦士', '召喚士'), ('僧侶', '召喚士')]
    req = {}
    for sk in SLIM:
        for a_, b_ in pairs:
            for n in (1, 2, 3):
                req[(sk, a_, b_, n)] = [(e, batch.add(row(pl[(a_, sk)], e, n, allies=[pl[(b_, sk)]]))) for e in zako_list(w, sk, cfg)]
    batch.run()
    out = ['### 4-6. 案N「何もしない」＝1人ソロを想定せず、前衛（旅人・戦士・僧侶）と 2 人で組んだ時\n',
           '先頭が前衛（敵に狙われる側）、もう 1 人が魔法使い／召喚士。雑魚 1 体・2 体・3 体と戦った最低勝率（%）。90% 未満は太字。見方A（今のシミュレータのまま）。\n']
    rows = []
    for a_, b_ in pairs:
        cells = []
        for n in (1, 2, 3):
            cells.append(' ／ '.join(('**%s**' if min(batch[r]['win'] for _, r in req[(sk, a_, b_, n)]) < .9 else '%s') % pc(min(batch[r]['win'] for _, r in req[(sk, a_, b_, n)])) for sk in SLIM))
        rows.append(['%s＋%s' % (a_, b_)] + cells)
    out.append(R.table(['編成'] + ['雑魚%d体（%s）' % (n, '／'.join(s.replace('S', '段') for s in SLIM)) for n in (1, 2, 3)], rows))
    out.append('')
    return out


def sec_price(d0, cfg, batch, budget=100):
    """特技の書の値段まで考えた場合: 「その段階の雑魚 budget 体分のゴールド以下」の書だけ持っているとする（装備は従来どおり値段を無視）"""
    ALLEQ = [equip_jobs(['赤銅の杖', '銀のナイフ', '皮の盾', '木の盾'], ['召喚士']),
             equip_jobs(['ラウンドシールド', 'カイトシールド', 'タワーシールド', '黒曜石の盾', 'チェインフード', '鋼の兜', 'フルヘルム', '黒曜石の兜'], MS)]
    cheap = book_price({79: 600, 83: 2500, 86: 6000, 6: 3000, 22: 8000})
    V = [('0 現状（書の値段のまま）', []),
         ('P1 書の値段だけ下げる（獣600・炎鬼2500・氷女6000・爆発3000・爆炎8000 G）', [cheap]),
         ('R1 装備＋爆発/炎鬼の前倒し＋召喚士HP（書の値段は現状）', [hp_pct(.7, jobs=['召喚士']), *ALLEQ, book_stage(6, 3), book_stage(83, 2)]),
         ('R1＋P1（書の値段も下げる）', [hp_pct(.7, jobs=['召喚士']), *ALLEQ, book_stage(6, 3), book_stage(83, 2), cheap])]
    BUDGET[0] = budget
    ev = {}
    for k, fns in V:
        ev[k] = PlanEval(apply(d0, *fns), batch, cfg, VIEW_B)
    BUDGET[0] = None
    batch.run()
    out = ['### 4-5. 特技の書の値段まで考えた場合（見方B）\n',
           '標準プレイヤーは書の値段を無視しているが、実際は買えないと使えない。ここでは「その段階の雑魚 %d 体分のゴールド（goldMed）以下の書だけ持っている」とした（武器 1 本が雑魚 60〜125 体分なので、書 1 冊もその程度までが現実的、という仮置き）。'
           '装備は従来どおり値段を無視。セルは雑魚の最低勝率（%%）。\n' % budget]
    cols = [(j, sk) for j in MS for sk in SLIM]
    rows = []
    for k, _ in V:
        x = ev[k]
        rows.append([k] + [('**%s**' if x.stat(j, sk)[0] < .9 else '%s') % pc(x.stat(j, sk)[0]) for j, sk in cols])
    out.append(R.table(['案'] + ['%s %s' % (j[0], SLABEL[sk].split('(')[0].replace('段階', '段')) for j, sk in cols], rows))
    out.append('')
    return out


def run(d0, cfg, batch, parts):
    w = R.World(d0)
    out = ['# 魔法使い・召喚士が目標1を満たさない原因と案の比較（表）\n']
    if '1' in parts:
        out += sec_facts(d0, w, cfg)
    if '2' in parts:
        out += sec_swap(d0, w, cfg, batch, None)
    if '3' in parts:
        o, _, _ = sec_role(d0, w, cfg, batch)
        out += o
    if '4' in parts:
        o, _ = sec_plans(d0, cfg, batch)
        out += o
        out += sec_price(d0, cfg, batch)
        out += sec_pairs(d0, cfg, batch)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--simdata', default=os.path.join(WORK, 'simdata.json'))
    ap.add_argument('--goal', default=os.path.join(HERE, 'goal.json'))
    ap.add_argument('--runs', type=int, default=1000)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--exe', default='/tmp/battlesim')
    ap.add_argument('--summon', type=float, default=4.0, help='見方Cで仮定する、召喚が 1 回の使用で当たる回数（既定 4）')
    ap.add_argument('--part', default='1,2,3,4', help='出す節（1=要因の比較 2=入れ替え 3=役割の再現 4=案）')
    a = ap.parse_args()
    parts = set(a.part.split(','))
    VIEW_C['summon'] = a.summon
    d0 = json.load(open(a.simdata, encoding='utf-8'))
    cfg = json.load(open(a.goal, encoding='utf-8'))
    batch = R.Batch(R.build_exe(a.exe), a.runs, a.seed)
    out = run(d0, cfg, batch, parts)
    print('\n'.join(out))


if __name__ == '__main__':
    main()
