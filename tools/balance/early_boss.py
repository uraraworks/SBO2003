#!/usr/bin/env python3
"""序盤の「高HPの敵」（段階1 プヨンクイーン／段階2 ぬめぬめ）が目標1（雑魚1体を全職が1人で勝率90%以上）を
満たさない問題の、改善案の比較表を作る（Markdown を標準出力へ）。データは書き換えない（simdata.json のコピーを
メモリ上で変えて battlesim を回すだけ）。

  python3 tools/balance/early_boss.py [--simdata simdata.json] [--runs 1000] [--seed 1] > early-bosses-tables.md

案:  (1) 敵のHP・攻撃を下げる（経験値は強さに比例して減らす）  (2) 同じ弱体で経験値は据え置き
     (3) 職業側を上げる（攻撃力・HP・特技ダメージ・Mana）  (4) 店売り装備・特技の入手を見直す
     (5) 何もしない（ボス格扱いにして目標1から外す。目標3＝ボス格の基準で確かめる）
report.py の World / Batch / scen_row をそのまま使う。
"""
import argparse, copy, json, os, sys
from statistics import median

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import report as R

WORK = '/mnt/project-files/_work/sbo2003-balance-sim'
JOBS = ['旅人', '戦士', '武道家', '僧侶', '魔法使い', '商人', '召喚士', '吟遊詩人']
SHORT = ['旅', '戦', '武', '僧', '魔', '商', '召', '吟']
JID = {n: i for i, n in enumerate(JOBS)}
TARGETS = {
    'queen': dict(sk='S1', lv=6, name='プヨンクイーン', boss=('S2', 8, '王様もぐら')),
    'numu': dict(sk='S2', lv=8, name='ぬめぬめ', boss=('S2', 8, '王様もぐら')),
}


# ---------------------------------------------------------------- 案の定義（simdata のコピーを書き換える関数）
def enemy_mod(name, hp=None, pw=None, exp=None):
    def f(d):
        e = next(x for x in d['enemies'] if x['name'] == name)
        if hp is not None: e['hp'] = hp
        if pw is not None: e['pw'] = pw
        if exp is not None: e['exp'] = exp
    return f


def job_mod(jobs, lvmax=8, hp=1.0, at=0, mana=1.0, spell=1.0):
    """職業のLv1〜lvmax の 最大HP×hp・力+at・最大Mana×mana、その職の（段階2までの本で覚える）攻撃特技ダメージ×spell"""
    ids = [JID[j] for j in jobs]

    def f(d):
        for jt in d['jobTables']:
            if jt['id'] in ids:
                for r in jt['rows'][:lvmax]:
                    r['wMAXHP'] = int(round(r['wMAXHP'] * hp)); r['byPW'] += at; r['wMAXMANA'] = int(round(r['wMAXMANA'] * mana))
        if spell != 1.0:
            for m in d['magics']:
                if m['kind'] == 'player' and m.get('avg') and set(m['jobs']) & set(ids) and m['dmgType'] != 2 \
                        and any(b['stage'] is not None and b['stage'] <= 2 for b in m['books']):
                    m['avg'] = m['avg'] * spell
    return f


def shop_mod(equip_jobs=None, weapons_stage2_to1=(), books=None):
    """equip_jobs: {装備名: [足す職業]}（装備できる職を増やす）／ weapons_stage2_to1: 職業のうち、段階2の店売り武器を段階1の店でも買えるようにする／
    books: {特技ID: 段階}（本の入手段階を前倒し）"""
    def f(d):
        for name, js in (equip_jobs or {}).items():
            for e in d['equips']:
                if e['name'] == name:
                    e['jobs'] = sorted(set(e['jobs']) | {JID[j] for j in js})
        ids = {JID[j] for j in weapons_stage2_to1}
        for e in d['equips']:
            if e['slot'] == '武器' and e.get('shopStage') == 2 and set(e['jobs']) & ids and e['atk'] == '直接':
                e['shopStage'] = 1
        for mid, st in (books or {}).items():
            for m in d['magics']:
                if m['id'] == mid:
                    for b in m['books']:
                        b['stage'] = st
    return f


def chain(*fs):
    def f(d):
        for g in fs:
            g(d)
    return f


