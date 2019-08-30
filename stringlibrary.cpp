#include "stringlibrary.h"
#include "display.h"
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

HashMap<uint32_t, string> StringLibrary::strings =
    HashMap<uint32_t, string>{{hash_string("hello!"), string("hello!")}};

uint32_t StringLibrary::insert(const string &s) {
	uint32_t hash_ = hash_string(s.data());
	if(strings.find(hash_) == strings.end()) {
		strings[hash_] = s;
	}
	return hash_;
}

uint32_t StringLibrary::insert(const char *s) {
	return insert(string(s));
}

uint32_t StringLibrary::insert(const char *s, size_t len) {
	return insert(string(s, len));
}

const string &StringLibrary::get(uint32_t hash_) {
	if(strings.find(hash_) == strings.end())
		panic("Bad hash_ %u!", hash_);
	return strings[hash_];
}

const char *StringLibrary::get_raw(uint32_t idx) {
	return get(idx).c_str();
}

void StringLibrary::print(ostream &os) {
	for(auto const &i : strings) {
		os << setw(10) << i.second << "\t" << i.first << endl;
	}
}
