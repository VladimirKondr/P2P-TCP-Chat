FROM ghcr.io/vladimirkondr/p2p-tcp-chat/base-image:main AS production_build
WORKDIR /home/runner/app

COPY . .
RUN bazel fetch //src:app
RUN bazel build //src:app


FROM ubuntu:22.04 AS app
RUN apt-get update && \
    apt-get install -y libpqxx-dev && \
    rm -rf /var/lib/apt/lists/*


COPY --from=production_build /home/runner/app/bazel-bin/src/app /app/app

EXPOSE 5050

ENTRYPOINT ["/app/app"]