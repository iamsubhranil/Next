#include "bits.h"
#include "bits_iterator.h"
#include "buffer.h"
#include "class.h"
#include "file.h"
#include "string.h"
#include "tuple.h"

Value next_bits_construct_empty(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	Bits *b     = Bits::create(0);
	b->bytes[0] = 0;
	return Value(b);
}

Value next_bits_construct(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "(number_of_bits)", 1, Integer);
	int64_t numbits = args[1].toInteger();
	if(numbits < 1) {
		RERR("Number of bits in a bit array should be > 0!");
	}
	Bits2 ba = Bits::create(numbits);
	std::memset(ba->bytes, 0, ba->chunkcount * Bits::ChunkSizeByte);
	return Value(ba);
}

Value next_bits_construct_value(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "(number_of_bits, default_bit)", 1, Integer);
	EXPECT(bits, "(number_of_bits, default_bit)", 2, Bit);
	int64_t numbits = args[1].toInteger();
	if(numbits < 1) {
		RERR("Number of bits in a bit array should be > 0!");
	}
	Bits::ChunkType defbit = args[2].toInteger();
	if(defbit) {
		defbit = Bits::ChunkFull;
	}
	Bits2 ba = Bits::create(numbits);
	std::memset(ba->bytes, defbit, ba->chunkcount * Bits::ChunkSizeByte);
	if(defbit && (numbits & Bits::ChunkRemainderAnd)) {
		// in the last chunk, set the remaining bits to zero
		ba->bytes[ba->chunkcount - 1] =
		    Bits::ChunkFull >>
		    (Bits::ChunkSize - (numbits & Bits::ChunkRemainderAnd));
	}
	return Value(ba);
}

Value next_bits_from(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "from(value)", 1, Integer);
	int64_t val = args[1].toInteger();
	Bits2   ba  = Bits::create(sizeof(int64_t) * 8);
	std::memcpy(ba->bytes, &val, sizeof(int64_t));
	return Value(ba);
}

Value next_bits_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "set(bit)", 1, Integer);
	int64_t val = args[1].toInteger();
	Bits *  ba  = args[0].toBits();
	if(val < 0 || val >= ba->size) {
		RERR("Invalid bit index!");
	}
	ba->bytes[val >> Bits::ChunkCountShift] |=
	    ((Bits::ChunkType)1 << (val & Bits::ChunkRemainderAnd));
	return ValueNil;
}

Value next_bits_setall(const Value *args, int numargs) {
	(void)numargs;
	Bits *ba = args[0].toBits();
	std::memset(ba->bytes, -1, ba->chunkcount * Bits::ChunkSizeByte);
	int64_t lastbits = ba->size & Bits::ChunkRemainderAnd;
	if(lastbits) {
		ba->bytes[ba->chunkcount - 1] =
		    Bits::ChunkFull >> (Bits::ChunkSize - lastbits);
	}
	return ValueNil;
}

Value next_bits_setinrange(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "set(from, to)", 1, Integer);
	EXPECT(bits, "set(from, to)", 2, Integer);
	Bits *  ba   = args[0].toBits();
	int64_t from = args[1].toInteger();
	int64_t to   = args[2].toInteger();

	if(from < 0 || from >= ba->size || to < 0 || to >= ba->size || from > to) {
		RERR("Invalid range specified!");
	}

	int64_t chunkfrom = from >> Bits::ChunkCountShift;
	int64_t chunkto   = to >> Bits::ChunkCountShift;
	if(chunkfrom + 1 < chunkto - 1) {
		std::memset(&ba->bytes[chunkfrom + 1], -1, (chunkto - 1 + chunkfrom));
	}
	int64_t fromrem = from & Bits::ChunkRemainderAnd;
	int64_t torem   = to & Bits::ChunkRemainderAnd;
	if(torem)
		torem = Bits::ChunkSize - torem - 1;
	Bits::ChunkType mask = Bits::ChunkFull << fromrem;
	if(chunkfrom == chunkto) {
		mask &= (Bits::ChunkFull >> torem);
		ba->bytes[chunkfrom] |= mask;
	} else {
		ba->bytes[chunkfrom] |= mask;
		ba->bytes[chunkto] |= (Bits::ChunkFull >> torem);
	}
	return ValueNil;
}

