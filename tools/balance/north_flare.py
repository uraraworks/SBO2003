#!/usr/bin/env python3
"""段階4 ノースフレアの敵「きらきら」（氷の塔アイスエッジ。HP200・攻撃20・守備(体力)200・魔法防御255・自分で回復）が
目標1（雑魚1体を全職が1人で勝率90%以上）を満たさない問題の、改善案の比較表を作る（Markdown を標準出力へ）。
データは書き換えない（simdata.json のコピーをメモリ上で変えて battlesim を回すだけ）。

  python3 tools/balance/north_flare.py [--simdata simdata.json] [--runs 1000] [--seed 1] > north-flare-tables.md

early_boss.py の案の作り方（敵の弱体・職業側・店・boss_override）を段階4 に当てはめたもの。
battlesim は「敵の魔法防御（dfm）」を見ない（プレイヤーの攻撃特技が敵の魔法防御で減る本番の式
SboSvrMagicProc.cpp の nTmp2 -= dfm/2 を入れていない）ので、この表では攻撃特技が「攻撃力＋値」型でない場合に
特技の平均ダメージから dfm/2 を引いて（0 未満は 0＝撃たない）から回す（きらきら＝魔法防御255 なら 127 引く）。
補正なし（battlesim そのまま）の現状も 1 行だけ載せる。
"""
import argparse, copy, json, os, sys
from statistics import median

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import report as R
import early_boss as EB

WORK = '/mnt/project-files/_work/sbo2003-balance-sim'
JOBS = EB.JOBS
SHORT = EB.SHORT
JID = EB.JID
JOB4 = EB.JOB4
NAME = 'きらきら'
SK, LV = 'S4', 10
RESPAWN, MAXN = 600, 2     # アイスエッジの発生: 1 か所・最大 2 体・600 秒ごと
CORRECT = True             # 敵の魔法防御を攻撃特技に反映する


# ---------------------------------------------------------------- 行の作り（魔法防御の補正つき）
def adj_player(p, e):
    p = dict(p)
    if CORRECT and e.get('dfm') and p['am'] and not p['aisat']:
        aa = p['aa'] - e['dfm'] // 2
        if aa <= 0:
            p['am'] = p['aa'] = p['at_'] = 0
        else:
            p['aa'] = aa
    return p


def row(p, e, n, allies=None):
    return R.scen_row(adj_player(p, e), e, n, allies=[adj_player(a, e) for a in allies] if allies else None)


# ---------------------------------------------------------------- 案の定義
def kk(d):
    return next(x for x in d['enemies'] if x['name'] == NAME)


def enemy_mod(hp=None, pw=None, st=None, dfm=None, exp=None, gold=None, heal_slots=None, heal_rng=None, boom_slots=None,
              boom_rng=None, dex=None):
    """heal_slots/boom_slots: 回復（全体）／爆発（敵）の枠の数（先頭から残す）。heal_rng/boom_rng: 基本値 (d1,d2)"""
    def f(d):
        e = kk(d)
        if hp is not None: e['hp'] = hp
        if pw is not None: e['pw'] = pw
        if st is not None: e['st'] = st; e['df'] = st // 2
        if dfm is not None: e['dfm'] = dfm
        if exp is not None: e['exp'] = exp
        if gold is not None: e['gold'] = gold
        if dex is not None: e['dex'] = dex
        for kind, n, rng in ((2, heal_slots, heal_rng), (1, boom_slots, boom_rng)):
            seen = 0
            for i, m in enumerate(e['skills']):
                if m and m['type'] == kind:
                    seen += 1
                    if n is not None and seen > n:
                        e['skills'][i] = None
                    elif rng:
                        m['d1'], m['d2'] = rng
    return f


