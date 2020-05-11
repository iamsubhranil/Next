#include "../gc.h"
#include "array.h"
#include "string.h"
#include <iostream>

using namespace std;
int main2() {
	cout << "Bootstrapping..\n";
	GcObject::init();
	// GcObject::print_stat();

	// Array *a = Array::create(10);

#ifdef DEBUG_GC
	GcObject::print_stat();
#endif
	// GcObject::print_stat();
	// GcObject::mark((GcObject *)a);
	// GcObject::mark((GcObject *)GcObject::ValueSetClass);
	GcObject::sweep();
	String::release_all();
#ifdef DEBUG_GC
	GcObject::print_stat();
#endif
}
