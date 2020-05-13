#include "import.h"
#include "objects/array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef DEBUG
#include <iostream>
#endif

int dirExists(const char *path_) {
	struct stat info;

	if(stat(path_, &info) != 0)
		return 0;
	else if(info.st_mode & S_IFDIR)
		return 1;
	else
		return 0;
}

const char *kPathSeparator =
#ifdef _WIN32
    "\\";
#else
    "/";
#endif

using namespace std;

ImportStatus Importer::import(vector<Token> &loc) {
	ImportStatus ret;
	ret.fileName    = String::from("<bad>");
	ret.res         = ImportStatus::BAD_IMPORT;
	ret.toHighlight = 0;

	Array *strs = Array::create(1);
	for(auto i = loc.begin(), j = loc.end(); i != j; i++) {
		strs->insert(String::from((*i).start, (*i).length));
	}

	bool    lastIsFolder = true;
	auto    it           = 0;
	String *dot          = String::from(".");
	String *paths        = dot;
	// the path will be relative to the file
	// where the import statement is written
	const char *lastPath   = (*loc.begin()).fileName;
	int         lastSepLoc = strlen(lastPath) - 1;
	// start from the back, and stop at the first '/' or '\'
	// encountered. if none found, bail out.
	while(lastSepLoc >= 0 &&
	      (lastPath[lastSepLoc] != '/' && lastPath[lastSepLoc] != '\\'))
		lastSepLoc--;
	if(lastSepLoc == -1)
		paths = dot;
	else
		paths = String::from(lastPath, lastSepLoc);

	const char *path_ = paths->str;

	while(lastIsFolder && it != strs->size) {
		paths = String::append(
		    paths, String::append(kPathSeparator, strs->values[it].toString()));
		path_ = paths->str;
		if(!dirExists(path_)) {
			lastIsFolder = false;
		}
		it++;
	}
#ifdef DEBUG
	cout << "Path generated : " << path_ << endl;
#endif
	if(!lastIsFolder && it != strs->size) {
		// cout << "Not a valid import : " << path_ << endl;
		ret.toHighlight = it;
		return ret;
	} else if(lastIsFolder && it == strs->size) {
		// cout << "Unable to import whole folder '" << paths << "'!" << endl;
		ret.res         = ImportStatus::FOLDER_IMPORT;
		ret.toHighlight = strs->size - 1;
		return ret;
	}
	paths   = String::append(path_, ".n");
	path_   = paths->str;
	FILE *f = fopen(path_, "r");
	if(f == NULL) {
		// cout << "Unable to open file : '" << path_ << "'!" << endl;
		ret.res         = ImportStatus::FILE_NOT_FOUND;
		ret.fileName    = paths;
		ret.toHighlight = it;
		return ret;
	}
	ret.res      = ImportStatus::IMPORT_SUCCESS;
	ret.fileName = paths;
#ifdef DEBUG
	cout << path_ << " imported successfully!" << endl;
#endif
	fclose(f);
	return ret;
}