Value next_bits_reset(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "reset(bit)", 1, Integer);
	int64_t val = args[1].toInteger();
	Bits *  ba  = args[0].toBits();
	if(val < 0 || val >= ba->size) {
		RERR("Invalid bit index!");
	}
	Bits::ChunkType reset_pattern =
	    ~((Bits::ChunkType)1 << (val & Bits::ChunkRemainderAnd));
	ba->bytes[val >> Bits::ChunkCountShift] &= reset_pattern;
	return ValueNil;
}

Value next_bits_resetall(const Value *args, int numargs) {
	(void)numargs;
	Bits *ba = args[0].toBits();
	std::memset(ba->bytes, 0, ba->chunkcount * Bits::ChunkSizeByte);
	return ValueNil;
}

Value next_bits_resetinrange(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "reset(from, to)", 1, Integer);
	EXPECT(bits, "reset(from, to)", 2, Integer);
	Bits *  ba   = args[0].toBits();
	int64_t from = args[1].toInteger();
	int64_t to   = args[2].toInteger();

	if(from < 0 || from >= ba->size || to < 0 || to >= ba->size || from > to) {
		RERR("Invalid range specified!");
	}

	int64_t chunkfrom = from >> Bits::ChunkCountShift;
	int64_t chunkto   = to >> Bits::ChunkCountShift;
	if(chunkfrom + 1 < chunkto - 1) {
		std::memset(&ba->bytes[chunkfrom + 1], 0, (chunkto - 1 + chunkfrom));
	}
	int64_t fromrem = from & Bits::ChunkRemainderAnd;
	int64_t torem   = to & Bits::ChunkRemainderAnd;
	if(torem)
		torem = Bits::ChunkSize - torem - 1;
	Bits::ChunkType mask = Bits::ChunkFull << fromrem;
	if(chunkfrom == chunkto) {
		mask &= (Bits::ChunkFull >> torem);
		ba->bytes[chunkfrom] &= ~mask;
	} else {
		ba->bytes[chunkfrom] &= ~mask;
		ba->bytes[chunkto] &= ~(Bits::ChunkFull >> torem);
	}
	return ValueNil;
}

Value next_bits_toggle(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "toggle(bit)", 1, Integer);
	int64_t val = args[1].toInteger();
	Bits *  ba  = args[0].toBits();
	if(val < 0 || val >= ba->size) {
		RERR("Invalid bit index!");
	}
	Bits::ChunkType pattern =
	    ((Bits::ChunkType)1 << (val & Bits::ChunkRemainderAnd));
	ba->bytes[val >> Bits::ChunkCountShift] ^= pattern;
	return ValueNil;
}

Value next_bits_toggleall(const Value *args, int numargs) {
	(void)numargs;
	Bits *ba = args[0].toBits();
	for(int64_t i = 0; i < ba->chunkcount; i++) {
		ba->bytes[i] = ~ba->bytes[i];
	}
	if(ba->size & Bits::ChunkRemainderAnd) {
		ba->bytes[ba->chunkcount - 1] &=
		    (Bits::ChunkFull >>
		     (Bits::ChunkSize - (ba->size & Bits::ChunkRemainderAnd)));
	}
	return ValueNil;
}

