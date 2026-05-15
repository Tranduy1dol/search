#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>

#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "search/common/config.h"
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
  search::Config cfg;
  try {
    cfg = search::Config::FromEnv();
  } catch (const std::exception& e) {
    std::cerr << "invalid config: " << e.what() << "\n";
    return 1;
  }

  search::LiveIndex live_index(cfg.index_path_, cfg.stopword_path_,
                               cfg.flush_interval_sec_);
  std::cout << "loaded" << live_index.TotalDocs() << "documents\n";

  search::LiveSearcher searcher(&live_index, cfg.stopword_path_);
  search::SearchServerImpl service(&searcher, &live_index);

  std::string addr = "0.0.0.0:" + cfg.port_;
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
