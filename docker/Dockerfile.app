FROM ghcr.io/vladimirkondr/p2p-tcp-chat/base-image@sha256:4d191d0ccb385ab64f7196bd275a232c34443b43f5b2194c78c2d0b43125ee60 AS production_build
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