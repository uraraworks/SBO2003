# Sbo2003 ステージング用イメージ（サーバーの Node.js 版 + ブラウザ版クライアント）。
# ビルドコンテキストはリポジトリのルート（.dockerignore 参照）。
#
# サーバーもクライアントも wasm なので、CPU（x86_64 / arm64）に依存しない。
# メモリの少ないホストではビルドが重いので、普段は GitHub Actions で作って送る（deploy.sh --image）。

FROM emscripten/emsdk:6.0.10 AS build
WORKDIR /src
COPY . .
# フッターに出すコミット（.git はイメージに入れないので、ビルドの時に --build-arg で渡す）
ARG SBO2003_BUILD_COMMIT=
ENV SBO2003_BUILD_COMMIT=$SBO2003_BUILD_COMMIT
RUN tools/web/build.sh release \
	&& tools/server/build.sh release

FROM node:22-bookworm-slim
WORKDIR /srv/sbo2003
COPY --from=build /src/out/server-release/ ./server/
COPY --from=build /src/out/web/dist/ ./web/dist/
# 初回起動時にデータフォルダ（ボリューム）へコピーする元データ
COPY Release/SBODATA/ ./seed/SBODATA/
COPY Release/sbosvr.ini ./seed/SboSvr.ini
COPY deploy/staging/entrypoint.sh ./entrypoint.sh
RUN mkdir -p /srv/sbo2003/data && chown -R node:node /srv/sbo2003
USER node
# 遊んだデータ（SBODATA/ と SboSvr.ini）はここに保存される
VOLUME ["/srv/sbo2003/data"]
# HTTP（ブラウザ版の配布と /ws）。Windows 版用の TCP 2004 は外に出さない
EXPOSE 8080
# SIGTERM を受けるとデータを保存してから終わる（docker stop で安全に止まる）
STOPSIGNAL SIGTERM
ENTRYPOINT ["./entrypoint.sh"]