Value next_bits_toggleinrange(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "toggle(from, to)", 1, Integer);
	EXPECT(bits, "toggle(from, to)", 2, Integer);
	Bits *  ba   = args[0].toBits();
	int64_t from = args[1].toInteger();
	int64_t to   = args[2].toInteger();

	if(from < 0 || from >= ba->size || to < 0 || to >= ba->size || from > to) {
		RERR("Invalid range specified!");
	}

	int64_t chunkfrom = from >> Bits::ChunkCountShift;
	int64_t chunkto   = to >> Bits::ChunkCountShift;
	for(int64_t i = chunkfrom + 1; i < chunkto - 1; i++) {
		ba->bytes[i] = ~ba->bytes[i];
	}
	int64_t fromrem = from & Bits::ChunkRemainderAnd;
	int64_t torem   = to & Bits::ChunkRemainderAnd;
	if(torem)
		torem = Bits::ChunkSize - torem - 1;
	Bits::ChunkType mask = Bits::ChunkFull << fromrem;
	if(chunkfrom == chunkto) {
		mask &= (Bits::ChunkFull >> torem);
		ba->bytes[chunkfrom] ^= mask;
	} else {
		ba->bytes[chunkfrom] ^= mask;
		ba->bytes[chunkto] ^= (Bits::ChunkFull >> torem);
	}
	return ValueNil;
}

Value next_bits_bit(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "bit(index)", 1, Integer);
	int64_t val = args[1].toInteger();
	Bits *  ba  = args[0].toBits();
	if(val < 0 || val >= ba->size) {
		RERR("Invalid bit index!");
	}
	Bits::ChunkType pattern =
	    ((Bits::ChunkType)1 << (val & Bits::ChunkRemainderAnd));
	Bits::ChunkType res = ba->bytes[val >> Bits::ChunkCountShift] & pattern;
	return Value((int)(res != 0));
}

Value next_bits_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "str(_)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	Bits *ba = args[0].toBits();
	if(ba->size == 0) {
		f->writableStream()->write('0');
		return ValueTrue;
	}
	for(int64_t i = ba->size - 1; i >= 0; i--) {
		Bits::ChunkType bit =
		    ba->bytes[i >> Bits::ChunkCountShift] &
		    ((Bits::ChunkType)1 << (i & Bits::ChunkRemainderAnd));
		char c = '0' + (bit != 0);
		f->writableStream()->write(c);
	}
	return ValueTrue;
}

#define NEXT_BITS_BINARY(name, op)                                \
	Value next_bits_##name(const Value *args, int numargs) {      \
		(void)numargs;                                            \
		EXPECT(bits, #op "(bits)", 1, Bits);                      \
		Bits *  b1   = args[0].toBits();                          \
		Bits *  b2   = args[1].toBits();                          \
		int64_t size = b1->size > b2->size ? b1->size : b2->size; \
		Bits *  b3   = Bits::create(size);                        \
		int64_t i    = 0;                                         \
		while(i < b1->chunkcount && i < b2->chunkcount) {         \
			b3->bytes[i] = b1->bytes[i] op b2->bytes[i];          \
			i++;                                                  \
		}                                                         \
		while(i < b1->chunkcount) {                               \
			b3->bytes[i] = b1->bytes[i];                          \
			i++;                                                  \
		}                                                         \
		while(i < b2->chunkcount) {                               \
			b3->bytes[i] = b2->bytes[i];                          \
			i++;                                                  \
		}                                                         \
		return Value(b3);                                         \
	}

// size of or/xor is that of the largest sequence
NEXT_BITS_BINARY(or, |)
NEXT_BITS_BINARY(xor, ^)

Value next_bits_and(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "&(value)", 1, Bits);
	Bits *a = args[0].toBits();
	Bits *b = args[1].toBits();
	// size of AND is that of the smallest sequence
	int64_t size   = a->size > b->size ? b->size : a->size;
	Bits *  c      = Bits::create(size);
	int64_t chunks = 0;
	while(chunks < a->chunkcount && chunks < b->chunkcount) {
		c->bytes[chunks] = a->bytes[chunks] & b->bytes[chunks];
		chunks++;
	}
	return Value(c);
}

