#pragma once

#include "value.h"
#include <cstdint>
#include <iomanip>
#include <memory>
#include <vector>

class BytecodeHolder {
  public:
	static const char *OpcodeNames[];
	std::vector<uint8_t> bytecodes;
	int                  stackMaxSize;
	int                  presentStackSize;
	int                  lastInsPos;
	BytecodeHolder()
	    : bytecodes(), stackMaxSize(0), presentStackSize(0), lastInsPos(0) {}

	enum Opcode : uint8_t {
#define OPCODE0(x, y) CODE_##x,
#define OPCODE1(w, x, y) OPCODE0(w, x)
#define OPCODE2(v, w, x, y) OPCODE0(v, w)
#include "opcodes.h"
	};

	void insertSlot() {
		presentStackSize++;
		stackMaxSize++;
	}

#define OPCODE0(x, y)                     \
	int x() {                             \
		stackEffect(y);                   \
		lastInsPos = bytecodes.size();    \
		bytecodes.push_back(CODE_##x);    \
		return lastInsPos;                \
	};                                    \
	int x(int pos) {                      \
		stackEffect(y);                   \
		lastInsPos            = pos;      \
		bytecodes[lastInsPos] = CODE_##x; \
		return lastInsPos;                \
	};

#define OPCODE1(x, y, z)               \
	int x(z arg) {                     \
		stackEffect(y);                \
		lastInsPos = bytecodes.size(); \
		bytecodes.push_back(CODE_##x); \
		insert_##z(arg);               \
		return lastInsPos;             \
	};                                 \
	int x(int pos, z arg) {            \
		stackEffect(y);                \
		lastInsPos     = pos;          \
		bytecodes[pos] = CODE_##x;     \
		insert_##z(pos + 1, arg);      \
		return pos;                    \
	};

#define OPCODE2(x, y, z, w)                    \
	int x(z arg1, w arg2) {                    \
		stackEffect(y);                        \
		lastInsPos = bytecodes.size();         \
		bytecodes.push_back(CODE_##x);         \
		insert_##z(arg1);                      \
		insert_##w(arg2);                      \
		return lastInsPos;                     \
	};                                         \
	int x(int pos, z arg1, w arg2) {           \
		stackEffect(y);                        \
		lastInsPos     = pos;                  \
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
	insert_type(uintptr_t);
	insert_type(Value);

#undef insert_type

	int getLastInsPos() const { return lastInsPos; }

	void setLastIns(uint8_t ins) { bytecodes[lastInsPos] = ins; }

	uint8_t getLastIns() const { return bytecodes[lastInsPos]; }

	void stackEffect(int effect) {
		presentStackSize += effect;
		if(presentStackSize > stackMaxSize)
			stackMaxSize = presentStackSize;
	}

	void disassemble_int(const uint8_t *data) const {
		std::cout << "\t" << *(int *)data;
	}

	void disassemble_double(const uint8_t *data) const {
		std::cout << "\t" << *(double *)data;
	}

	void disassemble_uintptr_t(const uint8_t *data) const {
		std::cout << "\t" << *(uintptr_t *)data;
	}

	void disassemble_NextString(const uint8_t *data) const {
		std::cout << "\t\"" << StringLibrary::get(*(NextString *)data) << "\"";
	}

	void disassemble_Value(const uint8_t *data) const {
		std::cout << "\t" << *(Value *)data;
	}

	void disassemble() const {
		const uint8_t *data = bytecodes.data();
		size_t   size = bytecodes.size();
		for(size_t i = 0; i < size;) {
			std::cout << std::setw(3) << i << ": " << std::setw(20)
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
	int            maxStackSize() const { return stackMaxSize; }
	int            getip() const { return bytecodes.size(); }
};

using Bytecode = std::unique_ptr<BytecodeHolder>;
