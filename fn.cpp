#include "fn.h"

using namespace std;

ostream &operator<<(ostream &os, const SlotVariable &s) {
	return os << s.slot;
}

Frame::Frame(Frame *p, Module *m) {
	parent = p;
	if(parent == nullptr) {
		scopeDepth = 0;
	} else
		scopeDepth = p->scopeDepth + 1;
	slotSize = 0;
	handlers = unq(std::vector<ExHandler>, 0);
	slots.clear();
	code        = BytecodeHolder();
	module      = m;
	// moduleStack = NULL;
}

int Frame::declareVariable(const char *name, int len, int scope) {
	slots[StringLibrary::insert(name, len)] = {slotSize++, true, scope};
	code.insertSlot();
	return slotSize - 1;
}

int Frame::declareVariable(NextString name, int scope) {
	slots[name] = {slotSize++, true, scope};
	code.insertSlot();
	return slotSize - 1;
}

bool Frame::hasVariable(NextString name) {
	if(slots.find(name) != slots.end()) {
		return slots.find(name)->second.isValid;
	}
	return false;
}

void Frame::insertdebug(Token t) {
	if(lineInfos.size()) {
		lineInfos.back().to = code.getip() - 1;
	}
	lineInfos.push_back(DebugInfo(code.getip(), code.getip(), t));
}

void Frame::finalizeDebug() {
	if(lineInfos.size()) {
		lineInfos.back().to = code.getip() - 1;
	}
}

void Frame::insertdebug(int from, Token t) {
	lineInfos.push_back(DebugInfo(from, from, t));
}

void Frame::insertdebug(int from, int to, Token t) {
	lineInfos.push_back(DebugInfo(from, to, t));
}

Token Frame::findLineInfo(const uint8_t *data) {
	int ip = data - code.bytecodes.data();
	for(auto i = lineInfos.begin(), j = lineInfos.end(); i != j; i++) {
		if(i->from <= ip && i->to >= ip)
			return i->t;
	}
	// TODO: THIS IS JUST WRONG
	return Token::PlaceholderToken;
}

ostream& operator<<(ostream& os, const Frame &f) {
	os << "Frame "
	   << " (slots : " << f.slotSize << " stacksize : " << f.code.maxStackSize()
	   << ")" << endl;
	for(auto const &i : f.slots) {
		os << "Slot #" << i.second << " : " << StringLibrary::get(i.first)
		   << endl;
	}
	os << "Exception handlers : " << (f.handlers->empty() ? "<empty>" : "")
	   << endl;
	for(auto i = f.handlers->begin(), j = f.handlers->end(); i != j; i++) {
		os << "From : " << (*i).from << "\tTo : " << (*i).to
		   << "\tType : " << StringLibrary::get((*i).caughtType.module) << "."
		   << StringLibrary::get((*i).caughtType.name)
		   << "\tIP : " << (*i).instructionPointer << endl;
	}
	f.code.disassemble();
	os << endl;
	return os;
}

FrameInstance::FrameInstance(Frame *f) {
	frame  = f;
	stack_ = (Value *)malloc(sizeof(Value) * f->code.maxStackSize());
	std::fill_n(stack_, f->code.maxStackSize(), Value::nil);
	stackPointer       = f->slotSize;
	presentSlotSize    = f->slotSize;
	instructionPointer = 0;
	code               = f->code.raw();
	enclosingFrame     = nullptr;
	// for module level instance,
	// f->module->frameInstance would be NULL
	if(f->module->frameInstance)
		moduleStack = f->module->frameInstance->stack_;
}

void FrameInstance::readjust(Frame *f) {
	// reallocate the stack
	stack_ = (Value *)realloc(stack_, sizeof(Value) * f->code.maxStackSize());
	// if there are new slots in the stack, make room
	if(presentSlotSize < f->slotSize) {
		// Calculate the number of new slots
		int moveup = f->slotSize - presentSlotSize;
		// Move stackpointer accordingly
		stackPointer += moveup;
		// Move all non-slot values up
		for(int i = stackPointer - 1; i > presentSlotSize; i--) {
			stack_[i] = stack_[i + 1];
		}
		presentSlotSize = f->slotSize;
	}
	// If there was an instance, it was halted
	// using 'halt', which does not increase
	// the pointer to point to next instruction.
	// We should do that first.
	// Also since 'bytecodes' vector can get
	// reallocated, especially in case of an REPL,
	// we recalculate that based on the saved
	// instruction pointer.
	code = &f->code.bytecodes.data()[instructionPointer + 1];
}

FrameInstance::~FrameInstance() {
	for(int i = 0; i < stackPointer; i++) {
		if(stack_[i].isObject())
			stack_[i].toObject()->decrCount();
	}
	free(stack_);
}

