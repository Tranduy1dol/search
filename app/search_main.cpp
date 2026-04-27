#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "search/indexer/disk_index.h"
#include "search/searcher/searcher.h"

int main() {
  std::cout << "loading index ...";
  try {
    auto disk_index = std::make_unique<search::DiskIndex>("index.bin");
    std::cout << "loaded " << disk_index->TotalDocs() << " documents\n";

    search::Searcher searcher(std::move(disk_index), "data/stopwords_vi.txt");

    std::string query;
    std::cout << "search: (type 'exit' to quit)\n";

    while (true) {
      std::getline(std::cin, query);

      if (query == "exit") {
        break;
      }
      if (query.empty()) {
        continue;
      }

      auto results = searcher.Search(query, 5);
      if (results.empty()) {
        std::cout << "no result found\n";
      }

      for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << results[i].title_ << "\n";
        std::cout << "    " << "score: " << results[i].score_ << "\n";
        std::cout << "    " << "url: " << results[i].snippet_ << "\n\n";
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "fatal error: " << e.what() << "\n";
    std::cerr << "ensure run indexer first\n";
    return 1;
  }

  return 0;
}