Value next_bits_not(const Value *args, int numargs) {
	(void)numargs;
	Bits *a = args[0].toBits();
	Bits *b = Bits::create(a->size);
	for(int64_t i = 0; i < a->chunkcount; i++) {
		b->bytes[i] = ~a->bytes[i];
	}
	if(a->size & Bits::ChunkRemainderAnd) {
		// if we have uneven last byte, set the unused bits to zero
		b->bytes[a->chunkcount - 1] &=
		    (Bits::ChunkFull >>
		     (Bits::ChunkSize - (a->size & Bits::ChunkRemainderAnd)));
	}
	return Value(b);
}

Value next_bits_lshift(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "<<(shift)", 1, Integer);
	int64_t shift = args[1].toInteger();
	if(shift < 0) {
		RERR("Bit shift expected to be > 0!");
	}
	Bits *a = args[0].toBits();
	Bits *b = Bits::create(a->size + shift);
	// find out the shift in chunk
	int64_t chunkshift = shift >> Bits::ChunkCountShift;
	// find out the shift in each byte
	int64_t bitshift = shift & Bits::ChunkRemainderAnd;
	// find out the number of bits that is _lost_ after a shift on the byte
	int64_t         remshift = Bits::ChunkSize - bitshift;
	Bits::ChunkType lastbyte = 0;
	// set the initial bytes to 0
	std::memset(b->bytes, 0, chunkshift * Bits::ChunkSizeByte);
	for(int64_t i = 0; i < a->chunkcount; i++) {
		// shift the present byte, and or the remaining of the last byte
		b->bytes[i + chunkshift] =
		    (a->bytes[i] << bitshift) | (lastbyte >> remshift);
		lastbyte = a->bytes[i];
	}
	// whatever is left, dump that on the last byte
	b->bytes[b->chunkcount - 1] |= (lastbyte >> remshift);
	return Value(b);
}

Value next_bits_rshift(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, ">>(shift)", 1, Integer);
	int64_t shift = args[1].toInteger();
	if(shift < 0) {
		RERR("Invalid shift value!");
	}
	Bits *a = args[0].toBits();
	if(shift >= a->size) {
		Bits *b     = Bits::create(0);
		b->bytes[0] = 0;
		return Value(b);
	}
	// right shift shrinks the bits
	Bits *b = Bits::create(a->size - shift);
	// clear the last byte
	b->bytes[b->chunkcount - 1] = 0;
	int64_t chunkcount          = a->chunkcount;
	// find out the shift in chunk
	int64_t chunkshift = shift >> Bits::ChunkCountShift;
	// find out the shift in each byte
	int64_t bitshift = shift & Bits::ChunkRemainderAnd;
	// find out the number of bits that is _lost_ after a shift on the byte
	int64_t         remshift = Bits::ChunkSize - bitshift;
	Bits::ChunkType mask     = Bits::ChunkFull >> remshift;
	Bits::ChunkType lastbyte = 0;
	for(int64_t i = chunkcount - 1 - chunkshift; i >= 0; --i) {
		// shift the present byte, and or the part of the last byte that was
		// lost, after shifting them back to the left
		b->bytes[i] = (a->bytes[i + chunkshift] >> bitshift) |
		              ((lastbyte & mask) << remshift);
		lastbyte = a->bytes[i + chunkshift];
	}
	return Value(b);
}

Value next_bits_size(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toBits()->size);
}

