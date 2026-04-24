#include <sys/types.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace search::utils {

bool IsDelimiter(char c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '.' ||
          c == ',' || c == '!' || c == '?' || c == ';' || c == ':' ||
          c == '(' || c == ')' || c == '"' || c == '\'' || c == '-' ||
          c == '/' || c == '\\' || c == '[' || c == ']');
}

std::vector<std::string> SplitTokens(const std::string& text) {
  std::vector<std::string> result;
  std::string current_word;

  for (char c : text) {
    if (IsDelimiter(c)) {
      if (!current_word.empty()) {
        result.push_back(current_word);
        current_word = "";
      }
    } else {
      current_word.push_back(c);
    }
  }

  if (!current_word.empty()) {
    result.push_back(current_word);
  }

  return result;
}

std::string Normalize(const std::string& utf8_text) {
  std::string result = utf8_text;

  for (char& c : result) {
    if ('A' <= c && c <= 'Z') {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
  }

  return result;
}

void WriteString(FILE* fp, const std::string& str) {
  uint32_t len = str.size();
  fwrite(&len, sizeof(uint32_t), 1, fp);
  fwrite(str.data(), 1, len, fp);
}

std::string ReadString(const char*& ptr) {
  uint32_t len;
  std::memcpy(&len, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  std::string str(ptr, len);
  ptr += len;
  return str;
}

}  // namespace search::utils
