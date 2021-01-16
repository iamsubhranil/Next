#include "printer.h"
#include <cstdio>

OutputStream Printer::StdOutStream = OutputStream();
FileStream   Printer::OutStream    = FileStream(stdout);

void Printer::init() {
	StdOutStream.setStream(&OutStream);
}
