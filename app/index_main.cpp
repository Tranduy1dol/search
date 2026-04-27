#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
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

  search::Crawler crawler;
  search::InvertedIndex index("data/stopwords_vi.txt");

  uint32_t doc_id = 1;

  for (const auto& url : seed_urls) {
    std::cout << "investigate source: " << url << "...";
    std::cout.flush();

    search::CrawledPage page = crawler.Crawl(url);

    if (page.success_) {
      std::cout << "success " << page.title_ << "\n";
      index.AddDocument(doc_id++, page.text_content_, page.title_, page.title_);
    } else {
      std::cout << "failed\n";
    }
  }

  std::cout << "writing index ...";
  search::DiskIndex::Serialize(index, "index.bin");
  std::cout << "done, total documents indexed: " << index.TotalDocs() << "\n";

  return 0;
}
