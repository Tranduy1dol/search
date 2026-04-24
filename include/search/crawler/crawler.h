#pragma once

#include <string>
namespace search {

struct CrawledPage {
  std::string url_;
  std::string title_;
  std::string text_content_;
  bool success_ = false;
};

class Crawler {
 public:
  Crawler();
  ~Crawler();

  Crawler(const Crawler&) = delete;
  Crawler& operator=(const Crawler&) = delete;

  CrawledPage Crawl(const std::string& url);

 protected:
  void ExtractTextAndTitle(const std::string& html, CrawledPage& page);

 private:
  void* curl_handle_ = nullptr;
};

}  // namespace search
