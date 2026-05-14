FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config git ca-certificates \
    libcurl4-openssl-dev \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libmecab-dev mecab-ipadic-utf8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p generated/grpc_service/v1 \
    && protoc --proto_path=proto \
       --cpp_out=generated \
       --grpc_out=generated \
       --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
       grpc_service/v1/search.proto

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target search_server -j$(nproc)

FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 libgrpc++ libprotobuf32t64 \
    mecab libmecab2 mecab-ipadic-utf8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/search_server .
COPY data/stopwords_vi.txt data/stopwords_vi.txt

ENV INDEX_PATH=/data/index/index.bin
ENV STOPWORDS_PATH=data/stopwords_vi.txt
ENV SEARCH_PORT=50051

EXPOSE 50051

CMD ["./search_server"]
