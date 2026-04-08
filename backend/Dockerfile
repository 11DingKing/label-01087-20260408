# 多阶段构建 - 支持 ARM64 和 AMD64
FROM --platform=$BUILDPLATFORM debian:bookworm-slim AS builder

# 安装编译工具（添加重试机制）
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    || (sleep 5 && apt-get update && apt-get install -y --no-install-recommends build-essential cmake)

# 复制源代码
WORKDIR /app
COPY . .

# 编译
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# 运行阶段
FROM debian:bookworm-slim AS runtime

# 安装运行时依赖
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends libstdc++6 \
    || (sleep 5 && apt-get update && apt-get install -y --no-install-recommends libstdc++6)

# 创建非root用户
RUN useradd -m -s /bin/bash appuser

WORKDIR /app

# 复制编译产物
COPY --from=builder /app/build/parking_system /app/

# 创建数据目录
RUN mkdir -p /app/data && chown -R appuser:appuser /app

USER appuser

# 数据卷
VOLUME ["/app/data"]

# 入口点
ENTRYPOINT ["/app/parking_system"]
