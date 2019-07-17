#pragma once

#include <vector>
#include "value.h"
#include "scanner.h"

typedef struct ImportStatus {
	enum { IMPORT_SUCCESS, BAD_IMPORT, FILE_NOT_FOUND, FOLDER_IMPORT } res;
	int         toHighlight; // number of token to highlight
	std::string fileName;
} ImportStatus;

ImportStatus import(std::vector<Token> &lines);
