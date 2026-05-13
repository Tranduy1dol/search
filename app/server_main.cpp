#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "search/indexer/disk_index.h"
#include "search/searcher/searcher.h"
#include "server/search_service.h"

std::unique_ptr<grpc::Server> server;

void SignalHandler(int signal) {
  if (server) {
    std::cout << "\nShutting down...\n";
    server->Shutdown();
  }
}

int main(int argc, char* argv[]) {
  std::string index_path = "data/index/index.bin";
  std::string stopword_path = "data/stopword_vi.txt";
  std::string port = "50051";

  if (const char* env = std::getenv("INDEX_PATH")) {
    index_path = env;
  }
  if (const char* env = std::getenv("STOPWORD_PATH")) {
    stopword_path = env;
  }
  if (const char* env = std::getenv("SEARCH_PORT")) {
    port = env;
  }

  auto disk_index = std::make_unique<search::DiskIndex>(index_path);
  std::cout << "loaded" << disk_index->TotalDocs() << "documents\n";

  auto* raw_index = disk_index.get();
  search::Searcher searcher(std::move(disk_index), stopword_path);
  search::SearchServerImpl service(&searcher, raw_index);

  std::string addr = "0.0.0.0:" + port;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  server = builder.BuildAndStart();
  std::cout << "search server listening on " << addr << "\n";

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  server->Wait();
  return 0;
}
