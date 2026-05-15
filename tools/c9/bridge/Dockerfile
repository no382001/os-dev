FROM gcc:12-bullseye AS builder
WORKDIR /src
COPY proto.c ./
COPY include/ ./include/
COPY bridge/c9_bridge.c bridge/Makefile ./bridge/
RUN make -C bridge

FROM debian:bookworm-slim
COPY --from=builder /src/bridge/c9_bridge /c9_bridge
EXPOSE 7564
CMD ["/c9_bridge"]
