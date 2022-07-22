#include "regalloc.h"

RegisterAllocator::State *RegisterAllocator::FreeStates = NULL;

RegisterAllocator::RegisterAllocator(BytecodeCompilationContext *b, int scope) {
	for(int i = 0; i < NumRegisters; i++) {
		registers[i].state   = NULL;
		registers[i].isDirty = false;
	}
	btx                   = b;
	lastRegisterAllocated = 0;
	scopeID               = scope;

	currentStackBase = currentStackMax = currentStackPointer = NULL;
	currentStackCapacity                                     = 0;

	parent = NULL;
}

void RegisterAllocator::inherit(RegisterAllocator *p) {
	parent = p;

	for(int i = 0; i < NumRegisters; i++) {
		registers[i] = parent->registers[i];
	}

	btx                   = parent->btx;
	lastRegisterAllocated = parent->lastRegisterAllocated;
	scopeID               = parent->scopeID + 1;

	currentStackMax      = parent->currentStackMax;
	currentStackBase     = parent->currentStackBase;
	currentStackPointer  = parent->currentStackPointer;
	currentStackCapacity = parent->currentStackCapacity;
}

RegisterAllocator *RegisterAllocator::fork() {
	RegisterAllocator *r =
	    (RegisterAllocator *)Gc_malloc(sizeof(RegisterAllocator));
	r->inherit(this);
	return r;
}

void RegisterAllocator::merge(RegisterAllocator *child) {
	// restore state of parent
	child->finalize();

	lastRegisterAllocated = child->lastRegisterAllocated;

	currentStackMax      = child->currentStackMax;
	currentStackPointer  = child->currentStackPointer;
	currentStackCapacity = child->currentStackCapacity;
}

void RegisterAllocator::finalize() {
	if(parent) {
		// if we have a parent, we're bothered with only resetting
		// the registers in the way we found them
		for(int i = 0; i < NumRegisters; i++) {
			if(parent->registers[i].state != registers[i].state) {
				// two possible scenarios
				State *s1 = parent->registers[i].state;
				if(s1->type == State::Type::REGISTER) {
					// 1. old state is in another register
					// just exchange
					exchange(s1->idx, i);
				} else {
					// 2. old state is in the stack
					// exchange with the stack
					btx->xchgs(i, s1->idx);

					State *s2                 = registers[i].state;
					s2->type                  = State::Type::STACK;
					s2->idx                   = s1->idx;
					currentStackBase[s1->idx] = s2;

					s1->type           = State::Type::REGISTER;
					s1->idx            = i;
					registers[i].state = s1;
				}
			}
		}
		// now compare stacks, and write any updated values
		// to their old position
		for(State **start = currentStackBase; start < currentStackMax;
		    start++) {
			if(*start && (*start)->scopeID == scopeID) {
				// check the stack once again if this pointer is found
				// elsewhere
				for(State **newStart = currentStackBase;
				    newStart < currentStackMax; newStart++) {
					if(newStart == start && (*newStart)->scopeID < scopeID) {
						// emit code to copy the new state to the old state
						int oldSlot = newStart - currentStackBase;
						int newSlot = start - currentStackBase;
						btx->movs(newSlot, oldSlot);
						*start = NULL; // reset the old state
						break;
					}
				}
			}
		}
	}
}

void RegisterAllocator::exchange(int reg1, int reg2) {
	State *s1 = registers[reg1].state;
	State *s2 = registers[reg2].state;

	btx->xchg(reg1, reg2);

	s2->idx = reg1;
	s1->idx = reg2;

	registers[reg1].state = s2;
	registers[reg2].state = s1;

	s1->scopeID = s2->scopeID = scopeID;
}

int RegisterAllocator::getFirstFreeRegister() {
	for(int i = 0; i < NumRegisters; i++) {
		if(registers[i].state == NULL)
			return i;
	}
	return -1;
}

RegisterAllocator::State *RegisterAllocator::getNewState() {
	if(FreeStates) {
		State *ret = FreeStates;
		FreeStates = FreeStates->next;
		return ret;
	}
	return (State *)Gc_malloc(sizeof(State));
}

RegisterAllocator::State *RegisterAllocator::alloc(int reg, int slot,
                                                   bool canSkip) {
	reg                  = evict(reg, -1, canSkip);
	State *s             = getNewState();
	s->idx               = reg;
	s->slot              = slot;
	s->scopeID           = scopeID;
	registers[reg].state = s;
	if(reg > lastRegisterAllocated)
		lastRegisterAllocated = reg;
	return s;
}

int RegisterAllocator::evict(int reg, int slot, bool canSkip) {
	if(reg == -1) {
		// check if we already have a free register
		reg = getFirstFreeRegister();
		if(reg != -1)
			return reg;
		// we do not have a free register neither were
		// we provided one, so we'll evict the 0th
		// register
		reg = 0;
	}
	// get the register
	State *s = registers[reg].state;
	if(s == NULL) // if the register is already empty, return
		return reg;
	// mark the register as free now
	registers[reg].state   = NULL;
	registers[reg].isDirty = false;
	s->scopeID             = scopeID;
	// check if we have free register where we can move this value to
	int newreg = getFirstFreeRegister();
	if(newreg != -1) {
		// just issue a move and we're done
		btx->mov(reg, newreg);
		s->idx = newreg;
		return reg;
	}
	// we don't have free register, so move to stack
	// mark the value as stack variable
	s->type = State::Type::STACK;
	if(s->slot != -1) {
		// it is a local variable, so it must go to its place
		s->idx = s->slot;
		// only store the variable if it is dirty
		// if(registers[reg].isDirty) {
		btx->store_slot(s->slot, reg);
		// }
	} else {
		// this is a temporary variable, so we can push it to stack
		if(canSkip) {
			// we were asked to allow skipping the store of
			// temporary variables. so, we directly free
			// the state
			this->free(s);
		} else if(slot == -1) {
			// no predefined slots were given to us, so
			// get one for ourself.
			// we'll also need to monitor the stacksize somehow
			slot   = push(s);
			s->idx = slot;
			btx->store_slot(slot, reg);
		} else {
			// we were provided a slot to store us.
			// now if we do store_slot directly, that
			// slot will get overwritten. so, perform
			// an exchange instead.
			s->idx = slot;
			btx->xchgs(reg, slot);
		}
	}
	return reg;
}

int RegisterAllocator::move(State *s, int reg, bool canSkip) {
	s->scopeID = scopeID;
	// if the value is already in a register, we don't need to do much
	if(s->type == State::Type::REGISTER) {
		// check if the register matches or we don't need a specific
		// register
		if(s->idx == reg || reg == -1)
			return s->idx;
		s->scopeID = scopeID;
		// otherwise, check if the dest register is empty
		if(registers[reg].state == NULL) {
			// just issue a move
			btx->mov(s->idx, reg);
			s->idx                  = reg;
			registers[s->idx].state = NULL;
			registers[reg].state    = s;
		} else if(!canSkip || registers[reg].state->slot != -1) {
			// if the dest register is not empty,
			// and we're told not to skip any temp value
			// in the reg
			exchange(s->idx, reg);
		}
		return reg;
	}
	// value is in the stack, so get ourselves a register
	reg = evict(reg, s->idx, canSkip);
	// evict already performed the exchange for us if needed.
	// so we just need to change our state
	s->type = State::Type::REGISTER;
	s->idx  = reg;
	return reg;
}
