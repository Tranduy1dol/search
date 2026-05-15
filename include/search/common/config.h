#pragma once

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace search {

struct Config {
  std::string index_path_;
  std::string stopword_path_;
  std::string port_;
  uint32_t flush_interval_sec_;

  static Config FromEnv() {
    Config cfg;

    cfg.index_path_ =
        GetEnv("APP_SEARCH__INDEX_PATH",
               GetEnv("INDEX_PATH", "data/index/index.bin"));
    cfg.stopword_path_ =
        GetEnv("APP_SEARCH__STOPWORDS_PATH",
               GetEnv("STOPWORDS_PATH", "data/stopwords_vi.txt"));
    cfg.flush_interval_sec_ =
        GetUint32("APP_SEARCH__FLUSH_INTERVAL_SEC",
                  GetUint32("FLUSH_INTERVAL_SEC", 60, 5, 3600), 5, 3600);

    cfg.port_ = GetEnv(
        "APP_SEARCH__PORT",
        GetEnv("SEARCH_PORT", GetEnv("PORT", "50051")));

    ValidatePort(cfg.port_);

    return cfg;
  }

 private:
  static std::string GetEnv(const char* key, std::string default_val) {
    if (const char* v = std::getenv(key)) {
      return v;
    }

    return default_val;
  }

  static uint32_t GetUint32(const char* key, uint32_t default_val,
                            uint32_t min_val, uint32_t max_val) {
    const char* v = std::getenv(key);
    if (!v) {
      return default_val;
    }

    try {
      int64_t parsed = std::stoll(v);
      if (parsed < min_val || parsed > max_val) {
        throw std::runtime_error(
            std::string(key) + "=" + v + " is out of range [" +
            std::to_string(min_val) + ", " + std::to_string(max_val) + "]");
      }

      return static_cast<uint32_t>(parsed);
    } catch (const std::invalid_argument&) {
      throw std::runtime_error(std::string(key) + "=" + v +
                               " is not a valid integer");
    }
  }

  static void ValidatePort(const std::string& port) {
    try {
      int p = std::stoi(port);
      if (p < 1 || p > 65535) {
        throw std::runtime_error("port out of range: " + port);
      }
    } catch (const std::invalid_argument&) {
      throw std::runtime_error("port is not a valid number: " + port);
    }
  }
};

}  // namespace search