#define NEXT_BITS_BINARY_INPLACE(name, op)                         \
	Value next_bits_##name(const Value *args, int numargs) {       \
		(void)numargs;                                             \
		EXPECT(bits, #name "(bits)", 1, Bits);                     \
		Bits *source = args[0].toBits();                           \
		Bits *with   = args[1].toBits();                           \
                                                                   \
		if(source->size < with->size) {                            \
			int64_t oldchunk = source->chunkcount;                 \
			source->resize(with->size);                            \
			for(int64_t i = oldchunk; i < with->chunkcount; i++) { \
				source->bytes[i] = with->bytes[i];                 \
			}                                                      \
			for(int64_t i = 0; i < oldchunk; i++) {                \
				source->bytes[i] op## = with->bytes[i];            \
			}                                                      \
			return source;                                         \
		}                                                          \
                                                                   \
		for(int64_t i = 0; i < with->chunkcount; i++) {            \
			source->bytes[i] op## = with->bytes[i];                \
		}                                                          \
                                                                   \
		return source;                                             \
	}

NEXT_BITS_BINARY_INPLACE(or_with, |)
NEXT_BITS_BINARY_INPLACE(xor_with, ^)

Value next_bits_and_with(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "and_with(bits)", 1, Bits);
	Bits *source = args[0].toBits();
	Bits *with   = args[1].toBits();

	if(with->size < source->size) {
		source->resize(with->size);
	}

	for(int64_t i = 0; i < source->chunkcount; i++) {
		source->bytes[i] &= with->bytes[i];
	}

	return Value(source);
}

Value next_bits_shift_left(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "shift_left(shift)", 1, Integer);
	int64_t shift = args[1].toInteger();
	if(shift < 0) {
		RERR("Bit shift expected to be > 0!");
	}
	Bits *a = args[0].toBits();
	a->resize(a->size + shift);
	// find out the shift in chunk
	int64_t chunkshift = shift >> Bits::ChunkCountShift;
	// find out the shift in each byte
	int64_t bitshift = shift & Bits::ChunkRemainderAnd;
	// find out the number of bits that is _lost_ after a shift on the byte
	int64_t         remshift = Bits::ChunkSize - bitshift;
	Bits::ChunkType lastbyte = 0;
	for(int64_t i = 0; i < a->chunkcount; i++) {
		// shift the present byte, and or the remaining of the last byte
		a->bytes[i + chunkshift] =
		    (a->bytes[i] << bitshift) | (lastbyte >> remshift);
		lastbyte = a->bytes[i];
	}
	// whatever is left, dump that on the last byte
	a->bytes[a->chunkcount - 1] |= (lastbyte >> remshift);
	if(chunkshift) {
		// clear up initial chunks
		std::memset(a->bytes, 0, chunkshift * Bits::ChunkSizeByte);
	}
	if(bitshift) {
		// clear up the initial bits of the leftmost chunk
		a->bytes[chunkshift] &= (Bits::ChunkFull << bitshift);
	}
	return Value(a);
}

Value next_bits_shift_right(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "shift_right(shift)", 1, Integer);
	int64_t shift = args[1].toInteger();
	if(shift < 0) {
		RERR("Invalid shift value!");
	}
	Bits *a = args[0].toBits();
	if(shift >= a->size) {
		a->resize(0);
		a->bytes[0] = 0;
		return Value(a);
	}
	int64_t chunkcount = a->chunkcount;
	// find out the shift in chunk
	int64_t chunkshift = shift >> Bits::ChunkCountShift;
	// find out the shift in each byte
	int64_t bitshift = shift & Bits::ChunkRemainderAnd;
	// find out the number of bits that is _lost_ after a shift on the byte
	int64_t         remshift = Bits::ChunkSize - bitshift;
	Bits::ChunkType mask     = Bits::ChunkFull >> remshift;
	Bits::ChunkType lastbyte = 0;
	for(int64_t i = 0; i < chunkcount - chunkshift; i++) {
		// shift the present byte, and or the part of the last byte that was
		// lost, after shifting them back to the left
		if(i + chunkshift + 1 < chunkcount)
			lastbyte = a->bytes[i + chunkshift + 1];
		else
			lastbyte = 0;
		a->bytes[i] = (a->bytes[i + chunkshift] >> bitshift) |
		              ((lastbyte & mask) << remshift);
	}
	a->resize(a->size - shift);
	// clear the extra part of the first chunk
	a->bytes[a->chunkcount - 1] &= Bits::ChunkFull >> bitshift;
	return Value(a);
}

