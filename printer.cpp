#include "printer.h"
#include <cstdio>

FileStream Printer::StdOutFileStream =
    FileStream(stdout, FileStream::Mode::Write);
FileStream Printer::StdErrFileStream =
    FileStream(stderr, FileStream::Mode::Write);
FileStream Printer::StdInFileStream = FileStream(stdin, FileStream::Mode::Read);

WritableStream &Printer::StdOutStream = Printer::StdOutFileStream;
WritableStream &Printer::StdErrStream = Printer::StdErrFileStream;
ReadableStream &Printer::StdInStream  = Printer::StdInFileStream;
