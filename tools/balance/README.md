# バランステスト用シミュレータ（PR2 の最初の一歩）

サーバー依存のない戦闘ループ `common/BattleSim.h`（header-only。式は `common/BattleCalc.h`）と、それを呼ぶ native CLI `battlesim.cpp`。emscripten 不要。

## ビルドと実行
```
g++ -std=c++11 -O2 -I. tools/balance/battlesim.cpp -o /tmp/battlesim
/tmp/battlesim [seed=1] [試行回数=1000] < scenarios.tsv
```
出力（タブ区切り、1行目はヘッダ）: 名前 / 勝率% / 残HP%（勝った時の平均）/ 所要秒（勝った時の平均）/ 回復回数（全試行の平均）/ 判定。
判定: 余裕=勝率95%以上かつ残HP50%以上 / 互角=80%以上 / きつい=40%以上 / 無理。

## 入力（1行1シナリオ、タブ区切り24列、`#` で始まる行は無視）
| 列 | 内容 |
|---|---|
| 1 | 名前（表示用） |
| 2-15 | プレイヤー: Lv, 最大HP, 最大Mana, 攻撃力（力など＋武器）, 守備力（Damage に渡す値）, 素早さ, 武器byMin, 回復特技Mana（0=無し）, 回復量, 回復time, 攻撃特技Mana（0=無し）, 攻撃特技の平均ダメージ, 攻撃特技time, 攻撃特技が攻撃力＋値型か（1/0） |
| 16-24 | 敵: HP, 攻撃力, 守備力（Damage に渡す値）, 素早さ, 体数（最大4）, 開始距離（マス）, 敵が動くか（1=敵が気づいて近づく/0=動かずプレイヤーが歩く）, 気づく範囲, 近づく確率% |

方針: 満タン周回で通常攻撃（空振り5%・会心2%）、HP40%以下で回復特技、Manaがあれば攻撃特技。敵の特技・回復、回復道具、ジャスト、遠隔、連携、状態異常は未対応。

## 注意
- 乱数は `std::mt19937`（seed 付き）を、0 以上の `long` を返す関数オブジェクトで包んで BattleCalc に渡している。`unsigned` のまま渡すとマイナスのダメージ計算が符号なしになって壊れる（サーバーの genrand は符号付き）。
- ゲージ時間・敵の行動間隔は `BattleCalc::GaugeTime` / `EnemyTurnTime`。SboSvr/SboCli はまだ旧実装のまま（置き換えは golden に影響するので別PR）。

## JS 試作との突き合わせ
```
python3 tools/balance/simdata2tsv.py simdata.json [段階=1] [体数=1] [開始距離] > /tmp/sc.tsv
/tmp/battlesim 1 20000 < /tmp/sc.tsv
node sim.js --stage 1 --n 20000     # JS 試作側（sbo2003-balance-sim）
```
JS 試作は `r()*100 > 66`（34%）、会心でも武器の減少値を引く点が BattleCalc（33%・会心は引かない）と違う。この2点を JS 側で揃えると段階1の雑魚88件の勝率差は最大0.3%。