Value next_bits_subs_get(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "[](_)", 1, Integer);
	int64_t idx = args[1].toInteger();
	Bits *  b   = args[0].toBits();
	if(idx < 0 || idx >= b->size) {
		RERR("Invalid bit index!");
	}
	int64_t chunk = idx >> Bits::ChunkCountShift;
	int64_t bit   = idx & Bits::ChunkRemainderAnd;
	return Value((int)((b->bytes[chunk] & (1 << bit)) != 0));
}

Value next_bits_subs_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "[](_,_)", 1, Integer);
	EXPECT(bits, "[](_,_)", 2, Bit);
	int64_t idx = args[1].toInteger();
	Bits *  b   = args[0].toBits();
	if(idx < 0 || idx >= b->size) {
		RERR("Invalid bit index!");
	}
	int64_t val   = args[2].toInteger();
	int64_t chunk = idx >> Bits::ChunkCountShift;
	int64_t bit   = idx & Bits::ChunkRemainderAnd;
	b->bytes[chunk] &= ~((Bits::ChunkType)1 << bit);
	b->bytes[chunk] |= ((Bits::ChunkType)val << bit);
	return Value(val);
}

Value next_bits_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(
	    BitsIterator::from(args[0].toBits(), BitsIterator::TraversalType::BIT));
}

Value next_bits_iterate_bytes(const Value *args, int numargs) {
	(void)numargs;
	return Value(BitsIterator::from(args[0].toBits(),
	                                BitsIterator::TraversalType::BYTE));
}

Value next_bits_iterate_ints(const Value *args, int numargs) {
	(void)numargs;
	return Value(BitsIterator::from(args[0].toBits(),
	                                BitsIterator::TraversalType::CHUNK));
}

Value next_bits_to_bytes(const Value *args, int numargs) {
	(void)numargs;
	Bits * b = args[0].toBits();
	Tuple *t = Tuple::create((b->size >> 3) + ((b->size & 7) != 0));
	for(int64_t i = 0, j = 0; i < b->size; i += 8, j++) {
		int64_t chunk  = i >> Bits::ChunkCountShift;
		int64_t bit    = i & Bits::ChunkRemainderAnd;
		int64_t val    = (b->bytes[chunk] >> bit) & 0xff;
		t->values()[j] = Value(val);
	}
	return t;
}

Value next_bits_to_ints(const Value *args, int numargs) {
	(void)numargs;
	Bits * b = args[0].toBits();
	Tuple *t = Tuple::create(b->chunkcount);
	for(int64_t i = 0, j = 0; i < b->chunkcount; i++, j++) {
		// hardcoded for 64bit chunks
		int64_t val    = b->bytes[i];
		t->values()[j] = Value(val);
	}
	return t;
}

Value next_bits_insert(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "insert(bit)", 1, Bit);
	int64_t b       = args[1].toInteger();
	Bits *  a       = args[0].toBits();
	int64_t oldsize = a->size;
	a->resize(a->size + 1);
	a->bytes[oldsize >> Bits::ChunkCountShift] |=
	    (b << (oldsize & Bits::ChunkRemainderAnd));
	return ValueNil;
}

Value next_bits_insert_byte(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "insert_byte(byte)", 1, Integer);
	int64_t b       = args[1].toInteger() & 0xff;
	Bits *  a       = args[0].toBits();
	int64_t oldsize = a->size;
	int64_t oldcc   = a->chunkcount;
	a->resize(a->size + 8);
	if(a->chunkcount == oldcc) {
		a->bytes[oldsize >> Bits::ChunkCountShift] |=
		    (b << (oldsize & Bits::ChunkRemainderAnd));
	} else {
		int64_t space = (oldsize & Bits::ChunkRemainderAnd);
		if(space) {
			a->bytes[a->chunkcount - 2] |= (b << space);
		}
		a->bytes[a->chunkcount - 1] = (b >> space);
	}
	return ValueNil;
}

