#include "engine.h"
#include "display.h"

using namespace std;

ExecutionEngine::ExecutionEngine() {}

FrameInstance *ExecutionEngine::newinstance(Frame *f) {
	return new FrameInstance(f);
}

// using BytecodeHolder::Opcodes;
void ExecutionEngine::printStackTrace(FrameInstance *f) {
	while(f != NULL) {
		f->frame->lineInfos.begin()->t.highlight();
		f = f->enclosingFrame;
	}
}

void ExecutionEngine::execute(Module *m, Frame *f) {
	(void)m;
	// FrameInstancePtr presentFramePtr = newinstance(f);
	FrameInstance *presentFrame = newinstance(f);

#ifdef NEXT_USE_COMPUTED_GOTO
	static const void *dispatchTable[] = {
#define OPCODE0(x, y) &&EXEC_CODE_##x,
#define OPCODE1(x, y, z) OPCODE0(x, y)
#define OPCODE2(w, x, y, z) OPCODE0(w, x)
#include "opcodes.h"
	};

#define LOOP() while(1)
#define SWITCH() \
	{ goto *dispatchTable[*presentFrame->code]; }
#define CASE(x) EXEC_CODE_##x
#define DISPATCH() \
	{ goto *dispatchTable[*(++presentFrame->code)]; }
#define DISPATCH_WINC() \
	{ goto *dispatchTable[*presentFrame->code]; }
#else
#define LOOP() while(*presentFrame->code != BytecodeHolder::CODE_halt)
#define SWITCH() switch(*presentFrame->code)
#define CASE(x) case BytecodeHolder::CODE_##x
#define DISPATCH()            \
	{                         \
		presentFrame->code++; \
		continue;             \
	}
#define DISPATCH_WINC() \
	{ continue; }
#endif

#define TOP presentFrame->stack_[presentFrame->stackPointer - 1]
#define PUSH(x) presentFrame->stack_[presentFrame->stackPointer++] = (x);
#define POP() presentFrame->stack_[--presentFrame->stackPointer]
#define RERR(x)                                                          \
	{                                                                    \
		printStackTrace(presentFrame);                                   \
		lnerr(x, presentFrame->frame->findLineInfo(presentFrame->code)); \
	}

#define JUMPTO(x)                \
	{                            \
		presentFrame->code += x; \
		continue;                \
	}

#define next_int()                      \
	(presentFrame->code += sizeof(int), \
	 *(int *)((presentFrame->code - sizeof(int) + 1)))
#define next_double()                      \
	(presentFrame->code += sizeof(double), \
	 *(double *)((presentFrame->code - sizeof(double) + 1)))
#define next_string()                          \
	(presentFrame->code += sizeof(NextString), \
	 *(NextString *)(presentFrame->code - sizeof(NextString) + 1))
#define next_ptr()                            \
	(presentFrame->code += sizeof(uintptr_t), \
	 *(uintptr_t *)(presentFrame->code - sizeof(uintptr_t) + 1))
	// std::cout << "x : " << TOP << " y : " << v << " op : " << #op <<
	// std::endl;

#define binary(op, name, argtype, restype)                                \
	{                                                                     \
		Value v = POP();                                                  \
		if(v.is##argtype() && TOP.is##argtype()) {                        \
			TOP.set##restype(TOP.to##argtype() op v.to##argtype());       \
			DISPATCH();                                                   \
		}                                                                 \
		RERR("Both of the operands of " #name " must be a " #argtype "!") \
	}

	LOOP() {
		SWITCH() {

			CASE(add) : binary(+, addition, Number, Number);
			CASE(sub) : binary(-, subtraction, Number, Number);
			CASE(mul) : binary(*, multiplication, Number, Number);
			CASE(div) : binary(/, division, Number, Number);
			CASE(lor) : binary(||, or, Boolean, Boolean);
			CASE(land) : binary(&&, and, Boolean, Boolean);
			CASE(eq) : binary(==, equals to, Number, Boolean);
			CASE(neq) : binary(!=, not equals to, Number, Boolean);
			CASE(greater) : binary(>, greater than, Number, Boolean);
			CASE(greatereq)
			    : binary(>=, greater than or equals to, Number, Boolean);
			CASE(less) : binary(<, lesser than, Number, Boolean);
			CASE(lesseq)
			    : binary(<=, lesser than or equals to, Number, Boolean);
			CASE(power) : RERR("Yet not implemented!");

			CASE(lnot) : {
				if(TOP.isBoolean()) {
					TOP.setBoolean(!TOP.toBoolean());
					DISPATCH();
				}
				RERR("'!' can only be applied over boolean value!");
			}

			CASE(neg) : {
				if(TOP.isNumber()) {
					TOP = -TOP.toNumber();
					DISPATCH();
				}
				RERR("'-' must only be applied over a number!");
			}

			CASE(pushd) : {
				PUSH(Value(next_double()));
				DISPATCH();
			}

			CASE(pushs) : {
				PUSH(Value(next_string()));
				DISPATCH();
			}

			CASE(pop) : {
				POP();
				DISPATCH();
			}

			CASE(jump) : {
				JUMPTO(next_int() -
				       sizeof(int)); // offset the relative jump address
			}

			CASE(jumpiftrue) : {
				Value v   = POP();
				int   dis = next_int();
				bool  tr  = (v.isBoolean() && v.toBoolean()) ||
				          (v.isNumber() && v.toNumber());
				if(tr) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(jumpiffalse) : {
				Value v   = POP();
				int   dis = next_int();
				bool  tr  = (v.isBoolean() && !v.toBoolean()) ||
				          (v.isNumber() && !v.toNumber());
				if(tr) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(print) : {
				Value v = POP();
				std::cout << v;
				DISPATCH();
			}

			CASE(call) : {
				// NextString       fn = next_string();
				FrameInstance *f = newinstance(m->frames[next_int()]);
				// Copy the arguments
				int numArg = next_int() - 1;
				while(numArg >= 0) {
					f->stack_[numArg] = POP();
					numArg--;
				}
				f->enclosingFrame = presentFrame;
				presentFrame      = f;
				DISPATCH_WINC();
			}

			CASE(ret) : {
				// Pop the return value
				Value          v   = POP();
				FrameInstance *bak = presentFrame->enclosingFrame;
				delete presentFrame;
				presentFrame = bak;
				PUSH(v);
				DISPATCH();
			}

			CASE(load_slot) : {
				PUSH(presentFrame->stack_[next_int()]);
				DISPATCH();
			}

			CASE(store_slot) : {
				// Do not pop the value off the stack yet
				presentFrame->stack_[next_int()] = TOP;
				DISPATCH();
			}

			CASE(incr_prefix) : {
				Value &v = presentFrame->stack_[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					PUSH(v);
					DISPATCH();
				}
				RERR("'++' can only be applied on a number!");
			}

			CASE(decr_prefix) : {
				Value &v = presentFrame->stack_[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					PUSH(v);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(incr_postfix) : {
				Value &v = presentFrame->stack_[next_int()];
				if(v.isNumber()) {
					PUSH(v);
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERR("'++' can only be applied on a number!");
			}

			CASE(decr_postfix) : {
				Value &v = presentFrame->stack_[next_int()];
				if(v.isNumber()) {
					PUSH(v);
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(halt) : { break; }
		}
	}
}
