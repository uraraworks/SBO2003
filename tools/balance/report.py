#!/usr/bin/env python3
"""バランスレポート生成（battlesim の上に載せる）。Markdown を 1 ファイル出す。
(1) 段階×職業×敵（雑魚・ボス格、1体と3体）の勝率・残HP・所要秒・判定
(2) 職業別の成長比較 Lv1〜15（魔法使いのHPが低すぎる、という要望の調査用。種込みの別列つき）
(3) --hp-override で職業のLv別最大HPを一時的に書き換えた時の差分（データは書き換えない）
(目標チェック) --goal-check: 雑魚1体は1人で勝てるか・ボス格は2〜3人で勝てて1人だと厳しいかを goal.json の基準で調べ、
    goal-check.md（外れている所と「直す候補リスト」）を出す。通常のレポートは出さない。

使い方: python3 tools/balance/report.py [--simdata simdata.json] [--out report.md] [--runs 1000] [--seed 1]
                                         [--hp-override 魔法使い:1=14,2=18,...] [--refresh] [--only 1|2]
詳しくは tools/balance/README.md。"""
import argparse, json, math, os, random, shutil, subprocess, sys, tempfile
from statistics import median, mean

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import enemyskill
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
WORK = '/mnt/project-files/_work/sbo2003-balance-sim'
VERDICTS = ['余裕', '互角', 'きつい', '無理']
SEED_HP, SEED_MANA, SEED_CRYSTAL = 25, 20, 120   # 種の上限（HP25・Mana20）と生命の結晶（HP120）


def verdict(w, h):
    """BattleSim.h の Verdict と同じ（w, h は 0〜1）"""
    return '余裕' if (w >= .95 and h >= .5) else '互角' if w >= .8 else 'きつい' if w >= .4 else '無理'


# ---------------------------------------------------------------- データ取得
def refresh(simdata_path):
    """本番を読んで dex.json → simdata.json を作る（SBO2003_AI_KEY が要る。書き込みはしない）"""
    if not os.environ.get('SBO2003_AI_KEY'):
        raise SystemExit('error: SBO2003_AI_KEY が未設定')
    tmp = tempfile.mkdtemp(prefix='sbo-balance-')
    prod = os.path.join(tmp, 'prod'); os.mkdir(prod)
    b = os.path.join(HERE, 'dexbuild')
    env = dict(os.environ, PROD=prod, SBO_AT=__import__('datetime').date.today().isoformat())
    subprocess.check_call([sys.executable, '-I', os.path.join(b, 'fetch.py'), prod], env=env, stdout=subprocess.DEVNULL)
    dex = os.path.join(tmp, 'dex.json')
    subprocess.check_call([sys.executable, '-I', os.path.join(b, 'build_dex.py'), dex], env=env, stdout=subprocess.DEVNULL)
    subprocess.check_call([sys.executable, '-I', os.path.join(b, 'prep.py'), dex, prod, simdata_path], env=env)
    shutil.rmtree(tmp, ignore_errors=True)


# ---------------------------------------------------------------- 標準プレイヤー（simdata2tsv.py の build と同じ作り方）
class World:
    def __init__(self, d, hp_override=None):
        self.d = d
        self.hpo = hp_override or {}
        self.stage = {s['key']: s for s in d['stages']}
        self.chk = {c['key']: c for c in d['checks']}
        self.jobs = [j['name'] for j in d['jobs']]

    def hp_at(self, job, lv, base):
        """--hp-override の適用。値は 数値（置き換え）か +N/-N（加算）"""
        v = self.hpo.get((job, lv))
        if v is None:
            return base
        kind, n = v
        return base + n if kind == 'add' else n

    def player(self, job, lv, stage_key, seed_hp=0, seed_mana=0):
        d = self.d
        so = min(self.stage[stage_key]['order'], 6)
        jt = next(j for j in d['jobTables'] if j['name'] == job)
        row = jt['rows'][lv - 1]
        jid = next(j['id'] for j in d['jobs'] if j['name'] == job)

        def best(slot):
            c = [e for e in d['equips'] if e['slot'] == slot and jid in e['jobs'] and e.get('shopStage') is not None
                 and e['shopStage'] <= so and e['kind'] == ('攻撃力' if slot == '武器' else '守備力')
                 and (slot != '武器' or e['atk'] == '直接')]
            return sorted(c, key=lambda e: -e['eff'])[0] if c else None
        w = best('武器')
        armor = [a for a in (best(s) for s in ['鎧', '盾', '兜', '靴']) if a]
        hp = self.hp_at(job, lv, row['wMAXHP']) + seed_hp
        mana = row['wMAXMANA'] + seed_mana

        def ok(m):
            return (m['kind'] == 'player' and jid in m['jobs'] and m.get('avg') and m['mana'] <= mana
                    and any(b['stage'] is not None and b['stage'] <= so for b in m['books']))
        heals = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] == 2], key=lambda m: m['mana'])
        atks = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] != 2 and 'HPを' not in m['effect']],
                      key=lambda m: -m['avg'])
        h = heals[0] if heals else None
        a = atks[0] if atks else None
        return dict(lv=lv, hp=hp, mana=mana, at=row['byPW'] + (w['val'] if w else 0),
                    df=row['byST'] // 2 + sum(e['val'] for e in armor), dex=row['byDEX'], wmin=w['min'] if w else 0,
                    hm=h['mana'] if h else 0, ha=int(h['avg']) if h else 0, ht=h['time'] if h else 0,
                    am=a['mana'] if a else 0, aa=int(a['avg']) if a else 0, at_=a['time'] if a else 0,
                    aisat=1 if a and a['isAT'] else 0)

    def enemies(self, stage_key, group):
        """group: 'zako' か 'boss'。名前の重複は 1 件にまとめる"""
        c = self.chk[stage_key]
        names = c['zako'] if group == 'zako' else [b['name'] for b in c['boss']]
        out, seen = [], set()
        for n in names:
            if n in seen:
                continue
            e = next((e for e in self.d['enemies'] if e['name'] == n), None)
            if e:
                out.append(e); seen.add(n)
        return out


ESKILL_ON = True    # --no-enemy-skills で False


def pl_cols(p, nobuff=False):
    """プレイヤー 14 列（Lv, HP, Mana, 攻撃力, 守備力, 素早さ, 武器byMin, 回復3, 攻撃特技3, AT型）"""
    hm, ha, ht = (0, 0, 0) if nobuff else (p['hm'], p['ha'], p['ht'])
    am, aa, at_ = (0, 0, 0) if nobuff else (p['am'], p['aa'], p['at_'])
    return [p['lv'], p['hp'], 0 if nobuff else p['mana'], p['at'], p['df'], p['dex'], p['wmin'],
            hm, ha, ht, am, aa, at_, p['aisat']]


