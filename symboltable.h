#pragma once

#include "value.h"
#include <vector>

class SymbolTable {
  private:
	static std::vector<NextString> symbols;

  public:
	static bool hasSymbol(NextString sym);
	// returns previous index if already inserted
	static uint64_t insertSymbol(NextString sym);

	static NextString getSymbol(uint64_t idx);
};
