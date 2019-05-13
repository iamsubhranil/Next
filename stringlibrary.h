#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class StringLibrary {
  private:
	static std::unordered_map<size_t, std::string> strings;

  public:
	static size_t        insert(const std::string &s);
	static size_t        insert(const char *s, size_t length);
	static const std::string &get(size_t hash_);
	static const char *       get_raw(size_t hash_);
	static void               print(std::ostream &os);
};