def scen_row(p, e, count, dist=None, nobuff=False, eskill=None, allies=None):
    """battlesim の 1 行（名前以外 24 列＋敵の特技）。nobuff=True はプレイヤーが特技を使わない（通常攻撃のみ）。
    eskill=False は敵の特技を入れない（特技を入れる前の結果を出す用）。
    allies: 追加の味方（プレイヤー dict のリスト。p が先頭＝前衛）。あれば 26 列目に付ける"""
    moves = 1 if e['npcType'] == 13 else 0
    if dist is None:
        dist = max(2, e['searchCx']) if moves else 3
    cols = pl_cols(p, nobuff) + [e['hp'], e['pw'], e['st'] // 2, e['dex'], count, dist, moves, e['searchCx'], e.get('moveX') or 20]
    sk = enemyskill.skill_column(e)[0] if (ESKILL_ON if eskill is None else eskill) else '-'
    row = '\t'.join(str(int(c)) for c in cols) + '\t' + sk
    if allies:
        row += '\t' + '|'.join(','.join(str(int(c)) for c in pl_cols(a, nobuff)) for a in allies)
    return row


# ---------------------------------------------------------------- battlesim を 1 プロセスで回す
class Batch:
    def __init__(self, exe, runs, seed, rule=0, renkei=1):
        self.exe, self.runs, self.seed = exe, runs, seed
        self.rule, self.renkei = rule, renkei
        self.rows = {}      # 行の中身 → 結果（同じ行は 1 回だけ計算する）

    def add(self, row):
        self.rows.setdefault(row, None)
        return row

    def run(self):
        keys = [k for k, v in self.rows.items() if v is None]    # まだ計算していない行だけ
        if not keys:
            return
        inp = ''.join('%d\t%s\n' % (i, k) for i, k in enumerate(keys))
        r = subprocess.run([self.exe, str(self.seed), str(self.runs), str(self.rule), str(self.renkei)], input=inp, capture_output=True, text=True)
        if r.returncode != 0:
            raise SystemExit('error: battlesim 失敗: ' + r.stderr.strip())
        lines = r.stdout.strip().split('\n')[1:]
        assert len(lines) == len(keys), 'battlesim の出力行数が合わない'
        for k, ln in zip(keys, lines):
            c = ln.split('\t')
            self.rows[k] = dict(win=float(c[1]) / 100, hp=float(c[2]) / 100, sec=float(c[3]), heal=float(c[4]), v=c[5],
                                em=float(c[6]), eh=float(c[7]), dead=float(c[8]) if len(c) > 8 else 0.0)

    def __getitem__(self, row):
        return self.rows[row]


def build_exe(path):
    if os.path.exists(path) and os.path.getmtime(path) > max(
            os.path.getmtime(os.path.join(ROOT, 'common', 'BattleSim.h')),
            os.path.getmtime(os.path.join(HERE, 'battlesim.cpp'))):
        return path
    r = subprocess.run(['g++', '-std=c++11', '-O2', '-I', ROOT, os.path.join(HERE, 'battlesim.cpp'), '-o', path],
                       capture_output=True, text=True)
    if r.returncode != 0:
        errs = [l for l in r.stderr.split('\n') if 'error' in l]
        raise SystemExit('\n'.join(errs) or 'error: g++ 失敗')
    return path


# ---------------------------------------------------------------- 敵からの被ダメージ（BattleCalc::Damage の移植）
def py_damage(at, df, rng, crit=False):
    at = max(at, 2)
    if crit:
        at += at // 2
    t = at - df // 2
    if rng.random() < .5:
        t += t * (rng.randrange(16) + 1) // 100
    else:
        t -= t * (rng.randrange(16) + 1) // 100
    t = max(t, 0)
    if t == 0 and rng.randrange(100) > 66:
        t = 1
    return t


def enemy_dpm(e, df, n=300):
    """敵 1 体が 1 分間に与える期待ダメージ（空振り5%・会心2%、行動間隔は EnemyTurnTime）"""
    rng = random.Random(1)
    step = max(e['dex'] * 100 // 255, 1)
    turn = max(1000 + (4000 - 4000 * step // 100), 1)
    tot = 0
    for _ in range(n):
        if rng.randrange(100) >= 95:
            continue
        crit = rng.randrange(100) >= 98
        tot += py_damage(e['pw'], df, rng, crit)
    return tot / n * 60000 / turn


def stage_for_lv(lv):
    """Lv → その Lv で相手にする段階。目安Lv: S1=6, S2=8, S3a=9, S3b/S4/S5=10, S6=12。10〜11 は S4 を代表にする"""
    return 'S1' if lv < 8 else 'S2' if lv == 8 else 'S3a' if lv == 9 else 'S4' if lv < 12 else 'S6'


# ---------------------------------------------------------------- 表の部品
def pct(x):
    return '%.0f%%' % (x * 100)


def table(head, rows):
    out = ['| ' + ' | '.join(head) + ' |', '|' + '|'.join(['---'] * len(head)) + '|']
    out += ['| ' + ' | '.join(str(c) for c in r) + ' |' for r in rows]
    return '\n'.join(out)


def parse_override(s):
    """'魔法使い:1=14,2=+3,6-15=30' → {(職業, Lv): ('set'|'add', 値)}。職業名は次の「職:」まで引き継ぐ"""
    out, job = {}, None
    if not s:
        return out
    for item in s.split(','):
        item = item.strip()
        if ':' in item:
            job, item = [x.strip() for x in item.split(':', 1)]
        if job is None or '=' not in item:
            raise SystemExit('error: --hp-override の形式は 職業名:Lv=値,Lv=値 （値は +N で加算）: ' + s)
        k, v = [x.strip() for x in item.split('=', 1)]
        lo, hi = (int(a) for a in k.split('-')) if '-' in k else (int(k), int(k))
        val = ('add', int(v)) if v[:1] in '+-' else ('set', int(v))
        for lv in range(lo, hi + 1):
            out[(job, lv)] = val
    return out


# ---------------------------------------------------------------- (1) 段階×職業
def part1_requests(w, batch, runs_stages, eskill=None):
    """各 (段階, 職業, 群, 体数) の行リストを登録して返す"""
    req = {}
    for sk in runs_stages:
        lv = w.chk[sk]['lv']
        for job in w.jobs:
            p = w.player(job, lv, sk)
            for grp in ('zako', 'boss'):
                for cnt in (1, 3):
                    req[(sk, job, grp, cnt)] = [(e['name'], batch.add(scen_row(p, e, cnt, eskill=eskill))) for e in w.enemies(sk, grp)]
    return req


def agg(batch, lst):
    rs = [batch[r] for _, r in lst]
    if not rs:
        return None
    wins = [r['win'] for r in rs]
    vs = {v: sum(1 for r in rs if r['v'] == v) for v in VERDICTS}
    wr = [r for r in rs if r['win'] > 0]
    return dict(win=mean(wins), wmin=min(wins), hp=mean(r['hp'] for r in wr) if wr else 0,
                sec=mean(r['sec'] for r in wr) if wr else 0, n=len(rs),
                v=' '.join('%s%d' % (v, c) for v, c in vs.items() if c), vs=vs)


def part1_md(w, batch, req, stages, title_suffix=''):
    out = []
    for sk in stages:
        c = w.chk[sk]
        out.append('### %s（標準プレイヤー Lv%d・店売り最強装備）\n' % (c['label'], c['lv']))
        for grp, gname in (('zako', '雑魚'), ('boss', 'ボス格')):
            es = w.enemies(sk, grp)
            if not es:
                continue
            out.append('**%s**（%d種: %s）\n' % (gname, len(es), '、'.join('%s(HP%d/攻%d)' % (e['name'], e['hp'], e['pw']) for e in es)))
            rows = []
            for job in w.jobs:
                r = [job]
                for cnt in (1, 3):
                    a = agg(batch, req[(sk, job, grp, cnt)])
                    r += ['%s(最低%s)' % (pct(a['win']), pct(a['wmin'])), pct(a['hp']) if a['win'] else '—',
                          '%.0f' % a['sec'] if a['win'] else '—', a['v']]
                rows.append(r)
            out.append(table(['職業', '1体 勝率(最低)', '残HP', '秒', '判定内訳', '3体 勝率(最低)', '残HP', '秒', '判定内訳'], rows))
            out.append('')
    return '\n'.join(out)


# ---------------------------------------------------------------- (2) 成長比較
def part2_requests(w, batch, lvs):
    """Lv×職業×種の有無ごとに、(その段階の雑魚)に対する 倒す時間（特技あり・通常のみ）を登録"""
    req = {}
    for lv in lvs:
        sk = stage_for_lv(lv)
        zs = w.enemies(sk, 'zako')
        for job in w.jobs:
            for var, (sh, sm) in (('base', (0, 0)), ('seed', (SEED_HP, SEED_MANA)), ('crystal', (SEED_CRYSTAL, SEED_MANA))):
                p = w.player(job, lv, sk, sh, sm)
                kill = [(e, batch.add(scen_row(p, e, 1, dist=1))) for e in zs]
                plain = [(e, batch.add(scen_row(p, e, 1, dist=1, nobuff=True))) for e in zs]
                req[(lv, job, var)] = dict(p=p, sk=sk, kill=kill, plain=plain)
    return req


def part2_metrics(batch, r):
    p, zs = r['p'], r['kill']
    dpm = median(enemy_dpm(e, p['df']) for e, _ in zs)
    endure = p['hp'] / (dpm / 60) if dpm > 0 else float('inf')
    ks = [batch[x] for _, x in zs]
    kill_sec = median(k['sec'] for k in ks if k['win'] > .2) if any(k['win'] > .2 for k in ks) else None
    ps = [batch[x] for _, x in r['plain']]
    plain_sec = median(k['sec'] for k in ps if k['win'] > .2) if any(k['win'] > .2 for k in ps) else None
    kills = None
    if p['am'] > 0:
        per = []
        for e, _ in zs:
            d = p['aa'] + (p['at'] - e['st'] // 2 if p['aisat'] else 0)
            per.append((p['mana'] // p['am']) / math.ceil(e['hp'] / d) if d > 0 else 0)
        kills = median(per)
    return dict(dpm=dpm, endure=endure, kill=kill_sec, plain=plain_sec, kills=kills, p=p)


def fmt(x, nd=1, none='—'):
    if x is None:
        return none
    if x == float('inf'):
        return '∞'
    return ('%.' + str(nd) + 'f') % x


def part2_md(w, batch, req, lvs):
    M = {k: part2_metrics(batch, v) for k, v in req.items()}
    jobs = w.jobs
    out = []

    def tbl(title, f, var='base', note=''):
        rows = [['Lv%d' % lv] + [f(M[(lv, j, var)]) for j in jobs] for lv in lvs]
        out.append('**%s**%s\n' % (title, note))
        out.append(table(['Lv'] + jobs, rows))
        out.append('')
    tbl('最大HP', lambda m: m['p']['hp'])
    tbl('守備力（体力/2＋店売り最強防具）', lambda m: m['p']['df'])
    tbl('攻撃力（力＋店売り最強武器）', lambda m: m['p']['at'])
    tbl('最大Mana', lambda m: m['p']['mana'])
    out.append('敵は「Lv→段階」の対応（Lv1〜7=段階1、8=段階2、9=段階3a、10〜11=段階4、12〜=段階6）の雑魚の中央値で比べる。\n')
    tbl('雑魚1体から1分間に受けるダメージ', lambda m: fmt(m['dpm'], 0))
    tbl('耐えられる秒数（最大HP÷毎秒ダメージ、回復なし・雑魚1体に殴られ続けた場合）', lambda m: fmt(m['endure'], 0))
    tbl('雑魚1体を倒す秒（特技あり・目の前から開始）', lambda m: fmt(m['kill']), note='　勝率2割以下は —')
    tbl('雑魚1体を倒す秒（通常攻撃のみ＝Mana切れ後）', lambda m: fmt(m['plain']))
    tbl('Mana切れまでに倒せる雑魚の数（攻撃特技のMana÷1体に要る回数。特技なしは —）', lambda m: fmt(m['kills']))
    out.append('#### 種込み（HP+%d・Mana+%d を足した場合）\n' % (SEED_HP, SEED_MANA))
    tbl('最大HP（種込み）', lambda m: m['p']['hp'], 'seed')
    tbl('耐えられる秒数（種込み）', lambda m: fmt(m['endure'], 0), 'seed')
    tbl('Mana切れまでに倒せる数（種込み）', lambda m: fmt(m['kills']), 'seed')
    out.append('#### 結晶込み（HP+%d・Mana+%d を足した場合）\n' % (SEED_CRYSTAL, SEED_MANA))
    tbl('最大HP（結晶込み）', lambda m: m['p']['hp'], 'crystal')
    tbl('耐えられる秒数（結晶込み）', lambda m: fmt(m['endure'], 0), 'crystal')
    return '\n'.join(out), M


def part2_summary(w, M, lvs):
    """魔法使いの位置づけを数字から 2〜3 行にまとめる"""
    others = [j for j in w.jobs if j != '魔法使い']
    # 耐えられる秒数: 他職の中央値に対する魔法使いの比
    ratio = {}
    for lv in lvs:
        o = median(M[(lv, j, 'base')]['endure'] for j in others)
        ratio[lv] = M[(lv, '魔法使い', 'base')]['endure'] / o if o and o != float('inf') else None
    low = [lv for lv in lvs if ratio[lv] is not None and ratio[lv] < .5]
    worst = min((lv for lv in lvs if ratio[lv] is not None), key=lambda x: ratio[x])
    hpr = {lv: M[(lv, '魔法使い', 'base')]['p']['hp'] / median(M[(lv, j, 'base')]['p']['hp'] for j in others) for lv in lvs}
    # 火力: 特技あり倒す秒、通常のみ
    kr = {lv: (M[(lv, '魔法使い', 'base')]['kill'], median([M[(lv, j, 'base')]['kill'] for j in others if M[(lv, j, 'base')]['kill']] or [0]))
          for lv in lvs}
    pr = {lv: (M[(lv, '魔法使い', 'base')]['plain'], median([M[(lv, j, 'base')]['plain'] for j in others if M[(lv, j, 'base')]['plain']] or [0]))
          for lv in lvs}
    kk = [lv for lv in lvs if kr[lv][0] and kr[lv][1] and kr[lv][0] <= kr[lv][1]]
    mk = {lv: M[(lv, '魔法使い', 'base')]['kills'] for lv in lvs}
    o_mk = {lv: median([M[(lv, j, 'base')]['kills'] for j in others if M[(lv, j, 'base')]['kills']] or [0]) for lv in lvs}
    seed_gain = {lv: M[(lv, '魔法使い', 'seed')]['endure'] / M[(lv, '魔法使い', 'base')]['endure'] for lv in lvs if M[(lv, '魔法使い', 'base')]['endure'] not in (0, float('inf'))}
    def rng(f, lo, hi):
        v = [f(lv) for lv in lvs if lo <= lv <= hi and f(lv) is not None]
        return (min(v), max(v)) if v else (None, None)
    e1, e2 = rng(lambda lv: ratio[lv], 1, 8), rng(lambda lv: ratio[lv], 9, 15)
    lines = []
    lines.append('- 最大HPは他職中央値の %s〜%s（全Lv）。耐えられる秒数は Lv1〜8 で他職中央値の %s〜%s、Lv9〜15 で %s〜%s。%s'
                 % (pct(min(hpr.values())), pct(max(hpr.values())), pct(e1[0]), pct(e1[1]), pct(e2[0]), pct(e2[1]),
                    ('最も差が開くのは Lv%d（%s）。Lv9（段階3の雑魚）以降で落ち込みが大きい。' % (worst, pct(ratio[worst])))
                    if e2[1] < e1[0] else '落ち込みはLvに依らずほぼ一定。'))
    lines.append('- 火力は、特技ありで雑魚を倒す秒が他職中央値以下のLvが %d/%d。通常攻撃のみ（Mana切れ後）は Lv%d で魔法使い %s秒・他職中央値 %s秒 でほぼ同じ。Mana切れまでに倒せる数は Lv%d で魔法使い %s 体・他職(特技持ち)中央値 %s 体。%s'
                 % (len(kk), len(lvs), worst, fmt(pr[worst][0]), fmt(pr[worst][1]), worst, fmt(mk[worst]), fmt(o_mk[worst]),
                    '火力（と継戦）は他職を上回っていて、HPの低さとの引き換えとしては釣り合い以上。' if (mk[worst] or 0) > (o_mk[worst] or 0) else '火力は他職に劣る。'))
    g = seed_gain.get(worst, 0)
    lines.append('- 種込み（HP+%d）にすると魔法使いの耐えられる秒数は Lv%d で %.1f 倍（他職中央値との比 %s→%s）。魔法使いの Lv15 上限は HP%d なので種の上限(%d)は足し込める余地が大きく、種の時代の仮説（種で埋めてきた）は成り立つが、種なしで遊ぶ人には HP の低さがそのまま出る。'
                 % (SEED_HP, worst, g, pct(ratio[worst]), pct(M[(worst, '魔法使い', 'seed')]['endure'] / median(M[(worst, j, 'seed')]['endure'] for j in others)),
                    M[(15, '魔法使い', 'base')]['p']['hp'], SEED_HP))
    return '\n'.join(lines)


# ---------------------------------------------------------------- (3) 差分
def diff_md(w0, w1, batch, stages, req0, req1, p2_0, p2_1, lvs, changed_jobs):
    out = []
    out.append('### (1) の差分（勝率の変化。%s のみ）\n' % '・'.join(changed_jobs))
    for job in changed_jobs:
        out.append('**%s**: 標準プレイヤー（段階の目安Lv）での 勝率 変更前→変更後（差）／判定内訳\n' % job)
        rows = []
        for sk in stages:
            r = [w0.chk[sk]['label']]
            for grp in ('zako', 'boss'):
                for cnt in (1, 3):
                    if not w0.enemies(sk, grp):
                        r.append('—'); continue
                    a, b = agg(batch, req0[(sk, job, grp, cnt)]), agg(batch, req1[(sk, job, grp, cnt)])
                    dd = (b['win'] - a['win']) * 100
                    r.append('%s→%s (%+.0f)' % (pct(a['win']), pct(b['win']), dd))
            rows.append(r)
        out.append(table(['段階', '雑魚1体', '雑魚3体', 'ボス1体', 'ボス3体'], rows))
        out.append('')
        # 判定の変わった敵
        ch = []
        for sk in stages:
            for grp in ('zako', 'boss'):
                for cnt in (1, 3):
                    for (n, ra), (_, rb) in zip(req0[(sk, job, grp, cnt)], req1[(sk, job, grp, cnt)]):
                        if batch[ra]['v'] != batch[rb]['v']:
                            ch.append('%s %s%s%s %s→%s(勝率%s→%s)' % (sk, n, '(ボス)' if grp == 'boss' else '', '×3' if cnt == 3 else '',
                                                                      batch[ra]['v'], batch[rb]['v'], pct(batch[ra]['win']), pct(batch[rb]['win'])))
        out.append('判定が変わった組み合わせ（%d件）: %s\n' % (len(ch), '、'.join(ch[:40]) + (' …' if len(ch) > 40 else '') if ch else 'なし'))
    out.append('### (2) の差分（耐えられる秒数・最大HP）\n')
    M0, M1 = p2_0, p2_1
    for job in changed_jobs:
        rows = []
        for lv in lvs:
            a, b = M0[(lv, job, 'base')], M1[(lv, job, 'base')]
            rows.append(['Lv%d' % lv, '%d→%d' % (a['p']['hp'], b['p']['hp']), '%s→%s' % (fmt(a['endure'], 0), fmt(b['endure'], 0)),
                         '%s→%s' % (fmt(a['kill']), fmt(b['kill']))])
        out.append('**%s**\n' % job)
        out.append(table(['Lv', '最大HP', '耐えられる秒数', '雑魚1体を倒す秒'], rows))
        out.append('')
    return '\n'.join(out)


# ---------------------------------------------------------------- (4) 敵の特技を入れる前後
def skills_md(w, batch, reqA, reqB, stages):
    """reqA: 特技なし（前）、reqB: 特技あり（後）。判定が変わった敵と、無視した種類の一覧"""
    out = ['## (4) 敵の特技（攻撃魔法・回復）を入れる前と後\n']
    out.append('敵の特技（本番の magic 表の効果種別・基本値・射程）をシミュレータに入れた。くじは本番どおり（16 枠、HP30%以下で50%の回復優先、'
               '回復は HP70% 未満の時だけ・離れた所では使わない）、攻撃魔法は魔法防御0（標準プレイヤーは魔法防御の装備なし）で、敵は Mana を使わない。'
               '前＝特技なし（通常攻撃のみ）、後＝(1)(2) の数字と同じ。\n')
    # 特技を持つ敵・無視した種類
    seen, ign, notes, heal_n = {}, {}, set(), 0
    for sk in stages:
        for grp in ('zako', 'boss'):
            for e in w.enemies(sk, grp):
                col, ig, nt = enemyskill.skill_column(e)
                if col == '-' or e['name'] in seen:
                    continue
                seen[e['name']] = (sk, grp)
                for x in ig:
                    ign.setdefault(x, set()).add(e['name'])
                notes.update(nt)
                if any(m and m['type'] == 2 for m in e.get('skills') or []):
                    heal_n += 1
    out.append('特技を持つ敵（(1) に出る敵のうち）: %d 種（うち回復持ち %d 種）。\n' % (len(seen), heal_n))
    # 無視した種類は本番の敵全体から集める（(1) に出ない敵も含む）
    ign_all = {}
    for e in w.d['enemies']:
        for x in enemyskill.skill_column(e)[1]:
            ign_all.setdefault(x, set()).add(e['name'])
    out.append('**無視した種類**（シミュレータが効果を再現していない物。枠はくじで当たり、行動は使うが何も起きない）: %s。'
               '(1) に出る敵では %s\n'
               % ('、'.join('%s（%s）' % (k, '・'.join(sorted(v))) for k, v in sorted(ign_all.items())) if ign_all else '本番の敵の特技には無し',
                  '、'.join(sorted(ign)) if ign else '該当なし'))
    out.append('')
    out.append('そのほか未対応: 属性軽減（敵の耐性・特技の属性）、連携、状態異常、同じ群れの位置関係（回復の「仲間」は同じ群れの生きている敵が範囲内にいるとみなす）。'
               + ('置く魔法（炎壁・火炎放射・風衣など、残って何度も当たる型）は当たった 1 回分だけで数えている（過小評価）。' if notes else '') + '\n')
    rows, tot, same = [], 0, 0
    heal_rows = {}
    for sk in stages:
        for grp in ('zako', 'boss'):
            for job in w.jobs:
                for cnt in (1, 3):
                    for (n, ra), (_, rb) in zip(reqA[(sk, job, grp, cnt)], reqB[(sk, job, grp, cnt)]):
                        if n not in seen:
                            continue
                        tot += 1
                        a, b = batch[ra], batch[rb]
                        if a['v'] == b['v']:
                            same += 1
                        else:
                            rows.append([sk, n + ('(ボス格)' if grp == 'boss' else ''), '%d体' % cnt, job,
                                         '%s→%s' % (a['v'], b['v']), '%s→%s' % (pct(a['win']), pct(b['win'])),
                                         '%.1f' % b['em'], '%.1f' % b['eh']])
    out.append('特技を持つ敵 × 職業 × 1体/3体 の %d 組のうち、判定が変わったのは %d 組、変わらないのは %d 組。\n' % (tot, len(rows), same))
    if rows:
        out.append('判定が変わった組（勝率は 前→後。右 2 列は後の 1 戦あたりの敵の特技回数・うち回復回数）:\n')
        out.append(table(['段階', '敵', '体数', '職業', '判定 前→後', '勝率 前→後', '特技/戦', '回復/戦'], rows))
        out.append('')
    return '\n'.join(out)


# ---------------------------------------------------------------- 目標チェック（--goal-check）
def _scale_player(p, hp=1.0, df=1.0, at=1.0):
    q = dict(p)
    q['hp'] = max(1, int(round(p['hp'] * hp))); q['df'] = int(round(p['df'] * df))
    q['at'] = int(round(p['at'] * at)); q['aa'] = int(round(p['aa'] * at))
    return q


def _scale_enemy(e, pw=1.0, hp=1.0):
    return dict(e, pw=int(round(e['pw'] * pw)), hp=max(1, int(round(e['hp'] * hp))))


def _factors(cfg):
    """見立て用の 1 項目ずつの変更。(名前, 自分への変換, 敵への変換, 敵の特技を外すか)"""
    up, dn = cfg['sensitivity']['up'], cfg['sensitivity']['down']
    return [('HP×%g' % up, lambda p: _scale_player(p, hp=up), None, False),
            ('守備力×%g' % up, lambda p: _scale_player(p, df=up), None, False),
            ('攻撃力×%g' % up, lambda p: _scale_player(p, at=up), None, False),
            ('敵の攻撃力×%g' % dn, None, lambda e: _scale_enemy(e, pw=dn), False),
            ('敵のHP×%g' % dn, None, lambda e: _scale_enemy(e, hp=dn), False),
            ('敵の特技なし', None, None, True)]


def _factors_down(cfg):
    """（簡単すぎる所用）1 項目ずつ弱める／敵を強める"""
    up, dn = cfg['sensitivity']['up'], cfg['sensitivity']['down']
    return [('HP×%g' % dn, lambda p: _scale_player(p, hp=dn), None, False),
            ('守備力×%g' % dn, lambda p: _scale_player(p, df=dn), None, False),
            ('攻撃力×%g' % dn, lambda p: _scale_player(p, at=dn), None, False),
            ('敵の攻撃力×%g' % up, None, lambda e: _scale_enemy(e, pw=up), False),
            ('敵のHP×%g' % up, None, lambda e: _scale_enemy(e, hp=up), False)]


def _sens_rows(batch, ps, e, cnt, factors):
    """ps: 味方 dict のリスト。各変更後の行を登録して [(名前, 行)] を返す"""
    out = []
    for name, fp, fe, noskill in factors:
        qs = [fp(p) for p in ps] if fp else ps
        qe = fe(e) if fe else e
        if noskill and enemyskill.skill_column(e)[0] == '-':
            continue
        out.append((name, batch.add(scen_row(qs[0], qe, cnt, eskill=False if noskill else None, allies=qs[1:] or None))))
    return out


def _sens_text(batch, base_win, rows, improve=True, min_delta=.05):
    ds = sorted(((batch[r]['win'] - base_win) * (1 if improve else -1), n) for n, r in rows)
    ds.sort(reverse=True)
    top = [(d, n) for d, n in ds if d >= min_delta][:3]
    if not top:
        return '1項目だけ動かしても勝率は %d%%pt 以上は変わらない（複数の要因が重なっている）' % round(min_delta * 100)
    return '、'.join('%s で %+.0fpt' % (n, d * 100) for d, n in top)


def _facts_solo(w, sk, job, p, e):
    """自分のHP・守備・攻撃が他職の中央値の何%か、敵の特技の有無"""
    lv = w.chk[sk]['lv']
    others = [w.player(j, lv, sk) for j in w.jobs if j != job]
    def rel(k):
        m = median(o[k] for o in others)
        return (p[k] / m) if m else 1.0
    # 1 発の期待ダメージ（守備・攻撃の効き）は式の簡易版で比べる
    dmg_in = max(e['pw'] - p['df'] // 2, 0)
    dmg_in_o = median(max(e['pw'] - o['df'] // 2, 0) for o in others)
    dmg_out = max(p['at'] - e['st'] // 4, 0)
    dmg_out_o = median(max(o['at'] - e['st'] // 4, 0) for o in others)
    parts = ['敵HP%d・攻撃%d' % (e['hp'], e['pw']), 'HP%d（他職中央値の%d%%）' % (p['hp'], rel('hp') * 100),
             '敵の1発%d（他職中央値%d）' % (dmg_in, dmg_in_o),
             '自分の1発%d（他職中央値%d）' % (dmg_out, dmg_out_o)]
    if p['am'] or p['hm']:
        parts.append('特技: %s%s' % ('攻撃(Mana%d)' % p['am'] if p['am'] else '', '回復(Mana%d)' % p['hm'] if p['hm'] else ''))
    else:
        parts.append('特技なし')
    if enemyskill.skill_column(e)[0] != '-':
        parts.append('敵は特技あり')
    return '、'.join(parts)


def _facts_party(ps, e):
    hp = sum(p['hp'] for p in ps)
    dmg_in = [max(e['pw'] - p['df'] // 2, 0) for p in ps]
    dps_in = (sum(dmg_in) / len(ps)) / (BattleTurn(e) / 1000.0)      # 狙われる人は平均的とみなす
    dmg_out = sum(max(p['at'] - e['st'] // 4, 0) for p in ps)
    turn = lambda p: 1000 + (2000 - (2000 * max(p['dex'] * 100 // 255, 1)) // 100)
    dps_out = sum(max(p['at'] - e['st'] // 4, 0) / (turn(p) / 1000.0) for p in ps)
    wipe = hp / dps_in if dps_in > 0 else float('inf')
    kill = e['hp'] / dps_out if dps_out > 0 else float('inf')
    heal = sum(1 for p in ps if p['hm'])
    t = '全員のHP合計%d・敵の通常攻撃だけなら全滅まで約%.0f秒／通常攻撃だけで倒すまで約%.0f秒（敵HP%d）' % (hp, wipe, kill, e['hp'])
    if heal:
        t += '・回復役%d人' % heal
    if enemyskill.skill_column(e)[0] != '-':
        t += '・敵は特技あり'
    return t


def BattleTurn(e):
    """敵の行動間隔(ms)（BattleCalc::EnemyTurnTime と同じ）"""
    step = max(e['dex'] * 100 // 255, 1)
    return max(1000 + (4000 - 4000 * step // 100), 1)


def goal_check(d, w, batch, cfg, out_path, a):
    th1, thp, ths = cfg['zako_solo_win_min'], cfg['boss_party_win_min'], cfg['boss_solo_win_max']
    stages = [k for k in cfg['stages'] if k in w.chk]
    sw = lambda sk: cfg['stage_weight'].get(str(min(w.stage[sk]['order'], 6)), 1)
    comps = [(int(n), c) for n, cl in sorted(cfg['parties'].items()) for c in cl]
    fx = _factors(cfg); fxd = _factors_down(cfg)
    md_delta = cfg['sensitivity']['min_delta']
    # ---- 登録 ----
    z_req, b_solo, b_party = {}, {}, {}
    for sk in stages:
        lv = w.chk[sk]['lv']
        pl = {j: w.player(j, lv, sk) for j in w.jobs}
        for job in w.jobs:
            z_req[(sk, job)] = [(e, batch.add(scen_row(pl[job], e, 1))) for e in w.enemies(sk, 'zako')]
        for e in w.enemies(sk, 'boss'):
            for job in w.jobs:
                b_solo[(sk, e['name'], job)] = batch.add(scen_row(pl[job], e, 1))
            for n, c in comps:
                ps = [pl[j] for j in c]
                b_party[(sk, e['name'], tuple(c))] = batch.add(scen_row(ps[0], e, 1, allies=ps[1:]))
    batch.run()
    # ---- 目標1 ----
    g1 = []     # (重さ, 外れ幅, 段階, 職業, 最悪の敵, 失敗した敵数, 敵数)
    z_cell = {}
    for (sk, job), lst in z_req.items():
        ws = [(batch[r]['win'], e) for e, r in lst]
        if not ws:
            continue
        worst = min(ws, key=lambda x: x[0])
        mean_w = mean(x[0] for x in ws)
        bad = [x for x in ws if x[0] < th1]
        z_cell[(sk, job)] = (mean_w, worst, len(bad), len(ws))
        if bad:
            gap = th1 - worst[0]
            g1.append(dict(w=gap * 100 * sw(sk), gap=gap, sk=sk, job=job, e=worst[1], win=worst[0], nbad=len(bad), n=len(ws),
                           bads=sorted(bad, key=lambda x: x[0])))
    # ---- 目標3 ----
    g3 = []
    bosses = [(sk, e) for sk in stages for e in w.enemies(sk, 'boss')]
    for sk, e in bosses:
        solo = {job: batch[b_solo[(sk, e['name'], job)]]['win'] for job in w.jobs}
        pw = {(n, tuple(c)): batch[b_party[(sk, e['name'], tuple(c))]]['win'] for n, c in comps}
        for size in sorted({n for n, _ in comps}):
            sub = {k: v for k, v in pw.items() if k[0] == size}
            med = median(sub.values())
            if med < thp:
                gap = thp - med
                g3.append(dict(kind='party', size=size, w=gap * 100 * sw(sk), gap=gap, sk=sk, e=e, win=med, best=max(sub.values()),
                               solo=solo, pw=sub))
        for job, v in solo.items():
            if v > ths:
                gap = v - ths
                g3.append(dict(kind='solo', w=gap * 100 * sw(sk), gap=gap, sk=sk, e=e, job=job, win=v, solo=solo, pw=pw))
    # ---- 見立て（1項目ずつ動かす） ----
    for g in g1:
        lv = w.chk[g['sk']]['lv']
        p = w.player(g['job'], lv, g['sk'])
        g['facts'] = _facts_solo(w, g['sk'], g['job'], p, g['e'])
        g['sens'] = _sens_rows(batch, [p], g['e'], 1, fx)
    for g in g3:
        lv = w.chk[g['sk']]['lv']
        if g['kind'] == 'solo':
            p = w.player(g['job'], lv, g['sk'])
            g['facts'] = _facts_solo(w, g['sk'], g['job'], p, g['e'])
            g['sens'] = _sens_rows(batch, [p], g['e'], 1, fxd)
        else:
            mid = sorted(g['pw'].items(), key=lambda kv: kv[1])[len(g['pw']) // 2][0]
            g['mid'] = mid
            ps = [w.player(j, lv, g['sk']) for j in mid[1]]
            g['facts'] = _facts_party(ps, g['e'])
            g['sens'] = _sens_rows(batch, ps, g['e'], 1, fx)
    batch.run()
    for g in g1:
        g['why'] = _sens_text(batch, g['win'], g['sens'], True, md_delta)
    for g in g3:
        if g['kind'] == 'solo':
            g['why'] = _sens_text(batch, g['win'], g['sens'], False, md_delta)
        else:
            g['why'] = _sens_text(batch, g['pw'][g['mid']], g['sens'], True, md_delta)
    # ---- 書き出し ----
    cn = lambda c: '＋'.join(c)
    md = ['# SBO2003 目標チェック（goal-check）\n',
          'データ: %s（%s 時点の本番）／ battlesim（試行 %d 回・seed %d）／ 標準プレイヤーは段階の目安Lv・店売り最強装備（種・結晶なし）。\n'
          % (os.path.basename(a.simdata), d.get('at', '?'), a.runs, a.seed),
          '## 目標と判定ライン（tools/balance/goal.json）\n',
          '- 目標1: 雑魚1体は、全職業が1人で「普通に倒せる」。合格ライン＝**雑魚1体の勝率 %d%% 以上**（敵の種類ごと）。' % round(th1 * 100),
          '- 目標3: ボス格は2〜3人で挑む想定。**2人・3人それぞれの代表編成で勝率 %d%% 以上（人数ごとの編成の中央値）**、**1人なら %d%% 以下**（職業ごと。0%% は目標どおり）。'
          % (round(thp * 100), round(ths * 100)),
          '- 代表編成: ' + ' ／ '.join('%d人 %s' % (n, cn(c)) for n, c in comps) + '（先頭が前衛。全員 段階の目安Lv）。',
          '- シミュレータの簡易ルール（詳しくは README）: 敵は%s、回復特技の持ち主は仲間のHPが40%%以下で回復、連携130%%（%s）。回復道具・ジャスト・遠隔・状態異常は未対応。'
          % ('並び順の先頭（前衛）を狙い、倒れたら次の人' if batch.rule == 0 else '狙い直す時に一番HPが低い人を狙い',
             '2秒以内に別の人が同じ敵へ当てた時だけ' if batch.renkei else '無効'),
          '- 重さ ＝ 外れ幅(pt) × 段階の重み（%s）。序盤ほど重い。'
          % '、'.join('段階%s=%s' % (k, v) for k, v in sorted(cfg['stage_weight'].items())),
          '']
    # 表1
    md.append('## 目標1: 雑魚1体を1人で（勝率 平均／最低。%d%% 未満は **太字**）\n' % round(th1 * 100))
    rows = []
    for sk in stages:
        r = [w.chk[sk]['label'] + '(Lv%d)' % w.chk[sk]['lv']]
        for job in w.jobs:
            m, (ww, e), nb, n = z_cell[(sk, job)]
            c = '%s／%s' % (pct(m), pct(ww))
            r.append('**%s**' % c if nb else c)
        rows.append(r)
    md.append(table(['段階'] + w.jobs, rows))
    md.append('')
    md.append('外れ（%d 組 / %d 組）: ' % (len(g1), len(z_cell)) + ('、'.join('%s×%s' % (w.chk[g['sk']]['label'], g['job']) for g in sorted(g1, key=lambda g: -g['w'])) or 'なし') + '\n')
    # 表3
    md.append('## 目標3: ボス格を 1人／2人／3人で（勝率）\n')
    for sk, e in bosses:
        solo = next((g['solo'] for g in g3 if g['sk'] == sk and g['e'] is e), None)
        solo = {job: batch[b_solo[(sk, e['name'], job)]]['win'] for job in w.jobs}
        pw = {(n, tuple(c)): batch[b_party[(sk, e['name'], tuple(c))]]['win'] for n, c in comps}
        md.append('### %s %s（HP%d・攻撃%d）\n' % (w.chk[sk]['label'], e['name'], e['hp'], next((x['pw'] for x in d['enemies'] if x['name'] == e['name']), 0)))
        mk = lambda v, ok: v if ok else '**%s**' % v
        md.append('1人（目安 %d%% 以下）: ' % round(ths * 100) + '、'.join('%s %s' % (j, mk(pct(v), v <= ths)) for j, v in solo.items()) + '\n')
        rows = [['%d人' % n, cn(c), mk(pct(v), v >= thp)] for (n, c), v in pw.items()]
        md.append(table(['人数', '編成', '勝率（目安 %d%% 以上）' % round(thp * 100)], rows))
        for n in sorted({n for n, _ in comps}):
            md.append('\n%d人の中央値 %s' % (n, pct(median(v for (k, _), v in pw.items() if k == n))))
        md.append('')
    md.append('外れ（2人または3人の代表編成で勝てない %d 件、1人で簡単に勝てる 職×ボス %d 件）。\n' % (
        sum(1 for g in g3 if g['kind'] == 'party'), sum(1 for g in g3 if g['kind'] == 'solo')))
    # 直す候補リスト
    cand = [dict(g, k='目標1') for g in g1] + [dict(g, k='目標3') for g in g3]
    cand.sort(key=lambda g: -g['w'])
    md.append('## 直す候補リスト（重さ順）\n')
    md.append('重さ＝外れ幅(pt)×段階の重み。「見立て」は、その組の標準プレイヤー（編成は中央の勝率のもの）で 1 項目だけ動かした時の勝率の変化（%d%%pt 以上だけ、効く順）と、数字の事実。'
              '乱数のばらつき（±3pt 程度）があるので目安。\n' % round(md_delta * 100))
    from collections import Counter
    cnt = Counter()
    for g in cand:
        if g['k'] == '目標3' and g['kind'] == 'solo':
            continue
        first = g['why'].split(' で ')[0] if ' で ' in g['why'] else '（複合）'
        cnt[first.split('、')[0].split('×')[0]] += 1
    if cnt:
        md.append('見立ての 1 位に出た要因の数（不足側のみ）: ' + '、'.join('%s %d件' % kv for kv in cnt.most_common()) + '。\n')
    rows = []
    for i, g in enumerate(cand, 1):
        if g['k'] == '目標1':
            what = '%s×%s: 雑魚1体の勝率 最低%s（%s）。%d/%d 種が %d%% 未満' % (w.chk[g['sk']]['label'], g['job'], pct(g['win']), g['e']['name'], g['nbad'], g['n'], round(th1 * 100))
            gapt = '%.0fpt 不足' % (g['gap'] * 100)
        elif g['kind'] == 'party':
            what = '%s %s: %d人でも勝てない（代表編成の中央値%s、最良%s）' % (w.chk[g['sk']]['label'], g['e']['name'], g['size'], pct(g['win']), pct(g['best']))
            gapt = '%.0fpt 不足' % (g['gap'] * 100)
        else:
            what = '%s %s: %s が1人で勝率%s（簡単すぎ）' % (w.chk[g['sk']]['label'], g['e']['name'], g['job'], pct(g['win']))
            gapt = '%.0fpt 超過' % (g['gap'] * 100)
        why = g['why'] + '。' + g['facts']
        if g['k'] == '目標3' and g['kind'] == 'party':
            why = '（%s）' % cn(g['mid'][1]) + why
        elif g['k'] == '目標3':
            why = '（効いている要因: ' + g['why'] + '）。' + g['facts']
        rows.append([i, g['k'], '%.0f' % g['w'], what, gapt, why])
    md.append(table(['順', '目標', '重さ', '外れている所', '外れ幅', '見立て'], rows) if rows else '外れている所はありません。')
    md.append('')
    open(out_path, 'w', encoding='utf-8').write('\n'.join(md) + '\n')
    print('書き出し: %s（目標1の外れ %d 組、目標3の外れ %d 件）' % (out_path, len(g1), len(g3)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--simdata', default=os.path.join(WORK, 'simdata.json'))
    ap.add_argument('--out', default=os.path.join(WORK, 'report.md'))
    ap.add_argument('--runs', type=int, default=1000, help='1シナリオの試行回数')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--hp-override', default='', help='職業名:Lv=値,Lv=値,…（値は +N で加算、6-15=30 の範囲指定可）')
    ap.add_argument('--refresh', action='store_true', help='本番を読んで simdata.json を作り直す（SBO2003_AI_KEY が要る）')
    ap.add_argument('--only', choices=['1', '2'], help='(1) か (2) だけ出す')
    ap.add_argument('--no-enemy-skills', action='store_true', help='敵の特技を入れない（通常攻撃のみ。特技を入れる前の数字）')
    ap.add_argument('--exe', default='/tmp/battlesim')
    ap.add_argument('--goal-check', action='store_true', help='目標チェック（goal-check.md）だけを出す')
    ap.add_argument('--goal', default=os.path.join(HERE, 'goal.json'), help='目標チェックの基準（既定 tools/balance/goal.json）')
    ap.add_argument('--goal-out', default=os.path.join(WORK, 'goal-check.md'))
    ap.add_argument('--target-rule', type=int, choices=[0, 1], default=0, help='敵の狙い方 0=並び順の先頭（前衛）/1=一番HPが低い味方')
    ap.add_argument('--no-renkei', action='store_true', help='連携ボーナスを入れない')
    ap.add_argument('--detail', help='(1) の敵ごとの結果を TSV に書く')
    a = ap.parse_args()
    global ESKILL_ON
    ESKILL_ON = not a.no_enemy_skills

    if a.refresh:
        refresh(a.simdata)
    d = json.load(open(a.simdata, encoding='utf-8'))
    exe = build_exe(a.exe)
    hpo = parse_override(a.hp_override)
    w0 = World(d)
    w1 = World(d, hpo) if hpo else None
    stages = [c['key'] for c in d['checks']]
    lvs = list(range(1, 16))
    batch = Batch(exe, a.runs, a.seed, a.target_rule, 0 if a.no_renkei else 1)
    if a.goal_check:
        goal_check(d, w0, batch, json.load(open(a.goal, encoding='utf-8')), a.goal_out, a)
        return
    req1_0 = part1_requests(w0, batch, stages) if a.only != '2' else None
    has_sk = ESKILL_ON and any(enemyskill.skill_column(e)[0] != '-' for e in d['enemies'])
    req1_B = part1_requests(w0, batch, stages, eskill=False) if (req1_0 and has_sk) else None
    req2_0 = part2_requests(w0, batch, lvs) if a.only != '1' else None
    req1_1 = part1_requests(w1, batch, stages) if w1 and a.only != '2' else None
    req2_1 = part2_requests(w1, batch, lvs) if w1 and a.only != '1' else None
    batch.run()

    md = ['# SBO2003 バランスレポート\n',
          'データ: %s（%s 時点の本番）／ battlesim（試行 %d 回・seed %d）／ 標準プレイヤーは段階の目安Lv・店売り最強装備。\n'
          % (os.path.basename(a.simdata), d.get('at', '?'), a.runs, a.seed),
          '判定: 余裕=勝率95%以上かつ残HP50%以上 / 互角=80%以上 / きつい=40%以上 / 無理。シミュレータが未対応の物（回復道具、ジャスト、遠隔、連携、属性軽減、状態異常の特技）は入っていない。敵の特技（攻撃魔法・回復）は入っている（末尾の (4)）。'
          '「所要秒」は勝った時の平均で、敵に近づく時間を含む。判定内訳は敵ごとの判定の数。\n',
          '読み方の注意: ボス格は1人で挑む前提の数字（本来は複数人想定）。FES はお祭り用。'
          '種込み・結晶込みの列は上限値を最大HP・Manaに単純に足した上限の見積もり。\n']
    if a.only != '2':
        md.append('## (1) 段階×職業の勝率\n')
        md.append(part1_md(w0, batch, req1_0, stages))
    if a.only != '1':
        md.append('## (2) 職業別の成長比較（Lv1〜15）\n')
        body, M0 = part2_md(w0, batch, req2_0, lvs)
        md.append('### まとめ（魔法使いのHP）\n')
        md.append(part2_summary(w0, M0, lvs) + '\n')
        md.append(body)
    if w1:
        changed = sorted({j for j, _ in hpo}, key=w0.jobs.index)
        md.append('## (3) もし変えたら（--hp-override %s）\n' % a.hp_override)
        md.append('職業のLv別最大HPだけを一時的に書き換えて再計算。データは書き換えていない。変更後の最大HP: %s\n'
                  % '、'.join('%s Lv%d=%s' % (j, lv, ('%+d' % v if k == 'add' else v)) for (j, lv), (k, v) in sorted(hpo.items())))
        if a.only is None:
            md.append(diff_md(w0, w1, batch, stages, req1_0, req1_1,
                              {k: part2_metrics(batch, v) for k, v in req2_0.items()},
                              {k: part2_metrics(batch, v) for k, v in req2_1.items()}, lvs, changed))
    if req1_B:
        md.append(skills_md(w0, batch, req1_B, req1_0, stages))
    open(a.out, 'w', encoding='utf-8').write('\n'.join(md) + '\n')
    if a.detail and req1_0:
        with open(a.detail, 'w', encoding='utf-8') as f:
            f.write('段階\t職業\t群\t体数\t敵\t勝率\t残HP\t秒\t判定\n')
            for (sk, job, grp, cnt), lst in req1_0.items():
                for n, r in lst:
                    x = batch[r]
                    f.write('%s\t%s\t%s\t%d\t%s\t%.3f\t%.3f\t%.1f\t%s\n' % (sk, job, grp, cnt, n, x['win'], x['hp'], x['sec'], x['v']))
    print('書き出し: %s（シナリオ %d 件）' % (a.out, len(batch.rows)))


if __name__ == '__main__':
    main()
