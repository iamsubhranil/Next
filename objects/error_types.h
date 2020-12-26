#ifndef ERROR
#define ERROR(x, name)
#endif

ERROR(FormatError, "format_error")
ERROR(ImportError, "import_error")
ERROR(IndexError, "index_error")
ERROR(IteratorError, "iterator_error")
ERROR(RuntimeError, "runtime_error")
ERROR(TypeError, "type_error")
ERROR(FileError, "file_error")

#undef ERROR
