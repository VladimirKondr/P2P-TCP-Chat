FROM ghcr.io/vladimirkondr/p2p-tcp-chat/base-image:main AS production_build
WORKDIR /home/runner/app

COPY . .
RUN bazel fetch //src:app
RUN bazel build //src:app


FROM ubuntu:22.04 AS app
COPY --from=production_build /app/bazel-bin/src/app /app/app

EXPOSE 5050

ENTRYPOINT ["/app/app"]