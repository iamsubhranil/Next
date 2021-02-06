#include "printer.h"
#include <cstdio>

FileStream Printer::StdOutFileStream =
    FileStream(stdout, FileStream::Mode::Write);
WritableStream &Printer::StdOutStream = Printer::StdOutFileStream;
