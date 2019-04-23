#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class StringLibrary {
  private:
	std::unordered_map<std::string, size_t> strings;
	size_t                                  stringCount;

  public:
	StringLibrary();
	size_t               insert(const std::string &s);
	size_t               insert(const char *s, size_t length);
	const std::string &  get(size_t idx);
	const char *         get_raw(size_t idx);
	friend std::ostream &operator<<(std::ostream &os, const StringLibrary &sl);
};
