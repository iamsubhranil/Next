#include "fn.h"

using namespace std;

ostream &operator<<(ostream &os, const SlotVariable &s) {
	return os << s.slot;
}

ostream& operator<<(ostream& os, const Frame &f) {
	os << "Frame "
	   << " (slots : " << f.slotSize << " stacksize : " << f.code.maxStackSize()
	   << ")" << endl;
	for(auto i = f.slots.begin(), j = f.slots.end(); i != j; i++) {
		os << "Slot #" << (*i).second << " : " << StringLibrary::get((*i).first)
		   << endl;
	}
	f.code.disassemble();
	os << endl;
	return os;
}

ostream &operator<<(ostream &os, const Fn &f) {
	os << "fn " << StringLibrary::get(f.name) << " (arity : " << f.arity
	   << ", isNative : " << f.isNative << ", isStatic : " << f.isStatic << ", "
	   << "isConstructor : " << f.isConstructor << ")" << endl;
	os << *(f.frame.get());
	return os;
}

ostream &operator<<(ostream &os, const Variable &v) {
	return os << "Slot #" << v.slot << " (isStatic : " << v.isStatic << ")";
}

ostream &operator<<(ostream &os, const NextClass &n) {
	os << "Class " << StringLibrary::get(n.name) << endl;
	os << "==================" << endl;
	os << "Members : " << endl;
	for(auto i = n.members.begin(), j = n.members.end(); i != j; i++) {
		os << (*i).second << " : " << StringLibrary::get((*i).first) << endl;
	}
	os << "Methods : " << endl;
	for(auto i = n.functions.begin(), j = n.functions.end(); i != j; i++) {
		os << "Method " << StringLibrary::get((*i).first) << " : " << endl;
		os << *((*i).second.get());
	}
	return os;
}

ostream &operator<<(ostream &os, const Module &m) {
	os << "Module " << StringLibrary::get(m.name) << endl;
	os << *(m.frame.get());
	os << "Functions : " << endl;
	for(auto i = m.functions.begin(), j = m.functions.end(); i != j; i++) {
		os << "Function " << StringLibrary::get((*i).first) << endl;
		os << *((*i).second.get());
	}
	os << "Classes : " << endl;
	for(auto i = m.classes.begin(), j = m.classes.end(); i != j; i++) {
		os << *((*i).second.get());
	}
	return os;
}