ostream &operator<<(ostream &os, const Fn &f) {
	os << "fn " << StringLibrary::get(f.name) << " (arity : " << f.arity
	   << ", isNative : " << f.isNative << ", isStatic : " << f.isStatic << ", "
	   << "isConstructor : " << f.isConstructor << ", vis : " << f.vis << ")"
	   << endl;
	os << *(f.frame.get());
	return os;
}

ostream &operator<<(ostream &os, const Variable &v) {
	return os << "Slot #" << v.slot << " (isStatic : " << v.isStatic << ")";
}

NextType NextClass::getClassType() {
	return NextType(module->name, name);
}

void NextClass::declareVariable(NextString name, Visibility vis, bool iss,
                                Token t) {
	Variable v(vis, t);
	v.isStatic         = iss;
	members[name]      = v;
	members[name].slot = slotNum++;
}

bool NextClass::hasVariable(NextString name) {
	return members.find(name) != members.end();
}

bool NextClass::hasPublicField(NextString name) {
	return members.find(name) != members.end() &&
	       members[name].vis == AccessModifiableEntity::PUB;
}

bool NextClass::hasPublicMethod(NextString name) {
	return functions.find(name) != functions.end() &&
	       functions[name]->vis == AccessModifiableEntity::PUB;
}

AccessModifiableEntity::Type NextClass::getEntityType() {
	return CLASS;
}

ostream &operator<<(ostream &os, const NextClass &n) {
	os << "Class " << StringLibrary::get(n.name) << endl;
	os << "==================" << endl;
	os << "Members : " << (n.members.empty() ? "<empty>" : "") << endl;
	for(auto const &i : n.members) {
		os << i.second << " : " << StringLibrary::get(i.first) << endl;
	}
	os << "Methods : " << (n.functions.empty() ? "<empty>" : "") << endl;
	for(auto const &i : n.functions) {
		os << "Method " << StringLibrary::get(i.first) << " : " << endl;
		os << *(i.second.get());
	}
	return os;
}

NextObject::NextObject(NextClass *c) {
	Class = c;
	slots = new Value[c->slotNum];
}

void NextObject::release() {
	for(int i = 0; i < Class->slotNum; i++) {
		if(slots[i].isObject())
			slots[i].toObject()->decrCount();
	}
	delete[] slots;
}

Module::Module(NextString n)
    : name(n), symbolTable(), functions(), variables(), importedModules(),
      classes() {
	frame         = unq(Frame, nullptr, this);
	frameInstance = NULL;
}

FrameInstance *Module::topLevelInstance() {
	if(frameInstance == NULL) {
		frameInstance = new FrameInstance(frame.get());
		initializeFramesWithModuleStack();
	}
	return frameInstance;
}

FrameInstance *Module::reAdjust(Frame *f) {
	Value *oldStack = frameInstance->stack_;
	frameInstance->readjust(f);
	if(oldStack != frameInstance->stack_) {
		initializeFramesWithModuleStack();
	}
	return frameInstance;
}

void Module::initializeFramesWithModuleStack() {
	/*
	for(auto i = frames.begin(), j = frames.end(); i != j; i++) {
	    (*i)->moduleStack = frameInstance->stack_;
	}*/
}

bool Module::hasCode() { // denotes whether the module has any top level code
	return !frame->code.bytecodes.empty();
}

bool Module::hasSignature(NextString n) {
	return symbolTable.find(n) != symbolTable.end();
}

bool Module::hasPublicFn(NextString n) {
	return functions.find(n) != functions.end() &&
	       functions[n]->vis == AccessModifiableEntity::PUB;
}

int Module::getIndexOfImportedFrame(Frame *f) {
	int i = 0;
	for(auto j = importedFrames.begin(), k = importedFrames.end(); j != k;
	    j++, i++) {
		if((*j) == f)
			return i;
	}
	importedFrames.push_back(f);
	return i;
}

bool Module::hasType(const NextString &n) {
	if(classes.find(n) != classes.end())
		return true;
	for(auto const &i : importedModules) {
		if(i.second->hasType(n))
			return true;
	}
	return false;
}

NextType Module::resolveType(const NextString &n) {
	if(classes.find(n) != classes.end()) {
		return NextType(name, n);
	}
	for(auto const &i : importedModules) {
		if(i.second->hasType(n))
			return i.second->resolveType(n);
	}
	return NextType::Error;
}

ostream &operator<<(ostream &os, const Module &m) {
	os << "Module " << StringLibrary::get(m.name) << endl;
	os << *(m.frame.get());
	os << "Functions : " << (m.functions.empty() ? "<empty>" : "") << endl;
	for(auto const &i : m.functions) {
		os << "Function " << StringLibrary::get(i.first) << endl;
		os << *(i.second.get());
	}
	os << "Classes : " << (m.classes.empty() ? "<empty>" : "") << endl;
	for(auto const &i : m.classes) {
		os << *(i.second.get());
	}
	return os;
}
