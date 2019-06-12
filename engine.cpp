#include "engine.h"
#include "display.h"

using namespace std;

ExecutionEngine::ExecutionEngine() {}

FrameInstancePtr ExecutionEngine::newinstance(Frame *f) {
	return unq(FrameInstance, f);
}

// using BytecodeHolder::Opcodes;
void ExecutionEngine::printStackTrace(FrameInstance *f) {
	while(f != NULL) {
		f->frame->lineInfos.begin()->t.highlight();
		if(f->enclosingFrame == nullptr)
			return;
		f = f->enclosingFrame.get();
	}
}

void ExecutionEngine::execute(Module *m, Frame *f) {
	(void)m;
	FrameInstancePtr presentFrame = newinstance(f);

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
#define TOP presentFrame->stack_.back()
#define PUSH(x) presentFrame->stack_.push_back((x));
#define POP() presentFrame->stack_.pop_back()
#define RERR(x)                                                          \
	{                                                                    \
		printStackTrace(presentFrame.get());                             \
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
	// std::cout << "x : " << TOP << " y : " << v << " op : " << #op <<
	// std::endl;

#define binary(op, name, type)                                         \
	{                                                                  \
		Value v = TOP;                                                 \
		POP();                                                         \
		if(v.is##type() && TOP.is##type()) {                           \
			TOP = Value(TOP.to##type() op v.to##type());               \
			DISPATCH();                                                \
		}                                                              \
		RERR("Both of the operands of " #name " must be a " #type "!") \
	}

	LOOP() {
		SWITCH() {

			CASE(add) : binary(+, addition, Number);
			CASE(sub) : binary(-, subtraction, Number);
			CASE(mul) : binary(*, multiplication, Number);
			CASE(div) : binary(/, division, Number);
			CASE(lor) : binary(||, or, Boolean);
			CASE(land) : binary(&&, and, Boolean);
			CASE(eq) : binary(==, equals to, Number);
			CASE(neq) : binary(!=, not equals to, Number);
			CASE(greater) : binary(>, greater than, Number);
			CASE(greatereq) : binary(>=, greater than or equals to, Number);
			CASE(less) : binary(<, lesser than, Number);
			CASE(lesseq) : binary(<=, lesser than or equals to, Number);
			CASE(power) : RERR("Yet not implemented!");

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

			CASE(jump) : {
				JUMPTO(next_int() -
				       sizeof(int)); // offset the relative jump address
			}

			CASE(jumpiftrue) : {
				Value v = TOP;
				POP();
				int dis = next_int();
				if((v.isBoolean() && v.toBoolean()) ||
				   (v.isNumber() && v.toNumber() != 0)) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(jumpiffalse) : {
				Value v = TOP;
				POP();
				int dis = next_int();
				if((v.isBoolean() && !v.toBoolean()) ||
				   (v.isNumber() && v.toNumber() == 0)) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(print) : {
				Value v = TOP;
				POP();
				std::cout << v;
				DISPATCH();
			}

			CASE(call) : {
				NextString       fn = next_string();
				FrameInstancePtr f = newinstance(m->functions[fn]->frame.get());
				// Copy the arguments
				int numArg = next_int() - 1;
				while(numArg >= 0) {
					f->stack_[numArg] = TOP;
					POP();
					numArg--;
				}
				f->enclosingFrame   = FrameInstancePtr(presentFrame.release());
				presentFrame        = FrameInstancePtr(f.release());
				DISPATCH_WINC();
			}

			CASE(ret) : {
				// Pop the return value
				Value v = TOP;
				presentFrame =
				    FrameInstancePtr(presentFrame->enclosingFrame.release());
				PUSH(v);
				DISPATCH();
			}

			CASE(load_slot) : {
				PUSH(presentFrame->stack_[next_int()]);
				DISPATCH();
			}

			CASE(store_slot) : {
				Value v = TOP;
				POP();
				presentFrame->stack_[next_int()] = v;
				DISPATCH();
			}

			CASE(halt) : { break; }
		}
	}
}
