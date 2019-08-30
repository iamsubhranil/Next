#pragma once

#include "hashmap.h"
#include <string>
#include <vector>

class StringLibrary {
  private:
	static HashMap<uint32_t, std::string> strings;

  public:
	static uint32_t           insert(const std::string &s);
	static uint32_t           insert(const char *s);
	static uint32_t           insert(const char *s, size_t length);
	static const std::string &get(uint32_t hash_);
	static const char *       get_raw(uint32_t hash_);
	static void               print(std::ostream &os);
};
