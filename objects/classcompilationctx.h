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
	typedef HashMap<String *, ClassCompilationContext::MemberInfo> MemberMap;

	GcObject   obj;
	MemberMap *members;            // string:slot
	ValueMap * public_signatures;  // string:token to report overload errors
	ValueMap * private_signatures; // string:token
	Class *    klass;     // generated runtime representation of a class
	Class *    metaclass; // metaclass of the class
	ValueMap * fctxMap; // a classctx also keeps track of all the function ctxes
	ValueMap * cctxMap; // a modulectx keeps track of classctxes declared inside
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
	int  add_public_mem(String *name, bool isStatic = false,
	                    bool declare = true);
	int  add_private_mem(String *name, bool isStatic = false,
	                     bool declare = true);
	bool has_mem(String *name);
	// unchecked. use has_mem before
	int        get_mem_slot(String *name);
	MemberInfo get_mem_info(String *name);
	bool       is_static_slot(String *name);

	bool add_public_fn(const String2 &sig, Function *f,
	                   FunctionCompilationContext *fctx = NULL);
	void add_public_signature(const String2 &sig, Function *f,
	                          FunctionCompilationContext *fctx);
	bool add_private_fn(const String2 &sig, Function *f,
	                    FunctionCompilationContext *fctx = NULL);
	void add_private_signature(const String2 &sig, Function *f,
	                           FunctionCompilationContext *fctx);
	bool has_fn(String *sig);
	// retrieve the ctx for a function
	FunctionCompilationContext *get_func_ctx(String *sig);
	// unchecked. use has_fn before
	int get_fn_sym(const String2 &sig);

	// check if a class exists
	bool                     has_class(String *name);
	ClassCompilationContext *get_class_ctx(String *name);
	// called for modules
	FunctionCompilationContext *get_default_constructor();

	Class *get_class();

	void finalize();

	static void init();

	void mark() const {
		for(auto &i : *members) GcObject::mark(i.first);
		GcObject::mark(public_signatures);
		GcObject::mark(private_signatures);
		GcObject::mark(klass);
		GcObject::mark(fctxMap);
		if(metaclass != NULL) {
			GcObject::mark(metaclass);
		}
		if(defaultConstructor != NULL) {
			GcObject::mark(defaultConstructor);
		}
		if(cctxMap != NULL) {
			GcObject::mark(cctxMap);
		}
		if(moduleContext != NULL) {
			GcObject::mark(moduleContext);
		}
	}

	void release() const {
		members->~MemberMap();
		GcObject_free(members, sizeof(MemberMap));
	}
#ifdef DEBUG
	void disassemble(std::ostream &o);
#endif
#ifdef DEBUG_GC
	const char *gc_repr() { return klass->name->str(); }
#endif
};
