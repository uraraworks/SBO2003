# 図鑑 dex.json（build_dex.py の出力）と 本番の stTmp.json・magic.json から、シミュレータ用の simdata.json を作る（読むだけ）
# 使い方: python3 prep.py <dex.json> <fetch.py で落としたフォルダ> <出力 simdata.json>
# 敵ごとに skills（特技 16 枠。空き枠は None）を足す。中身は本番の magic 表の 効果種別・基本値・射程。
# 置く魔法（種別 4/7）は子の魔法を child に入れる（深さ 4 まで）。使い方は tools/balance/enemyskill.py
import json, sys
dex = json.load(open(sys.argv[1]))
st = {x['szName']: x for x in json.load(open(sys.argv[2] + '/stTmp.json'))}
mg = {x['id']: x for x in json.load(open(sys.argv[2] + '/magic.json'))}


def magic_rec(i, depth=0):
    m = mg.get(i)
    if m is None:
        return None
    r = {'id': m['id'], 'name': m['szName'], 'type': m['byType'], 'd1': m['wData1'], 'd2': m['wData2'],
         'd4': m['wData4'], 'tt': m['byTargetType'], 'tg': m['byTarget'], 'area': m['byTargetArea']}
    if m['byType'] in (4, 7) and depth < 4:
        r['child'] = magic_rec(m['wData1'], depth + 1)
    return r


for e in dex['enemies']:
    s = st.get(e['name'])
    if s:
        e['npcType'] = s['byNpcType']; e['moveX'] = s['byMoveProbabilityX']; e['moveY'] = s['byMoveProbabilityY']
        e['searchCx'] = s['bySearchCx']; e['magicIds'] = s['abyMagic']
        e['skills'] = [magic_rec(i) if i else None for i in s['abyMagic']]
json.dump(dex, open(sys.argv[3], 'w'), ensure_ascii=False)
