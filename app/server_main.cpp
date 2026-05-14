#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "search/indexer/live_index.h"
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
  uint32_t flush_interval = 60;

  if (const char* env = std::getenv("INDEX_PATH")) index_path = env;
  if (const char* env = std::getenv("STOPWORD_PATH")) stopword_path = env;
  if (const char* env = std::getenv("SEARCH_PORT")) {
    port = env;
  } else if (const char* app_port = std::getenv("PORT")) {
    port = app_port;
  }
  if (const char* env = std::getenv("FLUSH_INTERVAL_SEC")) {
    flush_interval = std::stoul(env);
  }

  search::LiveIndex live_index(index_path, stopword_path, flush_interval);
  std::cout << "loaded" << live_index.TotalDocs() << "documents\n";

  search::LiveSearcher searcher(&live_index, stopword_path);
  search::SearchServerImpl service(&searcher, &live_index);

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
