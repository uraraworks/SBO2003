# 図鑑 dex.json（build_dex.py の出力）と 本番の stTmp.json から、シミュレータ用の simdata.json を作る（読むだけ）
# 使い方: python3 prep.py <dex.json> <fetch.py で落としたフォルダ> <出力 simdata.json>
import json, sys
dex = json.load(open(sys.argv[1]))
st = {x['szName']: x for x in json.load(open(sys.argv[2] + '/stTmp.json'))}
for e in dex['enemies']:
    s = st.get(e['name'])
    if s:
        e['npcType'] = s['byNpcType']; e['moveX'] = s['byMoveProbabilityX']; e['moveY'] = s['byMoveProbabilityY']
        e['searchCx'] = s['bySearchCx']; e['magicIds'] = s['abyMagic']
json.dump(dex, open(sys.argv[3], 'w'), ensure_ascii=False)
