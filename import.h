#pragma once

#include "objects/string.h"

typedef struct ImportStatus {
	enum {
		IMPORT_SUCCESS,
		BAD_IMPORT,
		FILE_NOT_FOUND,
		FOPEN_ERROR, // fileName will contain the strerror message
		FOLDER_IMPORT,
		PARTIAL_IMPORT // first part is resolved, the rest will be
		               // load_field
	} res;
	int     toHighlight; // number of token to highlight
	String *fileName;
} ImportStatus;

struct Importer {
	static ImportStatus import(const String2 &currentPath, const Value *parts,
	                           int size);
};
