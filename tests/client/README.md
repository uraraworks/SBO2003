# クライアントの画素比較テスト

ブラウザ版クライアントの描画が、修正前と1画素も変わっていないことを確かめます。

```sh
cd tests/client
npm install                          # 初回のみ（playwright）
../../tools/web/build.sh release     # 比べたいビルド
node pixels.js ../../out/web/dist    # golden.json と比べる（一致なら終了コード 0）
node pixels.js <修正前のdist> --record   # golden.json を取り直す（描画を意図して変えた時だけ）
```

- ブラウザは入っている Google Chrome を使います（`PW_CHANNEL` で変更可）。
- 仮想時計（Playwright の clock）で時刻を固定し、タイトル・ログイン画面（縁取り文字・流れる文字）の
  canvas 全画素のハッシュを比べます。負荷で位相がずれることがあるため、不一致は最大3回まで再試行します。
- `?verify=1` を付けて開くので、毎フレーム「差分転送後の canvas ＝ 全画面転送の画素」と
  「定期描画を省いた画面が前と同じ画素」も検査します（失敗は `Module.sboVerifyFail` とコンソールの `[verify]`）。
- サーバー接続後のマップ画面は自動化していません（サーバー側の時刻・乱数で再現性が無いため）。
  手動で確認する時は `?verify=1` を付けて遊び、コンソールに `[verify]` が出ないことを見てください。
