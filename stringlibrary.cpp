#include "stringlibrary.h"
#include "display.h"
#include <iomanip>

using namespace std;

unordered_map<size_t, string> StringLibrary::strings = {};

size_t StringLibrary::insert(const string &s) {
	size_t hash_ = hash<string>{}(s);
	if(strings.find(hash_) == strings.end()) {
		strings[hash_] = s;
	}
	return hash_;
}

size_t StringLibrary::insert(const char *s, size_t len) {
	string str(s, len);
	return insert(str);
}

const string &StringLibrary::get(size_t hash_) {
	if(strings.find(hash_) == strings.end())
		panic("Bad hash %u!", hash_);
	return strings[hash_];
}

const char *StringLibrary::get_raw(size_t idx) {
	return get(idx).c_str();
}

void StringLibrary::print(ostream &os) {
	for(auto i = strings.begin(), j = strings.end(); i != j; i++) {
		os << setw(10) << i->second << "\t" << i->first << endl;
	}
}
