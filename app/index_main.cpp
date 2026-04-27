#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "search/crawler/crawler.h"
#include "search/indexer/disk_index.h"
#include "search/indexer/inverted_index.h"

std::vector<std::string> LoadSeedUrls(const std::string& file_path) {
  std::vector<std::string> results;
  std::ifstream file(file_path);

  if (!file.is_open()) {
    std::cerr << "could not open file " << file_path << "\n";
    return results;
  }

  std::string line;
  while (std::getline(file, line)) {
    while (!line.empty() && std::isspace(line.back())) {
      line.pop_back();
    }
    while (!line.empty() && std::isspace(line.front())) {
      line.erase(line.begin());
    }

    if (!line.empty() && line[0]) {
      results.push_back(line);
    }
  }

  return results;
}

int main(int argc, char* argv[]) {
  std::cout << "start indexing ...";

  std::string seed_file = (argc > 1) ? argv[1] : "data/seed_urls.txt";
  std::vector<std::string> seed_urls = LoadSeedUrls(seed_file);
  if (seed_urls.empty()) {
    std::cerr << "no urls to crawl. exiting.\n";
    return 1;
  }

  std::mutex queue;
  std::mutex results;
  std::vector<search::CrawledPage> pages;
  size_t curr_url_idx = 0;

  auto worker_task = [&]() {
    search::Crawler local_crawler;
    while (true) {
      std::string url;

      {
        std::lock_guard<std::mutex> lock(queue);
        if (curr_url_idx >= seed_urls.size()) {
          break;
        }
        url = seed_urls[curr_url_idx];
        curr_url_idx++;
      }

      search::CrawledPage page = local_crawler.Crawl(url);
      if (page.success_) {
        std::lock_guard<std::mutex> lock(results);
        pages.push_back(std::move(page));
      }
    }
  };

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }
  num_threads = std::min(num_threads, 10u);

  std::vector<std::thread> threads;
  for (unsigned int i = 0; i < num_threads; i++) {
    threads.emplace_back(worker_task);
  }

  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  uint32_t doc_id = 1;
  search::InvertedIndex index("data/stopwords_vi.txt");
  for (const auto& page : pages) {
    index.AddDocument(doc_id++, page.text_content_, page.title_, page.url_);
  }

  search::DiskIndex::Serialize(index, "data/index/index.bin");
  std::cout << "done, total documents indexed: " << index.TotalDocs() << "\n";
  return 0;
}