Value next_bits_insert_int(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(bits, "insert_int(value)", 1, Integer);
	int64_t b       = args[1].toInteger();
	Bits *  a       = args[0].toBits();
	int64_t oldsize = a->size;
	int64_t oldcc   = a->chunkcount;
	a->resize(a->size + 64);
	if(a->chunkcount == oldcc) {
		a->bytes[oldsize >> Bits::ChunkCountShift] |=
		    (b << (oldsize & Bits::ChunkRemainderAnd));
	} else {
		int64_t space = (oldsize & Bits::ChunkRemainderAnd);
		if(space) {
			a->bytes[a->chunkcount - 2] |= (b << space);
		}
		a->bytes[a->chunkcount - 1] = (b >> space);
	}
	return ValueNil;
}

Value next_bits_equal(const Value *args, int numargs) {
	(void)numargs;
	if(args[1].isBits()) {
		Bits *a = args[0].toBits();
		Bits *b = args[1].toBits();
		if(a->size != b->size) {
			return ValueFalse;
		}
		return Value(std::memcmp(a->bytes, b->bytes,
		                         Bits::ChunkSizeByte * a->chunkcount) == 0);
	}
	return ValueFalse;
}

Value next_bits_unequal(const Value *args, int numargs) {
	(void)numargs;
	if(args[1].isBits()) {
		Bits *a = args[0].toBits();
		Bits *b = args[1].toBits();
		if(a->size != b->size) {
			return ValueTrue;
		}
		return Value(std::memcmp(a->bytes, b->bytes,
		                         Bits::ChunkSizeByte * a->chunkcount) != 0);
	}
	return ValueTrue;
}

Bits *Bits::create(int64_t number_of_bits) {
	Bits *b       = GcObject::allocBits();
	b->size       = number_of_bits;
	b->chunkcount = (number_of_bits >> Bits::ChunkCountShift) +
	                ((number_of_bits & Bits::ChunkRemainderAnd) != 0);
	b->chunkcapacity = b->chunkcount * Bits::ChunkSize;
	if(b->chunkcapacity == 0)
		b->chunkcapacity = Bits::ChunkSize;
	b->bytes = (Bits::ChunkType *)GcObject_malloc(
	    (b->chunkcapacity >> Bits::ChunkCountShift) * Bits::ChunkSizeByte);
	return b;
}

void Bits::resize(int64_t ns) {
	int64_t oldchunkcount = chunkcount;
	// readjust at all times
	chunkcount =
	    (ns >> Bits::ChunkCountShift) + ((ns & Bits::ChunkRemainderAnd) != 0);
	size = ns;
	// extend the capacity only if we are smaller
	if(ns > chunkcapacity) {
		bytes = (ChunkType *)GcObject_realloc(
		    bytes, oldchunkcount * ChunkSizeByte, chunkcount * ChunkSizeByte);
		chunkcapacity = ns;
	}
	// if we are shrinking, clear the extra chunks
	if(oldchunkcount > chunkcount) {
		std::memset(&bytes[chunkcount], 0,
		            (oldchunkcount - chunkcount) * Bits::ChunkSizeByte);
	}
	if(size & ChunkRemainderAnd) {
		// clear the extra part of the last chunk
		bytes[chunkcount - 1] &= ~(ChunkFull << (size & ChunkRemainderAnd));
	}
}

