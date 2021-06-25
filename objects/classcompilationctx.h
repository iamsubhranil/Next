#pragma once
#include "../gc.h"
#include "class.h"
#include "map.h"
#include "set.h"

// everything that is needed during compilation
// is clubbed together in this struct.
// most of the members of this struct is only
// marked safe at compile time. at runtime, only
// members needed to report debug informations
// are kept, and everything else is unmarked, and
// freed in a subsequent gc.
struct ClassCompilationContext {
	struct MemberInfo {
		int  slot;
		bool isStatic;
		bool isDeclared;
	};
	typedef HashMap<Value, ClassCompilationContext::MemberInfo> MemberMap;

	GcObject obj;

	MemberMap *members;            // string:slot
	Map *      public_signatures;  // string:token to report overload errors
	Map *      private_signatures; // string:token
	Class *    compilingClass; // generated runtime representation of a class
	Class *    metaclass;      // metaclass of the class
	Map *      fctxMap; // a classctx also keeps track of all the function ctxes
	Map *      cctxMap; // a modulectx keeps track of classctxes declared inside
	// super context
	struct ClassCompilationContext *moduleContext;
	// for module
	FunctionCompilationContext *defaultConstructor;
	// slots
	int  slotCount;
	int  staticSlotCount;
	bool isCompiled;
	bool isDerived;

	static ClassCompilationContext *
	create(struct ClassCompilationContext *superContext, String *name);

	void add_public_class(Class *c, ClassCompilationContext *ctx = NULL);
	void add_private_class(Class *c, ClassCompilationContext *ctx = NULL);
	int  add_public_mem(Value name, bool isStatic = false, bool declare = true);
	int add_private_mem(Value name, bool isStatic = false, bool declare = true);
	bool has_mem(Value name);
	// unchecked. use has_mem before
	int        get_mem_slot(Value name);
	MemberInfo get_mem_info(Value name);
	bool       is_static_slot(Value name);

	bool add_public_fn(const String2 &sig, Function *f,
	                   FunctionCompilationContext *fctx = NULL);
	void add_public_signature(const String2 &sig, Function *f,
	                          FunctionCompilationContext *fctx);
	bool add_private_fn(const String2 &sig, Function *f,
	                    FunctionCompilationContext *fctx = NULL);
	void add_private_signature(const String2 &sig, Function *f,
	                           FunctionCompilationContext *fctx);
	bool has_fn(Value sig);
	// retrieve the ctx for a function
	FunctionCompilationContext *get_func_ctx(Value sig);
	// unchecked. use has_fn before
	int get_fn_sym(const String2 &sig);

	// check if a class exists
	bool                     has_class(String *name);
	ClassCompilationContext *get_class_ctx(String *name);
	// called for modules
	FunctionCompilationContext *get_default_constructor();
	void reset_default_constructor(); // resets the default constructor
	                                  // to a new one, called for repl

	Class *get_class();

	void finalize();

	void mark() {
		for(auto &i : *members) Gc::mark(i.first);
		Gc::mark(public_signatures);
		Gc::mark(private_signatures);
		Gc::mark(compilingClass);
		Gc::mark(fctxMap);
		if(metaclass != NULL) {
			Gc::mark(metaclass);
		}
		if(defaultConstructor != NULL) {
			Gc::mark(defaultConstructor);
		}
		if(cctxMap != NULL) {
			Gc::mark(cctxMap);
		}
		if(moduleContext != NULL) {
			Gc::mark(moduleContext);
		}
	}

	void release() {
		members->~MemberMap();
		Gc_free(members, sizeof(MemberMap));
	}
#ifdef DEBUG
	void disassemble(WritableStream &o);
#endif
#ifdef DEBUG_GC
	void          depend() { Gc::depend(compilingClass); }
	const String *gc_repr() { return compilingClass->name; }
#endif
};