def weapon_mod(plus, from_stage=4):
    """各職の店売り最強武器（段階 from_stage 以下）の 1 つ上に「攻撃力+plus」の新武器を段階 from_stage の店に足す（仮）"""
    def f(d):
        new = []
        for jid in range(8):
            c = [e for e in d['equips'] if e['slot'] == '武器' and jid in e['jobs'] and e.get('shopStage') is not None
                 and e['shopStage'] <= from_stage and e['kind'] == '攻撃力' and e['atk'] == '直接']
            if not c:
                continue
            b = sorted(c, key=lambda e: -e['eff'])[0]
            n = dict(b); n['name'] = b['name'] + '＋%d(仮)' % plus; n['val'] = b['val'] + plus; n['eff'] = b['eff'] + plus
            n['jobs'] = [jid]; n['shopStage'] = from_stage
            new.append(n)
        d['equips'].extend(new)
    return f


JOB_ALL = list(JOBS)
REC = ('D1', '3o', '3p', '3q', '3r', '3s', '3t', '3j', '3m', '3f')
V = [
    # ---- 診断（原因の切り分け。案ではない）
    ('D0 現状（battlesim そのまま＝魔法防御を見ない）', 'raw', None),
    ('D1 現状（攻撃特技に魔法防御を反映）', '', None),
    ('D2 敵の特技なし（回復も爆発も）', '', enemy_mod(heal_slots=0, boom_slots=0)),
    ('D3 回復だけなし（爆発4枠は残す）', '', enemy_mod(heal_slots=0)),
    ('D4 爆発だけなし（回復2枠は残す）', '', enemy_mod(boom_slots=0)),
    ('D5 守備 200→100 だけ', '', enemy_mod(st=100)),
    ('D6 HP 200→100 だけ', '', enemy_mod(hp=100)),
    ('D7 素早さ 200→40 だけ（行動間隔 約1.9秒→約4.4秒）', '', enemy_mod(dex=40)),
    # ---- 案1 敵の特技
    ('1a 回復の枠 2→1', '', enemy_mod(heal_slots=1)),
    ('1b 回復量 1〜20 → 1〜5（枠2のまま）', '', enemy_mod(heal_rng=(1, 5))),
    ('1c 回復をなくす（爆発4枠のみ）', '', enemy_mod(heal_slots=0)),
    ('1d 爆発も弱める 4〜8 → 2〜4（回復は現状）', '', enemy_mod(boom_rng=(2, 4))),
    # ---- 案2 敵の守備・HP
    ('2a 守備 200→100', '', enemy_mod(st=100)),
    ('2b 守備 200→60', '', enemy_mod(st=60)),
    ('2c 守備 200→35（段階4の他の敵並み）', '', enemy_mod(st=35)),
    ('2d 守備 200→60・魔法防御 255→100', '', enemy_mod(st=60, dfm=100)),
    # ---- 案3 組み合わせ
    ('3a 回復なし＋守備100', '', enemy_mod(st=100, heal_slots=0)),
    ('3b 回復なし＋守備60', '', enemy_mod(st=60, heal_slots=0)),
    ('3c 回復なし＋守備60＋HP120', '', enemy_mod(st=60, hp=120, heal_slots=0)),
    ('3d 回復1枠・量1〜10＋守備60', '', enemy_mod(st=60, heal_slots=1, heal_rng=(1, 10))),
    ('3e 回復1枠・量1〜10＋守備100', '', enemy_mod(st=100, heal_slots=1, heal_rng=(1, 10))),
    ('3f 回復なし＋守備40＋HP100', '', enemy_mod(st=40, hp=100, heal_slots=0)),
    ('3g 回復なし＋守備40＋HP120', '', enemy_mod(st=40, hp=120, heal_slots=0)),
    ('3h 回復なし＋守備50＋HP100', '', enemy_mod(st=50, hp=100, heal_slots=0)),
    ('3i 守備40＋HP100（回復は現状）', '', enemy_mod(st=40, hp=100)),
    ('3j 回復1枠・量1〜10＋守備40＋HP100', '', enemy_mod(st=40, hp=100, heal_slots=1, heal_rng=(1, 10))),
    ('3k 回復なし＋守備60＋素早さ40', '', enemy_mod(st=60, dex=40, heal_slots=0)),
    ('3l 回復なし＋守備100＋素早さ40', '', enemy_mod(st=100, dex=40, heal_slots=0)),
    ('3m 回復なし＋守備60＋HP120＋素早さ100', '', enemy_mod(st=60, hp=120, dex=100, heal_slots=0)),
    ('3o 守備40＋素早さ40（他は現状）', '', enemy_mod(st=40, dex=40)),
    ('3p 守備60＋素早さ40（他は現状）', '', enemy_mod(st=60, dex=40)),
    ('3q 守備40＋素早さ40＋回復1枠・量1〜10', '', enemy_mod(st=40, dex=40, heal_slots=1, heal_rng=(1, 10))),
    ('3r 守備40＋素早さ40＋HP150', '', enemy_mod(st=40, dex=40, hp=150)),
    ('3s 守備40＋素早さ60（他は現状）', '', enemy_mod(st=40, dex=60)),
    ('3t 守備50＋素早さ40＋HP150', '', enemy_mod(st=50, dex=40, hp=150)),
    ('3n 3o＋経験値 100→60（お金・ドロップは据え置き）', '', enemy_mod(st=40, dex=40, exp=60)),
    # ---- 案4 職業側
    ('4a 職業 全職の力 +20（Lv1〜10）', '', EB.job_mod(JOB_ALL, lvmax=10, at=20, spell=1.0)),
    ('4b 職業 全職の力 +40（Lv1〜10）', '', EB.job_mod(JOB_ALL, lvmax=10, at=40, spell=1.0)),
    ('4c 店 段階4の店に各職の最強武器＋20 を足す', '', weapon_mod(20)),
    ('4d 店 段階4の店に各職の最強武器＋40 を足す', '', weapon_mod(40)),
]
KEYS = [v[0] for v in V]


