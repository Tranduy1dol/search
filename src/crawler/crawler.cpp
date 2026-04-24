#include "search/crawler/crawler.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <lexbor/html/html.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "lexbor/core/base.h"
#include "lexbor/core/types.h"
#include "lexbor/dom/interface.h"
#include "lexbor/dom/interfaces/node.h"
#include "lexbor/html/interfaces/document.h"
#include "lexbor/tag/const.h"

namespace search {

static size_t WriteCallback(void* content, size_t size, size_t nmemb,
                            void* userp) {
  size_t total_size = size * nmemb;
  std::string* str = static_cast<std::string*>(userp);
  str->append(static_cast<char*>(content), total_size);
  return total_size;
}

static void WalkDOM(lxb_dom_node_t* node, std::string& out_text) {
  if (node == nullptr) {
    return;
  }

  if (node->local_name == LXB_TAG_SCRIPT || node->local_name == LXB_TAG_STYLE) {
    return;
  }

  if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
    size_t len = 0;
    const lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    if (text && len > 0) {
      out_text.append(reinterpret_cast<const char*>(text), len);
      out_text.push_back(' ');
    }
  }

  WalkDOM(lxb_dom_node_first_child(node), out_text);
  WalkDOM(lxb_dom_node_next(node), out_text);
}

Crawler::Crawler() { curl_handle_ = curl_easy_init(); }

Crawler::~Crawler() {
  if (curl_handle_) {
    curl_easy_cleanup(curl_handle_);
  }
}

CrawledPage Crawler::Crawl(const std::string& url) {
  CrawledPage page;
  page.url_ = url;

  if (!curl_handle_) {
    return page;
  }

  std::string raw_html;
  curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &raw_html);
  curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "mini-search-agent");

  CURLcode res = curl_easy_perform(curl_handle_);

  if (res != CURLE_OK) {
    std::cerr << "curl error on " << url << ": " << curl_easy_strerror(res)
              << "\n";
    return page;
  }

  uint64_t http_code = 0;
  curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 200) {
    return page;
  }

  ExtractTextAndTitle(raw_html, page);
  page.success_ = true;

  return page;
}

void Crawler::ExtractTextAndTitle(const std::string& html, CrawledPage& page) {
  lxb_html_document_t* document = lxb_html_document_create();
  lxb_status_t status = lxb_html_document_parse(
      document, reinterpret_cast<const lxb_char_t*>(html.data()),
      html.length());

  if (status != LXB_STATUS_OK) {
    lxb_html_document_destroy(document);
    return;
  }

  const lxb_char_t* title = lxb_html_document_title(document, nullptr);
  if (title) {
    page.title_ = reinterpret_cast<const char*>(title);
  }

  lxb_dom_node_t* body =
      lxb_dom_interface_node(lxb_html_document_body_element(document));
  WalkDOM(body, page.text_content_);

  lxb_html_document_destroy(document);
}

}  // namespace search
