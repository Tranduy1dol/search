#include <gtest/gtest.h>

#include "search/crawler/crawler.h"

using search::CrawledPage;
using search::Crawler;

class TestableCrawler : public Crawler {
 public:
  void TestExtract(const std::string& html, CrawledPage& page) {
    ExtractTextAndTitle(html, page);
  }
};

TEST(CrawlerTest, ParseHTML) {
  TestableCrawler crawler;
  CrawledPage page;

  std::string mock_html = R"(
        <html>
        <head>
            <title>My Test Page</title>
            <style>body { color: red; }</style>
        </head>
        <body>
            <h1>Hello World</h1>
            <p>This is a <b>test</b>.</p>
            <script>console.log("ignore me");</script>
        </body>
        </html>
    )";

  crawler.TestExtract(mock_html, page);

  EXPECT_EQ(page.title_, "My Test Page");

  // Text should not contain style or script content
  EXPECT_TRUE(page.text_content_.find("Hello World") != std::string::npos);
  EXPECT_TRUE(page.text_content_.find("This is a") != std::string::npos);
  EXPECT_TRUE(page.text_content_.find("test") != std::string::npos);

  EXPECT_TRUE(page.text_content_.find("color: red") == std::string::npos);
  EXPECT_TRUE(page.text_content_.find("ignore me") == std::string::npos);
}
