#include "stringlibrary.h"
#include "display.h"
#include <iomanip>

using namespace std;

StringLibrary::StringLibrary() : stringCount(0) {}

size_t StringLibrary::insert(const string &s) {
	if(strings.find(s) != strings.end()) {
		return strings[s];
	}
	strings[string(s)] = stringCount++;
	return stringCount - 1;
}

size_t StringLibrary::insert(const char *s, size_t len) {
	string str(s, len);
	if(strings.find(str) != strings.end()) {
		return strings[str];
	}
	strings[string(str)] = stringCount++;
	return stringCount - 1;
}

const string &StringLibrary::get(size_t idx) {
	for(auto i = strings.begin(), j = strings.end(); i != j; i++) {
		if(i->second == idx)
			return i->first;
	}
	panic("Bad index %u!", idx);
}

const char *StringLibrary::get_raw(size_t idx) {
	return get(idx).c_str();
}

ostream &operator<<(ostream &os, const StringLibrary &sl) {
	for(auto i = sl.strings.begin(), j = sl.strings.end(); i != j; i++) {
		os << setw(10) << i->first << "\t" << i->second << endl;
	}
	return os;
}
