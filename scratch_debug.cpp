#include <iostream>
#include "search/crawler/crawler.h"

using namespace search;

class TestableCrawler : public Crawler {
public:
    void TestExtract(const std::string& html, CrawledPage& page) {
        ExtractTextAndTitle(html, page);
    }
};

int main() {
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
    std::cout << "EXTRACTED TEXT:\n'" << page.text_content_ << "'\n";
    return 0;
}
