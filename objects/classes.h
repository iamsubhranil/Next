#pragma once

#include "../func_utils.h"

#define OBJTYPE(r, n) struct r;
#include "../objecttype.h"

struct Number;
struct Boolean;
struct Nil;

// acts as a db of class pointers for all builtin types,
// does nothing else
struct Classes {

	template <typename T> using ZeroArg = void (T::*)();
	template <typename T> using GcRepr  = const String *(T::*)();
	template <typename T> struct ClassInfo {
		Class *    klass;
		ZeroArg<T> MarkFn, ReleaseFn, DependFn;
		GcRepr<T>  GcReprFn;

		// S has to obviously be equal to T, we just do
		// this here to delay the instantiation of this
		// function to a point at which we know the
		// concrete type of T, which is when the module
		// calls BuiltinModule::add_builtin_class, which in turn calls
		// this.
		template <typename S> void set(Class *c) {
			klass     = c;
			MarkFn    = FuncUtils::GetIfExists_mark<S, ZeroArg<T>>();
			ReleaseFn = FuncUtils::GetIfExists_release<S, ZeroArg<T>>();
			DependFn  = FuncUtils::GetIfExists_depend<S, ZeroArg<T>>();
			GcReprFn  = FuncUtils::GetIfExists_gc_repr<S, GcRepr<T>>();
		}
	};
	template <typename T> static Class *       get();
	template <typename T> static ClassInfo<T> *getClassInfo();
#define OBJTYPE(r, n) static ClassInfo<r> r##ClassInfo;
	OBJTYPE(Number, "")
	OBJTYPE(Boolean, "")
	OBJTYPE(Nil, "")
#include "../objecttype.h"
};

#define OBJTYPE(r, n)                                     \
	template <> Class *                Classes::get<r>(); \
	template <> Classes::ClassInfo<r> *Classes::getClassInfo<r>();
OBJTYPE(Number, "")
OBJTYPE(Boolean, "")
OBJTYPE(Nil, "")
#include "../objecttype.h"
