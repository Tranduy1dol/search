#pragma once

#include <string>
#include <vector>

namespace search::utils {
std::string Normalize(const std::string& utf8_text);
std::vector<std::string> SplitTokens(const std::string& text);
bool IsDelimiter(char c);

void WriteString(FILE* fp, const std::string& str);
std::string ReadString(const char*& ptr);
}  // namespace search::utils
