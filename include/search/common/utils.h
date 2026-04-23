#pragma once

#include <string>
#include <vector>

namespace search::utils {
std::string Normalize(const std::string& utf8_text);
std::vector<std::string> SplitTokens(const std::string& text);
bool IsDelimiter(char c);
}  // namespace search::utils
