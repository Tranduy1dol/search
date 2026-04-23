#pragma once

#include <string>
#include <vector>

namespace search::utils {
std::string normalize(const std::string &utf8_text);
std::vector<std::string> split_tokens(const std::string &text);
bool is_delimiter(char c);
} // namespace search::utils
