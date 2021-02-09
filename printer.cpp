#include "printer.h"
#include <cstdio>

FileStream Printer::StdOutFileStream =
    FileStream(stdout, FileStream::Mode::Write);
FileStream Printer::StdErrFileStream =
    FileStream(stderr, FileStream::Mode::Write);
StdInputStream Printer::StdInFileStream = StdInputStream();

WritableStream &Printer::StdOutStream = Printer::StdOutFileStream;
WritableStream &Printer::StdErrStream = Printer::StdErrFileStream;
ReadableStream &Printer::StdInStream  = Printer::StdInFileStream;
