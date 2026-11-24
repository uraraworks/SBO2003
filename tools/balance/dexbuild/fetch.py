import os, json, sys, urllib.request, time
S='https://sbo2003.urara-works.jp'; K=os.environ['SBO2003_AI_KEY']
OUT=sys.argv[1]
def call(path,body):
    for t in range(4):
        try:
            r=urllib.request.Request(S+path,data=json.dumps(body).encode(),headers={'Authorization':'Bearer '+K,'Content-Type':'application/json'})
            return json.load(urllib.request.urlopen(r,timeout=60))
        except Exception as e:
            print('retry',path,e,file=sys.stderr); time.sleep(2**t)
    raise SystemExit('fail '+path)
def listall(kind):
    recs=[];off=0
    while True:
        b={'kind':kind,'offset':off,'limit':500}
        if kind=='char': b['match']={'isPlayer':False}
        time.sleep(0.5)
        r=call('/api/data/list',b)
        recs+=r['records']; off+=r['count']
        if not r.get('more') or r['count']==0: break
    return recs
for k in ['map','char','item','stTmp','magic','magicUser','equUser','job','recipe','quest','maker']:
    d=listall(k); json.dump(d,open(f'{OUT}/{k}.json','w'),ensure_ascii=False); print(k,len(d))
json.dump(call('/api/data/schema',{}),open(f'{OUT}/schema.json','w'),ensure_ascii=False)
