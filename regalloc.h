#pragma once

#include "objects/bytecodecompilationctx.h"

#define NUM_REGISTERS 32

struct RegisterAllocator {
	static const int NumRegisters = NUM_REGISTERS;
	static const int Accumulator  = 0;
	// details of a value stored in a memory.
	// if type is STACK, it denotes that the value
	// is stored into the 'idx' offset from
	// the base of the stack.
	// if it is REGISTER, it denotes that the
	// value is stored in the designated register.
	union State {
		// required values of an allocated state
		enum class Type { STACK, REGISTER };
		struct {
			Type type;
			int  idx;
			// if this is a local variable, slot denotes
			// the slot of the variable in the stack.
			// this is required because while eviction,
			// local variables will go into their
			// designated slots instead of getting pushed
			// onto the TOS.
			// if this is a temporary variable, the slot
			// is set to -1.
			int slot;
			// scopeID which last modified the state
			int scopeID;
		};

		// pointer to the next free state
		State *next;
	};

	struct ScopedState {
		State *s;

		ScopedState(State *st) { s = st; }

		operator State *() { return s; }

		ScopedState &operator=(State *st) {
			release();
			s = st;
			return *this;
		}

		ScopedState &operator=(ScopedState &r) {
			release();
			s   = r.s;
			r.s = NULL;
			return *this;
		}

		void release() {
			if(s)
				RegisterAllocator::freeUnused(s);
			s = NULL;
		}
	};
	// free states are globally managed by the allocator,
	// to reduce memory usage
	static State *FreeStates;
	static State *getNewState();

	// A set of registers is nothing but a collection
	// of value states. If registers[i]->state == NULL, that
	// denotes that the register is empty.
	struct Register {
		State *state;
		// this denotes that this register is written to.
		// this is useful for backing up the register
		// in stack in case of eviction.
		bool isDirty;
	} registers[NumRegisters];

	// While using any register for the first time, we will push
	// its value into the stack to keep a backup. We will
	// pop that value back when the function returns.
	// Now, since, the allocation will start from reg 1 and go
	// all the way upto reg max, for now we can assume that
	// the registers will be pushed in that order in the stack.
	// So, we can pop in the reverse order to make the registers
	// intact for the caller. Hence, this variable denotes the last
	// register we used in this function context, i.e. the first
	// register to restore on return.
	int lastRegisterAllocated;

	// returns the index of the first free register
	// returns -1 if all the registers are occupied
	int getFirstFreeRegister();

	// alloc a register for a variable.
	// if reg is provided, only that particular register
	// is allocated.
	// if slot is provided, the created state carries
	// that particular slot.
	// if canSkip is true, instructs the evict function
	// to skip backing up of temporary variables in the
	// register.
	State *alloc(int reg = -1, int slot = -1, bool canSkip = false);

	// evict any register to stack to free up.
	// allocator will emit code to do so.
	// returns the free register.
	// right now, the eviction policy is simply freeing the first register,
	// since that may be the oldest one.
	//
	// argument 'slot' denotes if there is a predefined
	// empty slot where the evicted value may go. if it is
	// -1, we'll use the current stack top.
	// returns reg itself.
	// if 'canSkip' is true, the backup operation will be skipped
	// if the destination register contains a State which has
	// slot == -1, i.e. a temporary variable.
	int evict(int reg = -1, int slot = -1, bool canSkip = false);

	// move a particular value into a register.
	// allocator will emit code to do so if required.
	// returns the reg no.
	// if canSkip is true, instructs the evict function
	// to skip backing up of temporary variables in the
	// register.
	int move(State *s, int reg = -1, bool canSkip = false);

	// adds a state to the freelist.
	// does not change any stack or registers.
	static void free(State *s);
	static void freeUnused(State *s) {
		if(s->slot == -1)
			free(s);
	}

	// exchanges the states of two registers, emits code
	void exchange(int reg1, int reg2);

	// we need this because we will emit codes for pushing
	// and popping stuff
	BytecodeCompilationContext *btx;

	RegisterAllocator(BytecodeCompilationContext *b, int scopeID);

	void finalize();

	void markFree(int reg);  // mark the register as free
	void markDirty(int reg); // mark the register as dirty

	// we also need to backup the state of the environment per
	// block, including registers and stuff. so we propose
	// the following rule:
	// 1. each block gets its own register allocator, which
	// derives from the parent allocator, carrying its information
	// alongside a few restrictions
	// 2. each State will have a scope identifier, which will
	// mark the scope which moved a state to its current position
	// 3. each allocator will carry a scopeID, which it will use
	// to mark the states it modifies
	// 4. an allocator with scopeID 'i' cannot modify any stack
	// variables with scopeID < 'i'.
	// 5. an allocator with scopeID 'i' can however move registers
	// freely as it wants
	// 6. each allocator will get its own copy of registers, however
	// they will share the same stack, and their registers will be
	// copied down from the parent.
	// 7. at the end of the block, the child allocator will emit code
	// for resetting all the registers to the parent configuration
	// by traversing both sets of registers simultaneously.
	// 8. after restting the registers, the child allocator will also
	// emit code for copying the updated values of any variable modified
	// which was present on the old stack.

	// scopeID
	int scopeID;

	// stack
	State **currentStackBase;
	State **currentStackPointer;
	State **currentStackMax;
	int     currentStackCapacity;

	int getFirstFreeStack();
	int push(State *s); // returns the slot idx

	// forks a child register allocator which carries the parent
	// information
	RegisterAllocator *fork();

	// called in the child allocator to inherit information
	// from the parent allocator
	void inherit(RegisterAllocator *parent);

	// finalizes the child allocator
	void merge(RegisterAllocator *child);

	RegisterAllocator *parent;
};
