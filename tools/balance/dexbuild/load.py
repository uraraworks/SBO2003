# 本番データを読み、入手経路などを組み立てる共通部分
import json,collections,re,os
P=os.environ.get('PROD','prod')
L=lambda k: json.load(open(f'{P}/{k}.json'))
I={x['id']:x for x in L('item')}; MG={x['id']:x for x in L('magic')}
MU={x['id']:x for x in L('magicUser')}; EU={x['id']:x for x in L('equUser')}
J=sorted(L('job'),key=lambda x:x['id']); ST={x['id']:x for x in L('stTmp')}
C=L('char'); M={x['id']:x for x in L('map')}; RC=L('recipe'); Q=L('quest'); MK={x['id']:x for x in L('maker')}
JOBN={j['id']:j['szName'] for j in J}
NONE=0xFFFFFFFF
ENEMY_TYPES={11,12,13,24}
def mname(i): return '%03d %s'%(i,M[i]['szName'] if i in M else '?')
# ---- お店（店員のいる店だけ「売っている」）
shop_src=collections.defaultdict(set); shop_noclerk=collections.defaultdict(set)
clerk=collections.defaultdict(set)
for c in C:
    s=c['szNpcSpeech'] or ''
    for mo in re.finditer(r'@5(\d\d)',s): clerk[c['dwMapID']].add(int(mo.group(1)))
for mid,m in M.items():
    for sh in m['shops']:
        for it in sh['items']:
            if not it: continue
            (shop_src if sh['no'] in clerk[mid] else shop_noclerk)[it].add(mid)
# ---- 敵の出る場所（発生地点＋決まった位置）
enemy_maps=collections.defaultdict(set); enemy_count=collections.Counter()
fixed_src=[]
for c in C:
    if c['dwParentTblID']!=NONE: continue
    t=c['dwStTmplateID']
    if c['byNpcType']==5 and t!=NONE and t in ST:
        enemy_maps[t].add(c['dwMapID']); enemy_count[t]+=max(1,c['byPutCount'])
    elif c['byNpcType'] in ENEMY_TYPES:
        if t!=NONE and t in ST: enemy_maps[t].add(c['dwMapID']); enemy_count[t]+=1
        else: fixed_src.append(c)
# ---- ドロップ
drop_src=collections.defaultdict(set)   # item -> 敵の名前
for t,s in ST.items():
    if not enemy_maps.get(t) or not s['byDropProbability']: continue
    for it in s['waItem']:
        if it: drop_src[it].add(s['szName'])
for c in fixed_src:
    if c['byDropProbability']:
        for it in c['waItem']:
            if it: drop_src[it].add(c['szName'] or '(名前なし)')
# ---- 合成・依頼・NPCからもらう
recipe_src=collections.defaultdict(list)
for r in RC:
    if r['byState']: recipe_src[r['wItem']].append(MK.get(r['byMaker'],{}).get('szName','作り手%d'%r['byMaker']))
quest_src=collections.defaultdict(list)
for q in Q:
    if q['byState'] and q['wRewardItem']: quest_src[q['wRewardItem']].append(q['szName'])
    for n in q['npcs']:
        if n.get('wItem'): quest_src[n['wItem']].append(q['szName']+'（依頼の品）')
give_src=collections.defaultdict(set)
for c in C:
    for mo in re.finditer(r'@4(\d{5})',c['szNpcSpeech'] or ''): give_src[int(mo.group(1))].add(c['dwMapID'])
def sources(it):
    r=[]
    if shop_src.get(it): r.append(('店','、'.join(M[m]['szName'] for m in sorted(shop_src[it]))))
    if drop_src.get(it): r.append(('ドロップ','、'.join(sorted(drop_src[it]))))
    if recipe_src.get(it): r.append(('合成','、'.join(recipe_src[it])))
    if quest_src.get(it): r.append(('依頼','、'.join(quest_src[it])))
    if give_src.get(it): r.append(('もらう','、'.join(M[m]['szName'] for m in sorted(give_src[it]))))
    if not r and shop_noclerk.get(it): r.append(('店員のいない店','、'.join(M[m]['szName'] for m in sorted(shop_noclerk[it]))))
    return r
