#!/usr/bin/env python3
"""JS 試作の simdata.json から battlesim 用のシナリオ（TSV）を作る。
使い方: python3 tools/balance/simdata2tsv.py simdata.json [段階=1] [体数=1] [開始距離] > scenarios.tsv
標準プレイヤーの作り方は JS 試作 sim.js の buildPlayer と同じ。"""
import json, sys

d = json.load(open(sys.argv[1], encoding='utf-8'))
stage_no = int(sys.argv[2]) if len(sys.argv) > 2 else 1
count = int(sys.argv[3]) if len(sys.argv) > 3 else 1
dist_arg = int(sys.argv[4]) if len(sys.argv) > 4 else None
key = 'S%d' % stage_no
stage = next(s for s in d['stages'] if s['key'] == key)
chk = next(c for c in d['checks'] if c['key'] == key)
so = min(stage['order'], 6)


def build(job):
    jt = next(j for j in d['jobTables'] if j['name'] == job)
    row = jt['rows'][chk['lv'] - 1]
    jid = next(j['id'] for j in d['jobs'] if j['name'] == job)

    def best(slot):
        c = [e for e in d['equips'] if e['slot'] == slot and jid in e['jobs'] and e.get('shopStage') is not None
             and e['shopStage'] <= so and e['kind'] == ('攻撃力' if slot == '武器' else '守備力')
             and (slot != '武器' or e['atk'] == '直接')]
        return sorted(c, key=lambda e: -e['eff'])[0] if c else None
    w = best('武器')
    armor = [a for a in (best(s) for s in ['鎧', '盾', '兜', '靴']) if a]

    def ok(m):
        return (m['kind'] == 'player' and jid in m['jobs'] and m.get('avg') and m['mana'] <= row['wMAXMANA']
                and any(b['stage'] is not None and b['stage'] <= so for b in m['books']))
    heals = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] == 2], key=lambda m: m['mana'])
    atks = sorted([m for m in d['magics'] if ok(m) and m['dmgType'] != 2 and '(HPを' not in m['effect'] and 'HPを' not in m['effect']],
                  key=lambda m: -m['avg'])
    h = heals[0] if heals else None
    a = atks[0] if atks else None
    return dict(lv=chk['lv'], hp=row['wMAXHP'], mana=row['wMAXMANA'], at=row['byPW'] + (w['val'] if w else 0),
                df=row['byST'] // 2 + sum(e['val'] for e in armor), dex=row['byDEX'], wmin=w['min'] if w else 0,
                hm=h['mana'] if h else 0, ha=int(h['avg']) if h else 0, ht=h['time'] if h else 0,
                am=a['mana'] if a else 0, aa=int(a['avg']) if a else 0, at_=a['time'] if a else 0,
                aisat=1 if a and a['isAT'] else 0)


print('# 名前\tLv\tHP\tMana\t攻撃力\t守備力\t素早さ\t武器byMin\t回復Mana\t回復量\t回復time\t攻撃特技Mana\t攻撃特技平均\t攻撃特技time\t攻撃特技がAT型\t敵HP\t敵攻撃力\t敵守備力\t敵素早さ\t体数\t開始距離\t敵が動く\t気づく範囲\t近づく確率')
for jn in [j['name'] for j in d['jobs']]:
    p = build(jn)
    for e in d['enemies']:
        if e['name'] not in chk['zako']:
            continue
        moves = 1 if e['npcType'] == 13 else 0
        dist = dist_arg if dist_arg is not None else (max(2, e['searchCx']) if moves else 3)
        # 敵の守備力は JS 試作と同じく st/2 を Damage に渡す
        cols = [jn + '/' + e['name'], p['lv'], p['hp'], p['mana'], p['at'], p['df'], p['dex'], p['wmin'],
                p['hm'], p['ha'], p['ht'], p['am'], p['aa'], p['at_'], p['aisat'],
                e['hp'], e['pw'], e['st'] // 2, e['dex'], count, dist, moves, e['searchCx'], e.get('moveX') or 20]
        print('\t'.join(str(c) for c in cols))
