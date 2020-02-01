#include "stringlibrary.h"
#include <iomanip>

using namespace std;

constexpr uint32_t hash_string(const char *s) {
	uint32_t hash_ = 0;

	for(; *s; ++s) {
		hash_ += *s;
		hash_ += (hash_ << 10);
		hash_ ^= (hash_ >> 6);
	}

	hash_ += (hash_ << 3);
	hash_ ^= (hash_ >> 11);
	hash_ += (hash_ << 15);

	return hash_;
}

NextString create_string(const char *s) {
	_NextString *ns = (_NextString *)malloc(sizeof(_NextString));
	ns->str         = strdup(s);
	uint32_t hash_  = 0;
	int      l      = 0;

	for(; *s; ++s) {
		hash_ += *s;
		hash_ += (hash_ << 10);
		hash_ ^= (hash_ >> 6);
		l++;
	}

	hash_ += (hash_ << 3);
	hash_ ^= (hash_ >> 11);
	hash_ += (hash_ << 15);

	ns->hash_ = hash_;
	ns->len   = l;

	return ns;
}

unordered_set<NextString, NextStringHash, NextStringEqual>
    StringLibrary::strings =
        unordered_set<NextString, NextStringHash, NextStringEqual>();

NextString StringLibrary::insert(const string &s) {
	NextString ns  = create_string(s.c_str());
	auto       res = strings.insert(ns);
	if(!res.second) {
		free(ns->str);
		free(ns);
	}
	return *res.first;
}

NextString StringLibrary::insert(const char *s) {
	return insert(string(s));
}

NextString StringLibrary::insert(const char *s, size_t len) {
	return insert(string(s, len));
}

NextString StringLibrary::append(initializer_list<const char *> parts) {
	char *final_string = NULL;
	int   len          = 0;
	for(auto &part : parts) {
		int newlen        = len + strlen(part) + 1;
		final_string      = (char *)realloc(final_string, newlen);
		final_string[len] = 0;
		strcat(final_string, part);
		len = newlen - 1;
	}
	NextString res = insert(final_string);
	free(final_string);
	return res;
}

NextString StringLibrary::append(initializer_list<NextString> parts) {
	char *final_string = NULL;
	int   len          = 0;
	for(auto &part : parts) {
		int newlen        = len + part->len + 1;
		final_string      = (char *)realloc(final_string, newlen);
		final_string[len] = 0;
		strcat(final_string, part->str);
		len = newlen - 1;
	}
	NextString res = insert(final_string);
	free(final_string);
	return res;
}

const char *StringLibrary::get(NextString ns) {
	return ns->str;
}

const char *StringLibrary::get_raw(NextString ns) {
	return ns->str;
}

int StringLibrary::get_len(NextString ns) {
	return ns->len;
}

void StringLibrary::print(ostream &os) {
	for(auto const &i : strings) {
		os << setw(10) << i->hash_ << "\t" << i->str << endl;
	}
}
