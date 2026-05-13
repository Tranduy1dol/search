FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config git \
    libcurl4-openssl-dev \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target search_server -j$(nproc)

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 libgrpc++ libprotobuf32t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/search_server .
COPY data/stopwords_vi.txt data/stopwords_vi.txt

ENV INDEX_PATH=/data/index/index.bin
ENV STOPWORDS_PATH=data/stopwords_vi.txt
ENV SEARCH_PORT=50051

EXPOSE 50051

CMD ["./search_server"]