# ---------------------------------------------------------------- 評価
def apply(d0, fn):
    d = copy.deepcopy(d0)
    if fn:
        fn(d)
    return d


class Eval:
    def __init__(self, d, batch, goal, raw):
        global CORRECT
        CORRECT = not raw
        self.d, self.b = d, batch
        self.w = R.World(d)
        self.e = kk(d)
        self.pl = {j: self.w.player(j, LV, SK) for j in JOBS}
        self.solo = {(j, c): batch.add(row(self.pl[j], self.e, c)) for j in JOBS for c in (1, 2, 3)}
        self.parties = {}
        for n, cl in goal['parties'].items():
            for c in cl:
                ps = [self.pl[j] for j in c]
                self.parties[(int(n), tuple(c))] = batch.add(row(ps[0], self.e, 1, allies=ps[1:]))
        self.others = [x for x in self.w.enemies(SK, 'zako') if x['name'] != NAME]
        self.zk = {(j, x['name']): batch.add(row(self.pl[j], x, 1)) for j in JOBS for x in self.others}
        self.boss = self.w.enemies(SK, 'boss')[0]
        self.bsolo = {j: batch.add(row(self.pl[j], self.boss, 1)) for j in JOBS}
        self.bparty = {}
        for n, cl in goal['parties'].items():
            for c in cl:
                ps = [self.pl[j] for j in c]
                self.bparty[(int(n), tuple(c))] = batch.add(row(ps[0], self.boss, 1, allies=ps[1:]))
        CORRECT = True

    def win(self, j, c=1):
        return self.b[self.solo[(j, c)]]['win']

    def sec(self, j, c=1):
        return self.b[self.solo[(j, c)]]['sec']

    def party_med(self, n):
        return median(self.b[r]['win'] for (k, _), r in self.parties.items() if k == n)

    def bparty_med(self, n):
        return median(self.b[r]['win'] for (k, _), r in self.bparty.items() if k == n)

    def zako_kill_sec(self, j):
        xs = [self.b[self.zk[(j, x['name'])]] for x in self.others]
        xs = [x['sec'] for x in xs if x['win'] > .5]
        return median(xs) if xs else None

    def zako_min(self, j):
        return min(self.b[self.zk[(j, x['name'])]]['win'] for x in self.others)

    def need_exp(self, j):
        rows = next(t for t in self.d['jobTables'] if t['name'] == j)['rows']
        return rows[LV]['exp'] - rows[LV - 1]['exp']


def pc(x):
    return '%.0f' % (x * 100)


