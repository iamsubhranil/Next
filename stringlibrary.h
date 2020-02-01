#pragma once

#include <cstring>
#include <unordered_set>

struct _NextString {
	char *   str;
	int      len;
	uint32_t hash_;
};

typedef _NextString *NextString;

struct NextStringHash {
	std::size_t operator()(_NextString *v) const { return v->hash_; }
};

struct NextStringEqual {
	bool operator()(const _NextString *a, const _NextString *b) const {
		return a->hash_ == b->hash_ && a->len == b->len &&
		       strcmp(a->str, b->str) == 0;
	}
};

class StringLibrary {
  private:
	static std::unordered_set<NextString, NextStringHash, NextStringEqual>
	    strings;

  public:
	static NextString  insert(const std::string &s);
	static NextString  insert(const char *s);
	static NextString  insert(const char *s, size_t length);
	static NextString  append(std::initializer_list<const char *> parts);
	static NextString  append(std::initializer_list<NextString> parts);
	static const char *get(NextString str);
	static const char *get_raw(NextString str);
	static int         get_len(NextString str);
	static void        print(std::ostream &os);
};
