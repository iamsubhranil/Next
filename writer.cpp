#include "writer.h"
#include "stream.h"

std::size_t ByteWriter::write(const void *start, std::size_t n,
                              OutputStream &stream) {
	return stream.writebytes(start, n);
}