def cell(a, b, th=.9):
    s = '%s/%s' % (pc(a), pc(b))
    return '**%s**' % s if a < th else s


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--simdata', default=os.path.join(WORK, 'simdata.json'))
    ap.add_argument('--goal', default=os.path.join(HERE, 'goal.json'))
    ap.add_argument('--runs', type=int, default=1000)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--exe', default='/tmp/battlesim')
    a = ap.parse_args()
    d0 = json.load(open(a.simdata, encoding='utf-8'))
    goal = json.load(open(a.goal, encoding='utf-8'))
    batch = R.Batch(R.build_exe(a.exe), a.runs, a.seed)
    ev = {}
    for label, note, fn in V:
        ev[label] = Eval(apply(d0, fn), batch, goal, note == 'raw')
    batch.run()
    out = []

    out.append('**表A 勝率%（1体／3体）**。1体が90%未満は太字。右端の「全職1体≥90」は目標1の合否、「2体」は最大2体湧く実際の最大（8職の最低勝率）。\n')
    rows = []
    for label, note, fn in V:
        x = ev[label]
        ok = all(x.win(j) >= .9 for j in JOBS)
        rows.append([label] + [cell(x.win(j), x.win(j, 3)) for j in JOBS] + ['○' if ok else '×', pc(min(x.win(j, 2) for j in JOBS))])
    out.append(R.table(['案'] + SHORT + ['全職1体≥90', '2体の最低'], rows))
    out.append('')

    out.append('**表B ボス格の見方（boss_override した時）**。目標3＝1人は40%以下・2人・3人の代表編成は中央値70%以上。\n')
    rows = []
    for label, note, fn in V:
        x = ev[label]
        over = [SHORT[i] for i, j in enumerate(JOBS) if x.win(j) > .4]
        rows.append([label, pc(x.party_med(2)), pc(x.party_med(3)), '%d人（%s）' % (len(over), ''.join(over)) if over else '0人'])
    out.append(R.table(['案', '2人 中央値%', '3人 中央値%', '1人で40%超の職'], rows))
    out.append('')

    out.append('**表C 経験値・お金の効率**。「Lv上げ1回に倒す体数」＝Lv10→11 の経験値÷敵の経験値（8職の中央値［最小〜最大］）。'
               '「経験値/戦闘秒」は勝てる職の中央値（近づく時間込み）。「経験値/分（湧き込み）」は 1 か所・最大2体・湧き600秒として、'
               '2体を倒す所要（勝てる職の中央値の秒×2）を足した 1 分あたり。勝てる職が無ければ —。\n')
    rows = []
    for label, note, fn in V:
        x = ev[label]
        xp = x.e['exp']
        ks = [x.need_exp(j) / xp for j in JOBS]
        wins = [j for j in JOBS if x.win(j) > .5 and x.sec(j) > 0]
        eps = [xp / x.sec(j) for j in wins]
        secs = [x.sec(j) for j in wins]
        per_min = MAXN * xp / (RESPAWN + (median(secs) * MAXN if secs else 0)) * 60 if secs else None
        rows.append([label, xp, x.e['gold'], '%.1f［%.1f〜%.1f］' % (median(ks), min(ks), max(ks)),
                     '%.1f' % median(eps) if eps else '—', '%.1f' % per_min if per_min else '—'])
    xs = ev[V[1][0]]
    zk = xs.others
    zmed = median(z['exp'] for z in zk)
    zsec = median(xs.b[xs.zk[(j, z['name'])]]['sec'] for j in JOBS for z in zk if xs.b[xs.zk[(j, z['name'])]]['win'] > .5)
    rows.append(['（参考）段階4の他の雑魚の中央値', '%.1f' % zmed, '%.0f' % median(z['gold'] for z in zk),
                 '%.0f' % median(xs.need_exp(j) / zmed for j in JOBS), '%.2f' % (zmed / zsec), '湧きは多数（敵ごとに別）'])
    out.append(R.table(['案', '経験値', 'お金', 'Lv上げ1回に倒す体数', '経験値/戦闘秒', '経験値/分（湧き込み・2体）'], rows))
    out.append('')

    out.append('**表D 他への副作用**。「雑魚を倒す秒」は段階4の他の雑魚（きらきらを除く8種）の中央値で、旅人・戦士・武道家（前衛）の平均を 1 とした比。'
               '敵だけを変える案は他の敵に触らないので現状（D1）と同じ数字になるため省いた。1 を大きく下回る職があれば強すぎ。「アイスメイデン」は段階4 の本物のボス格（HP2500）で、1人40%以下・2〜3人70%以上が目標。\n')
    rows = []
    for label, note, fn in V:
        if not (label.startswith('4') or label.startswith('D1')):
            continue    # 敵だけを変える案は他の敵・アイスメイデンに影響しない（D1 と同じ数字）
        x = ev[label]
        front = [x.zako_kill_sec(j) for j in ('旅人', '戦士', '武道家')]
        ref = sum(front) / 3
        rat = ['%s %.2f' % (SHORT[JID[j]], x.zako_kill_sec(j) / ref) if x.zako_kill_sec(j) else '%s —' % SHORT[JID[j]] for j in JOB4]
        bo = [x.b[x.bsolo[j]]['win'] for j in JOBS]
        bover = [SHORT[i] for i, v in enumerate(bo) if v > .4]
        rows.append([label, ' / '.join(rat), ''.join(bover) or 'なし', pc(x.bparty_med(2)), pc(x.bparty_med(3)),
                     ' '.join('%s%s' % (SHORT[JID[j]], pc(x.zako_min(j))) for j in JOBS)])
    out.append(R.table(['案', '雑魚を倒す秒の比（僧/魔/召/吟）', 'アイスメイデン 1人>40%の職', 'アイスメイデン 2人中央値%', '同 3人中央値%',
                        '段階4の他の雑魚の最低勝率%（職別）'], rows))
    out.append('')

    # 有力案の 1体/2体/3体
    out.append('**表G 有力案の 1体／2体／3体の勝率%**（最大2体湧く所なので 2体も見る）。\n')
    rows = []
    for label, note, fn in V:
        if label[:2] in REC:
            x = ev[label]
            rows.append([label] + ['%s/%s/%s' % (pc(x.win(j, 1)), pc(x.win(j, 2)), pc(x.win(j, 3))) for j in JOBS])
    out.append(R.table(['案'] + SHORT, rows))
    out.append('')

    # 段階4 の他の雑魚 × 職（現状）
    x = ev[V[1][0]]
    out.append('**表E 段階4の他の雑魚の勝率%（現状・1体・Lv10 標準プレイヤー）**。きらきら以外の失敗の有無を見る。\n')
    rows = []
    for z in x.others:
        rows.append([z['name'], 'HP%d 攻%d 守%d' % (z['hp'], z['pw'], z['st'])] +
                    [('**%s**' % pc(x.b[x.zk[(j, z['name'])]]['win'])) if x.b[x.zk[(j, z['name'])]]['win'] < .9 else pc(x.b[x.zk[(j, z['name'])]]['win'])
                     for j in JOBS])
    out.append(R.table(['敵', '強さ'] + SHORT, rows))
    out.append('')

    # 標準プレイヤー（きらきらに対する数字）
    out.append('**表F 標準プレイヤー（Lv10・段階4の店売り最強装備）と、きらきらへの 1 発**。1発＝通常攻撃の平均（守備 st/4 を引いた後）。\n')
    rows = []
    e = x.e
    for j in JOBS:
        p = x.pl[j]
        pa = adj_player(p, e)
        hit = max(0, p['at'] - p['wmin'] // 2 * 0 - e['st'] // 4)
        rows.append([j, p['hp'], p['at'], p['df'], hit,
                     '%s(Mana%d・平均%d→魔防後%d%s)' % ('攻撃力＋値型' if p['aisat'] else '魔法型', p['am'], p['aa'], pa['aa'], '' if pa['aa'] else '・撃てない') if p['am'] else 'なし',
                     'あり(Mana%d)' % p['hm'] if p['hm'] else 'なし'])
    out.append(R.table(['職', 'HP', '攻撃力', '守備', '通常1発(目安)', '攻撃特技', '回復特技'], rows))
    print('\n'.join(out))


if __name__ == '__main__':
    main()
