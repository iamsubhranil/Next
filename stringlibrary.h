#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class StringLibrary {
  private:
	std::unordered_map<size_t, std::string> strings;

  public:
	StringLibrary();
	size_t               insert(const std::string &s);
	size_t               insert(const char *s, size_t length);
	const std::string &  get(size_t hash_);
	const char *         get_raw(size_t hash_);
	friend std::ostream &operator<<(std::ostream &os, const StringLibrary &sl);
};
