#ifndef ERRORTYPE
#define ERRORTYPE(x, name)
#endif

ERRORTYPE(FormatError, "format_error")
ERRORTYPE(ImportError, "import_error")
ERRORTYPE(IndexError, "index_error")
ERRORTYPE(IteratorError, "iterator_error")
ERRORTYPE(RuntimeError, "runtime_error")
ERRORTYPE(TypeError, "type_error")
ERRORTYPE(FileError, "file_error")

#undef ERRORTYPE
