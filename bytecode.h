#pragma once

#include "value.h"
#include <cstdint>
#include <iomanip>
#include <memory>
#include <vector>

class BytecodeHolder {
  private:
	static const char *OpcodeNames[];

  public:
	std::vector<uint8_t> bytecodes;
	int                  stackMaxSize;
	int                  presentStackSize;

	BytecodeHolder() : bytecodes(), stackMaxSize(0), presentStackSize(0) {}

	enum Opcode : uint8_t {
#define OPCODE0(x, y) CODE_##x,
#define OPCODE1(w, x, y) OPCODE0(w, x)
#define OPCODE2(v, w, x, y) OPCODE0(v, w)
#include "opcodes.h"
	};

#define OPCODE0(x, y)                        \
	int x() {                                \
		presentStackSize += y;               \
		if(presentStackSize > stackMaxSize)  \
			stackMaxSize = presentStackSize; \
		bytecodes.push_back(CODE_##x);       \
		return bytecodes.size() - 1;         \
	};                                       \
	int x(int pos) {                         \
		presentStackSize += y;               \
		if(presentStackSize > stackMaxSize)  \
			stackMaxSize = presentStackSize; \
		bytecodes[pos] = CODE_##x;           \
		return pos;                          \
	};

#define OPCODE1(x, y, z)                     \
	int x(z arg) {                           \
		presentStackSize += y;               \
		if(presentStackSize > stackMaxSize)  \
			stackMaxSize = presentStackSize; \
		int pos = bytecodes.size();          \
		bytecodes.push_back(CODE_##x);       \
		insert_##z(arg);                     \
		return pos;                          \
	};                                       \
	int x(int pos, z arg) {                  \
		presentStackSize += y;               \
		if(presentStackSize > stackMaxSize)  \
			stackMaxSize = presentStackSize; \
		bytecodes[pos] = CODE_##x;           \
		insert_##z(pos + 1, arg);            \
		return pos;                          \
	};

#define OPCODE2(x, y, z, w)                    \
	int x(z arg1, w arg2) {                    \
		presentStackSize += y;                 \
		if(presentStackSize > stackMaxSize)    \
			stackMaxSize = presentStackSize;   \
		int pos = bytecodes.size();            \
		bytecodes.push_back(CODE_##x);         \
		insert_##z(arg1);                      \
		insert_##w(arg2);                      \
		return pos;                            \
	};                                         \
	int x(int pos, z arg1, w arg2) {           \
		presentStackSize += y;                 \
		if(presentStackSize > stackMaxSize)    \
			stackMaxSize = presentStackSize;   \
		bytecodes[pos] = CODE_##x;             \
		insert_##z(pos + 1, arg1);             \
		insert_##w(pos + 1 + sizeof(z), arg2); \
		return pos;                            \
	};
#include "opcodes.h"

#define insert_type(type)                                                     \
	int insert_##type(type x) {                                               \
		uint8_t *num = (uint8_t *)&x;                                         \
		for(size_t i = 0; i < sizeof(type); i++) bytecodes.push_back(num[i]); \
		return bytecodes.size() - sizeof(type);                               \
	}                                                                         \
	int insert_##type(int pos, type x) {                                      \
		uint8_t *num = (uint8_t *)&x;                                         \
		for(size_t i = 0; i < sizeof(type); i++)                              \
			bytecodes[pos + i] = (num[i]);                                    \
		return pos;                                                           \
	}

	insert_type(int);
	insert_type(NextString);
	insert_type(double);

#undef insert_type

	void disassemble_int(uint8_t *data) { std::cout << "\t" << *(int *)data; }

	void disassemble_double(uint8_t *data) {
		std::cout << "\t" << *(double *)data;
	}

	void disassemble_NextString(uint8_t *data) {
		std::cout << "\t\"" << StringLibrary::get(*(NextString *)data) << "\"";
	}

	void disassemble() {
		uint8_t *data = bytecodes.data();
		size_t   size = bytecodes.size();
		for(size_t i = 0; i < size;) {
			std::cout << std::setw(3) << i << ": " << std::setw(17)
			          << OpcodeNames[data[i]];
			switch(data[i]) {
#define OPCODE1(x, y, z)           \
	case CODE_##x:                 \
		i++;                       \
		disassemble_##z(&data[i]); \
		i += sizeof(z);            \
		break;
#define OPCODE2(w, x, y, z)        \
	case CODE_##w:                 \
		i++;                       \
		disassemble_##y(&data[i]); \
		i += sizeof(y);            \
		disassemble_##z(&data[i]); \
		i += sizeof(z);            \
		break;
#include "opcodes.h"
				default: i++; break;
			}
			std::cout << "\n";
		}
	}

	unsigned char *raw() { return bytecodes.data(); }
	int            maxStackSize() { return stackMaxSize; }
	int            getip() { return bytecodes.size(); }
};

using Bytecode = std::unique_ptr<BytecodeHolder>;