void Bits::init() {
	Class *BitsClass = GcObject::BitsClass;
	BitsClass->init("bits", Class::ClassType::BUILTIN);

	BitsClass->add_builtin_fn("()", 0, next_bits_construct_empty);
	BitsClass->add_builtin_fn("(_)", 1,
	                          next_bits_construct); // number of bits
	BitsClass->add_builtin_fn(
	    "(_,_)", 1,
	    next_bits_construct_value); // number of bits, default value
	BitsClass->add_builtin_fn("from(_)", 1, next_bits_from, false,
	                          true); // integer
	// planned API
	// also, all file *bytes operations should operate on bytes
	BitsClass->add_builtin_fn("set()", 0, next_bits_setall);
	BitsClass->add_builtin_fn("set(_)", 1, next_bits_set);
	BitsClass->add_builtin_fn("set(_,_)", 2, next_bits_setinrange);
	BitsClass->add_builtin_fn("reset()", 0, next_bits_resetall);
	BitsClass->add_builtin_fn("reset(_)", 1, next_bits_reset);
	BitsClass->add_builtin_fn("reset(_,_)", 2, next_bits_resetinrange);
	BitsClass->add_builtin_fn("toggle()", 0, next_bits_toggleall);
	BitsClass->add_builtin_fn("toggle(_)", 1, next_bits_toggle);
	BitsClass->add_builtin_fn("toggle(_,_)", 2, next_bits_toggleinrange);
	BitsClass->add_builtin_fn("size()", 0, next_bits_size);
	BitsClass->add_builtin_fn("bit(_)", 1, next_bits_bit);
	BitsClass->add_builtin_fn("str(_)", 1, next_bits_str);
	// bitwise operations, each of them produces
	// a new Bits
	BitsClass->add_builtin_fn("|(_)", 1, next_bits_or);
	BitsClass->add_builtin_fn("&(_)", 1, next_bits_and);
	BitsClass->add_builtin_fn("^(_)", 1, next_bits_xor);
	// leftshift expands, rightshift shrinks the size
	BitsClass->add_builtin_fn("<<(_)", 1, next_bits_lshift);
	BitsClass->add_builtin_fn(">>(_)", 1, next_bits_rshift);
	BitsClass->add_builtin_fn("not()", 0, next_bits_not);
	// bitwise operations, but they are in place,
	// modifies the source Bits, and returns that.
	// in place not() can be done by toggle()
	BitsClass->add_builtin_fn("or_with(_)", 1, next_bits_or_with);
	BitsClass->add_builtin_fn("and_with(_)", 1, next_bits_and_with);
	BitsClass->add_builtin_fn("xor_with(_)", 1, next_bits_xor_with);
	BitsClass->add_builtin_fn("shift_left(_)", 1, next_bits_shift_left);
	BitsClass->add_builtin_fn("shift_right(_)", 1, next_bits_shift_right);
	// subscript operators, only accept and return bit
	BitsClass->add_builtin_fn("[](_)", 1, next_bits_subs_get);
	BitsClass->add_builtin_fn("[](_,_)", 2, next_bits_subs_set);
	// iterators
	BitsClass->add_builtin_fn("iterate()", 0, next_bits_iterate);
	BitsClass->add_builtin_fn("as_bytes()", 0, next_bits_iterate_bytes);
	BitsClass->add_builtin_fn("as_ints()", 0, next_bits_iterate_ints);
	// converters, return tuple of values
	BitsClass->add_builtin_fn("to_bytes()", 0, next_bits_to_bytes);
	BitsClass->add_builtin_fn("to_ints()", 0, next_bits_to_ints);
	// insertion
	BitsClass->add_builtin_fn("insert(_)", 1, next_bits_insert);
	BitsClass->add_builtin_fn("insert_byte(_)", 1, next_bits_insert_byte);
	BitsClass->add_builtin_fn("insert_int(_)", 1, next_bits_insert_int);
	// equality and inequality
	BitsClass->add_builtin_fn("==(_)", 1, next_bits_equal);
	BitsClass->add_builtin_fn("!=(_)", 1, next_bits_unequal);
	// replace
	// BitsClass->add_builtin_fn("replace(_,_)", 2, next_bits_replace);
}
