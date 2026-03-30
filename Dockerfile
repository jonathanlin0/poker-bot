# compile poker_server (discarded after build to keep the final image small)
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    build-essential \
    libsqlite3-dev \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/
COPY tests/ tests/

RUN cmake -B build && cmake --build build --target poker_server

# lightweight runtime image with only the compiled binary and frontend
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    locales \
    python3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && locale-gen en_US.UTF-8

ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

WORKDIR /app

COPY --from=builder /app/build/poker_server .
COPY frontend/ frontend/
COPY scripts/load_demo_data.py scripts/
COPY scripts/entrypoint.sh scripts/
RUN chmod +x scripts/entrypoint.sh

EXPOSE 9001

ENTRYPOINT ["scripts/entrypoint.sh"]