# 6 = 召喚士の獣召喚の次（炎鬼召喚 83）、魔法使いの爆発 6
BOOK_EARLY = {83: 1, 6: 1}
JOB4 = ['僧侶', '魔法使い', '召喚士', '吟遊詩人']

VARIANTS = {
    'queen': [
        ('0 現状（＝5 何もしない）', 'HP300・攻7・経験値100', None),
        ('1x 攻撃だけ下げる 攻3（HP300のまま）', '経験値100', enemy_mod('プヨンクイーン', None, 3, None)),
        ('1a 弱体 HP100・攻5', '経験値は比例して34', enemy_mod('プヨンクイーン', 100, 5, 34)),
        ('1b 弱体 HP60・攻5', '経験値は比例して20', enemy_mod('プヨンクイーン', 60, 5, 20)),
        ('1c 弱体 HP30・攻5', '経験値は比例して10', enemy_mod('プヨンクイーン', 30, 5, 10)),
        ('2a 弱体 HP100・攻5・経験値据え置き', '経験値100', enemy_mod('プヨンクイーン', 100, 5, None)),
        ('2b 弱体 HP60・攻5・経験値据え置き', '経験値100', enemy_mod('プヨンクイーン', 60, 5, None)),
        ('2c 弱体 HP30・攻5・経験値据え置き', '経験値100', enemy_mod('プヨンクイーン', 30, 5, None)),
        ('3a 職業 小幅（僧魔召吟 Lv1-8: 力+3・HP×1.3・特技×1.5）', '', job_mod(JOB4, hp=1.3, at=3, spell=1.5)),
        ('3b 職業 大幅（同: 力+15・HP×3・特技×3・Mana×2）', '全職合格に必要な規模', job_mod(JOB4, hp=3.0, at=15, mana=2.0, spell=3.0)),
        ('4a 店: 召喚士が赤銅の杖・銀のナイフを装備可＋段階2の武器を段階1の店へ（僧魔召吟）', '',
         shop_mod({'赤銅の杖': ['召喚士'], '銀のナイフ': ['召喚士']}, JOB4)),
        ('4b 4a＋特技の本を前倒し（魔法使い 爆発・召喚士 炎鬼召喚 を段階1で）', '', chain(
            shop_mod({'赤銅の杖': ['召喚士'], '銀のナイフ': ['召喚士']}, JOB4, BOOK_EARLY))),
        ('5b ボス格を締める HP450・攻7（参考）', '経験値100', enemy_mod('プヨンクイーン', 450, None, None)),
        ('5c ボス格を締める HP600・攻7（参考）', '経験値100', enemy_mod('プヨンクイーン', 600, None, None)),
    ],
    'numu': [
        ('0 現状（＝5 何もしない）', 'HP200・攻15・経験値11', None),
        ('1a 弱体 HP150・攻12', '経験値は比例して8', enemy_mod('ぬめぬめ', 150, 12, 8)),
        ('1b 弱体 HP200・攻12（攻撃だけ）', '経験値据え置き', enemy_mod('ぬめぬめ', 200, 12, None)),
        ('1c 弱体 HP200・攻10（攻撃だけ）', '経験値据え置き', enemy_mod('ぬめぬめ', 200, 10, None)),
        ('1d 弱体 HP200・攻14（攻撃だけ）', '経験値据え置き', enemy_mod('ぬめぬめ', 200, 14, None)),
        ('1e 弱体 HP200・攻13（攻撃だけ）', '経験値据え置き', enemy_mod('ぬめぬめ', 200, 13, None)),
        ('2a 弱体 HP150・攻12・経験値据え置き', '経験値11', enemy_mod('ぬめぬめ', 150, 12, None)),
        ('2b 弱体 HP120・攻12・経験値据え置き', '経験値11', enemy_mod('ぬめぬめ', 120, 12, None)),
        ('3a 職業 小幅（僧魔召吟 Lv1-8: 力+3・HP×1.3・特技×1.5）', '', job_mod(JOB4, hp=1.3, at=3, spell=1.5)),
        ('3b 職業 大幅（同: 力+15・HP×3・特技×3・Mana×2）', '', job_mod(JOB4, hp=3.0, at=15, mana=2.0, spell=3.0)),
        ('4a 店: 召喚士が赤銅の杖・銀のナイフを装備可＋段階2の武器を段階1の店へ（僧魔召吟）', '',
         shop_mod({'赤銅の杖': ['召喚士'], '銀のナイフ': ['召喚士']}, JOB4)),
        ('4b 4a＋特技の本を前倒し（魔法使い 爆発・召喚士 炎鬼召喚 を段階1で）', '', shop_mod({'赤銅の杖': ['召喚士'], '銀のナイフ': ['召喚士']}, JOB4, BOOK_EARLY)),
    ],
}


