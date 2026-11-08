# サーバー回帰テスト

性能改善の前後で、送信バイト列・キャラ状態・経路探索の結果が変わらないことを確かめる。

- 実行して golden と比べる: `tests/server/compare.sh`（「一致」と出れば OK。BENCH 行は計測値）
- 出力だけ見る: `tests/server/run.sh [出力ファイル]`
- golden を作り直す（挙動を意図して変えた時だけ）: `tests/server/run.sh /tmp/x.txt && grep -v -e '^BENCH' -e '^\[' /tmp/x.txt > tests/server/golden.txt`

中身（test_main.cpp）: A 送信(SendTo) / B 同画面内配信 / C myArray / D 経路探索(SEARCH2) / E キャラ巡回(TimerProc) / F 飛ぶ特技 / G すごろくの観戦参加 / H 他人に送るキャラ情報の項目消し / I 町転送の行き先帳。
時刻と乱数は固定、通信は mock_net.js、実データは非公開リポジトリ SBO2003-private の Release/SBODATA のコピー（元は書き換えない。探し方は tools/server/find-data.sh）。
