#include "writer.h"
#include "stream.h"

std::size_t ByteWriter::write(const void *start, std::size_t n,
                              WritableStream &stream) {
	return stream.writebytes(start, n);
}