# ---------------------------------------------------------------- 評価
def apply(d0, fn):
    d = copy.deepcopy(d0)
    if fn:
        fn(d)
    return d


class Eval:
    """1 つの案（d）について、必要な行を batch に登録して、後でまとめて引く"""

    def __init__(self, d, batch, goal, tgt):
        self.d, self.b, self.tgt = d, batch, tgt
        self.w = R.World(d)
        sk, lv, nm = tgt['sk'], tgt['lv'], tgt['name']
        self.e = next(x for x in d['enemies'] if x['name'] == nm)
        self.pl = {j: self.w.player(j, lv, sk) for j in JOBS}
        self.solo = {(j, c): batch.add(R.scen_row(self.pl[j], self.e, c)) for j in JOBS for c in (1, 3)}
        self.parties = {}
        for n, cl in goal['parties'].items():
            for c in cl:
                ps = [self.pl[j] for j in c]
                self.parties[(int(n), tuple(c))] = batch.add(R.scen_row(ps[0], self.e, 1, allies=ps[1:]))
        # 同じ段階の他の雑魚（倒す秒・副作用）
        self.others = [x for x in self.w.enemies(sk, 'zako') if x['name'] != nm]
        self.zk = {(j, x['name']): batch.add(R.scen_row(self.pl[j], x, 1)) for j in JOBS for x in self.others}
        # 段階2 の王様もぐら（強すぎの確認）
        bsk, blv, bn = tgt['boss']
        self.bsk, self.blv = bsk, blv
        self.be = next(x for x in d['enemies'] if x['name'] == bn)
        self.bpl = {j: self.w.player(j, blv, bsk) for j in JOBS}
        self.bsolo = {j: batch.add(R.scen_row(self.bpl[j], self.be, 1)) for j in JOBS}
        self.bparty = {}
        for n, cl in goal['parties'].items():
            for c in cl:
                ps = [self.bpl[j] for j in c]
                self.bparty[(int(n), tuple(c))] = batch.add(R.scen_row(ps[0], self.be, 1, allies=ps[1:]))

    # --- 取り出し
    def win(self, j, c):
        return self.b[self.solo[(j, c)]]['win']

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
        """今の Lv から次の Lv までの経験値"""
        rows = next(t for t in self.d['jobTables'] if t['name'] == j)['rows']
        lv = self.tgt['lv']
        return rows[lv]['exp'] - rows[lv - 1]['exp']


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
    for key, tgt in TARGETS.items():
        for label, note, fn in VARIANTS[key]:
            ev[(key, label)] = Eval(apply(d0, fn), batch, goal, tgt)
    batch.run()

    out = []
    for key, tgt in TARGETS.items():
        e0 = ev[(key, VARIANTS[key][0][0])].e
        out.append('### %s（%s Lv%d の標準プレイヤー）\n' % (tgt['name'], tgt['sk'], tgt['lv']))
        # 表A: 勝率（1体/3体）
        out.append('**表A 勝率%（1体／3体）**。1体が90%未満は太字。右端の「全職1体≥90」は目標1の合否。\n')
        rows = []
        for label, note, fn in VARIANTS[key]:
            x = ev[(key, label)]
            ok = all(x.win(j, 1) >= .9 for j in JOBS)
            rows.append([label] + [cell(x.win(j, 1), x.win(j, 3)) for j in JOBS] + ['○' if ok else '×'])
        out.append(R.table(['案'] + SHORT + ['全職1体≥90'], rows))
        out.append('')
        # 表B: ボス格として見た時（目標3）と強さ
        out.append('**表B ボス格の見方（目標3: 1人は40%以下、2人・3人の代表編成は中央値70%以上）**。1人>40%の職は旅戦武僧魔商召吟のうち何人か。\n')
        rows = []
        for label, note, fn in VARIANTS[key]:
            x = ev[(key, label)]
            over = [SHORT[i] for i, j in enumerate(JOBS) if x.win(j, 1) > .4]
            rows.append([label, pc(x.party_med(2)), pc(x.party_med(3)), '%d人（%s）' % (len(over), ''.join(over)) if over else '0人'])
        out.append(R.table(['案', '2人 中央値%', '3人 中央値%', '1人で40%超の職'], rows))
        out.append('')
        # 表C: 経験値効率
        out.append('**表C 経験値効率**。「Lv上げ1回に倒す体数」＝今のLvから次のLvまでの経験値÷敵の経験値（職業8つの中央値［最小〜最大］）。'
                   '「経験値/戦闘秒」は勝てる職の中央値（近づく時間込み）。「経験値/分（湧き込み）」は湧き間隔（%s）を足した 1 分あたり。\n'
                   % ('300秒' if key == 'queen' else '180秒'))
        rows = []
        respawn = 300 if key == 'queen' else 180
        for label, note, fn in VARIANTS[key]:
            x = ev[(key, label)]
            xp = x.e['exp']
            ks = [x.need_exp(j) / xp for j in JOBS]
            eps = [xp / x.b[x.solo[(j, 1)]]['sec'] for j in JOBS if x.win(j, 1) > .5 and x.b[x.solo[(j, 1)]]['sec'] > 0]
            secs = [x.b[x.solo[(j, 1)]]['sec'] for j in JOBS if x.win(j, 1) > .5]
            per_min = xp / (respawn + (median(secs) if secs else 0)) * 60
            rows.append([label, xp, '%.1f［%.1f〜%.1f］' % (median(ks), min(ks), max(ks)), '%.1f' % median(eps) if eps else '—', '%.1f' % per_min])
        xs = ev[(key, VARIANTS[key][0][0])]
        zk = xs.others
        zmed = median(z['exp'] for z in zk)
        zsec = median(xs.b[xs.zk[(j, z['name'])]]['sec'] for j in JOBS for z in zk if xs.b[xs.zk[(j, z['name'])]]['win'] > .5)
        rows.append(['（参考）同じ段階の雑魚の中央値', '%.1f' % zmed, '%.0f' % median(xs.need_exp(j) / zmed for j in JOBS), '%.2f' % (zmed / zsec), '湧きは多数（敵ごとに別）'])
        out.append(R.table(['案', '経験値', 'Lv上げ1回に倒す体数', '経験値/戦闘秒', '経験値/分（湧き込み）'], rows))
        out.append('')
        # 表D: 副作用（強すぎにならないか）
        out.append('**表D 他への副作用**。「雑魚を倒す秒」は同じ段階の他の雑魚（敵を変えた案でも変わらない）の中央値で、旅人・戦士・武道家（前衛）の平均を 1 とした比。'
                   '1 を大きく下回る職があれば強すぎ。「王様もぐら」は段階2の本物のボス格（HP900）で、1人40%以下・2〜3人70%以上が目標。\n')
        rows = []
        base = ev[(key, VARIANTS[key][0][0])]
        for label, note, fn in VARIANTS[key]:
            x = ev[(key, label)]
            front = [x.zako_kill_sec(j) for j in ('旅人', '戦士', '武道家')]
            ref = sum(front) / 3
            rat = ['%s %.2f' % (SHORT[JID[j]], x.zako_kill_sec(j) / ref) for j in JOB4]
            bo = [x.b[x.bsolo[j]]['win'] for j in JOBS]
            bover = [SHORT[i] for i, v in enumerate(bo) if v > .4]
            rows.append([label, ' / '.join(rat), '%s' % (''.join(bover) or 'なし'), pc(x.bparty_med(2)), pc(x.bparty_med(3)),
                         pc(min(x.zako_min(j) for j in JOBS))])
        out.append(R.table(['案', '雑魚を倒す秒の比（僧/魔/召/吟）', '王様もぐら 1人>40%の職', '王様もぐら 2人中央値%', '王様もぐら 3人中央値%', '他の雑魚の最低勝率%'], rows))
        out.append('')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
