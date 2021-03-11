#include "core.h"
#include "../engine.h"
#include "../import.h"
#include "../loader.h"
#include "../printer.h"
#include "array.h"
#include "array_iterator.h"
#include "bits.h"
#include "bits_iterator.h"
#include "boundmethod.h"
#include "buffer.h"
#include "builtin_module.h"
#include "bytecode.h"
#include "bytecodecompilationctx.h"
#include "class.h"
#include "classcompilationctx.h"
#include "errors.h"
#include "fiber.h"
#include "file.h"
#include "function.h"
#include "functioncompilationctx.h"
#include "map.h"
#include "map_iterator.h"
#include "nil.h"
#include "range.h"
#include "range_iterator.h"
#include "set.h"
#include "set_iterator.h"
#include "symtab.h"
#include "tuple.h"
#include "tuple_iterator.h"

#include <time.h>

Value next_core_clock(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value((double)clock());
}

Value next_core_type_of(const Value *args, int numargs) {
	(void)numargs;
	Value v = args[1];
	return Value(v.getClass());
}

Value next_core_is_same_type(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[1].getClass() == args[2].getClass());
}

Value next_core_print(const Value *args, int numargs) {
	File2 f = File::create(Printer::StdOutStream);
	for(int i = 1; i < numargs; i++) {
		if(args[i].write(f) == ValueNil)
			return ValueNil;
	}
	return ValueNil;
}

Value next_core_println(const Value *args, int numargs) {
	next_core_print(args, numargs);
	Printer::print("\n");
	return ValueNil;
}

// called from the repl by the codegen implicitly,
// does not print implicit 'nil' values
Value next_core_printRepl(const Value *args, int numargs) {
	for(int i = 1; i < numargs; i++) {
		if(!args[i].isNil())
			next_core_println(&args[i - 1], 2);
	}
	return ValueNil;
}

Value next_core_str(const Value *args, int numargs) {
	(void)numargs;
	StringStream s;
	File2        f = File::create(s);
	if(args[1].write(f) == ValueNil)
		return ValueNil;
	return s.toString();
}

Value next_core_format(const Value *args, int numargs) {
	EXPECT(fmt, "fmt(_)", 1, String);
	// format is used everywhere in Next, and it does
	// expect the very first argument to be a string
	return Formatter::valuefmt(&args[1], numargs - 1);
}

Value next_core_yield_0(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	Fiber *f = ExecutionEngine::getCurrentFiber();
	// yield if we have somewhere to return to
	if(f->parent) {
		f->setState(Fiber::YIELDED);
		f->parent->setState(Fiber::RUNNING);
		ExecutionEngine::setCurrentFiber(f->parent);
	} else {
		// we don't allow calling yield on root fiber
		RERR("Calling yield on root fiber is not allowed!");
	}
	return ValueNil;
}

Value next_core_yield_1(const Value *args, int numargs) {
	next_core_yield_0(args, numargs);
	return args[1];
}

Value next_core_gc(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
	GcObject::gc(true);
	return ValueNil;
}

Value next_core_input0(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
	Utf8Source s    = Utf8Source(NULL);
	size_t     size = Printer::StdInStream.read(s);
	return String::from(s, size);
}

Value next_core_exit(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
	exit(0);
	return ValueNil;
}

Value next_core_exit1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(core, "exit(_)", 1, Integer);
	exit(args[1].toInteger());
	return ValueNil;
}

Value next_core_input1(const Value *args, int numargs) {
	EXPECT(core, "input(_)", 1, String);
	next_core_print(args, numargs);
	return next_core_input0(args, numargs);
}

Value next_core_resolve_partial_import(Value mod, const Value *parts,
                                       int numparts, int h) {
	// perform load_field on rest of the parts
	while(h < numparts) {
		if(mod.getClass()->has_fn(parts[h].toString())) {
			mod = mod.getClass()->accessFn(
			    mod.getClass(), mod, SymbolTable2::insert(parts[h].toString()));
		} else {
			String *s =
			    Formatter::fmt<Value>("Unable to import '{}' from '{}'!",
			                          parts[h], parts[h - 1])
			        .toString();
			if(s == NULL)
				return ValueNil;
			IMPORTERR(s);
		}
		h++;
	}
	return mod;
}

Value next_core_import_(String *currentPath, const Value *parts, int numparts) {
	ImportStatus is        = Importer::import(currentPath, parts, numparts);
	int          highlight = is.toHighlight;
	String2      fname     = is.fileName;
	switch(is.res) {
		case ImportStatus::BAD_IMPORT: {
			String2 s = Formatter::fmt(
			                "Folder '{}' does not exist or is not accessible!",
			                parts[highlight])
			                .toString();
			if(s == NULL)
				return ValueNil;
			IMPORTERR(s);
			break;
		}
		case ImportStatus::FOLDER_IMPORT: {
			String2 s =
			    Formatter::fmt("Importing folder '{}' is not supported!",
			                   parts[highlight])
			        .toString();
			if(s == NULL)
				return ValueNil;
			IMPORTERR(s);
			break;
		}
		case ImportStatus::FILE_NOT_FOUND: {
			String2 s =
			    Formatter::fmt("No such module '{}' found in the given folder!",
			                   parts[highlight])
			        .toString();
			if(s == NULL)
				return ValueNil;
			IMPORTERR(s);
			break;
		}
		case ImportStatus::FOPEN_ERROR: {
			String2 s = Formatter::fmt("Unable to open '{}' for reading: ",
			                           parts[highlight])
			                .toString();
			if(s == NULL)
				return ValueNil;
			// is.filename contains the strerror message
			IMPORTERR(String::append(s, fname));
			break;
		}
		case ImportStatus::BUILTIN_IMPORT: {
			Value mod = is.builtin;
			if(numparts > 1) {
				return next_core_resolve_partial_import(mod, parts, numparts,
				                                        1);
			}
			return mod;
		};
		case ImportStatus::PARTIAL_IMPORT:
		case ImportStatus::IMPORT_SUCCESS: {
			Loader2   loader = Loader::create();
			GcObject *m      = loader->compile_and_load(fname, true);
			if(m == NULL) {
				// compilation of the module failed
				IMPORTERR("Compilation of imported module failed!");
				break;
			}
			// if it is a partial import, load the rest
			// of the parts
			if(is.res == ImportStatus::PARTIAL_IMPORT) {
				return next_core_resolve_partial_import(m, parts, numparts,
				                                        is.toHighlight);
			}
			return m;
		}
	}
	return ValueNil;
}

Value next_core_import1(const Value *args, int arity) {
	// since this function cannot directly be called
	// by any function from next, we don't need to verify
	// here
	/*
	for(int i = 1; i < arity; i++) {
	    EXPECT(core, "import(_,...)", i, String);
	}*/
	// get the file name
	Fiber *   f  = ExecutionEngine::getCurrentFiber();
	Function *fu = f->callFrames[f->callFramePointer - 1].f;
	if(fu->getType() == Function::Type::BUILTIN) {
		RERR("Builtin functions cannot call import(_)!");
	}
	// get the file name from the bytecodecontext
	String2 fname = String::from(fu->code->ctx->ranges_[0].token.fileName);
	return next_core_import_(fname, &args[1], arity - 1);
}

bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool isDigit(char c) {
	return (c >= '0' && c <= '9');
}

bool isAlphaNumeric(char c) {
	return isAlpha(c) || isDigit(c);
}

Value next_core_import2(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(core, "import_file(_)", 1, String);
	// now we verify whether the import string followes
	// the grammar
	String *imp = args[1].toString();
	if(imp->size == 0) {
		IMPORTERR("Empty import string!");
	}
	Array2 parts = Array::create(1);
	// insert the core as object to the array
	parts->insert(args[0]);
	Utf8Source s    = imp->str();
	int        size = imp->len();
	for(int i = 0; i < size; i++, ++s) {
		// the first character must be an alphabet
		if(!isAlpha(*s)) {
			IMPORTERR("Part of an import string must start with a valid "
			          "alphabet or '_'!");
		}
		const void *bak = s.source;
		i++;
		s++;
		while(i < size && *s != '.') {
			if(!isAlphaNumeric(*s)) {
				IMPORTERR("Import string contains invalid character!");
			}
			i++;
			s++;
		}
		// create a part, and insert
		String *part = String::from(bak, (uintptr_t)s.source - (uintptr_t)bak);
		parts->insert(part);
		// if we're at the end, we're good
		if(i == size)
			break;
		// if we stumbled upon a '.', there must
		// be some more part to it
		else if(*s == '.') {
			if(i == size - 1) {
				IMPORTERR("Unexpected end of import string!");
			}
		}
	}
	// now finally, perform the import
	return next_core_import1(parts->values, parts->size);
}

void addCoreFunctions(BuiltinModule *m) {
	m->add_builtin_fn("clock()", 0, next_core_clock);
	m->add_builtin_fn("type_of(_)", 1, next_core_type_of);
	m->add_builtin_fn("is_same_type(_,_)", 2, next_core_is_same_type);
	m->add_builtin_fn("yield()", 0, next_core_yield_0, false);  // can switch
	m->add_builtin_fn("yield(_)", 1, next_core_yield_1, false); // can switch
	m->add_builtin_fn("gc()", 0, next_core_gc);
	m->add_builtin_fn("input()", 0, next_core_input0);
	m->add_builtin_fn("exit()", 0, next_core_exit);
	m->add_builtin_fn("exit(_)", 1, next_core_exit1);
	m->add_builtin_fn("input(_)", 1, next_core_input1, false); // can nest
	m->add_builtin_fn(" import(_)", 1, next_core_import1,
	                  true); // is va, can nest
	m->add_builtin_fn("import_file(_)", 1, next_core_import2,
	                  false); // can nest
	// all of these can nest
	m->add_builtin_fn("print()", 0, next_core_print, true);
	m->add_builtin_fn("println()", 0, next_core_println, true);
	m->add_builtin_fn("fmt(_)", 1, next_core_format, true);
	m->add_builtin_fn(" printRepl(_)", 1, next_core_printRepl, true);
	m->add_builtin_fn("str(_)", 1,
	                  next_core_str); // calls str(_) or str() appropriately
}

void addClocksPerSec(BuiltinModule *m) {
	m->add_builtin_variable("clocks_per_sec", Value(CLOCKS_PER_SEC));
}

void addCoreVariables(BuiltinModule *m) {
	addClocksPerSec(m);
}

void Core::preInit() {
	// first, create all the classes, and
	// then call init
	// this helps us not to rebind the objects
	// with their classes once they are created,
	// since all of the init functions of all
	// classes allocate some kind of object.
#define OBJTYPE(x, c)                  \
	Class2 x##Class = Class::create(); \
	BuiltinModule::register_hooks<x>(x##Class);
	// add primitive classes
	OBJTYPE(Number, "")
	OBJTYPE(Boolean, "")
	OBJTYPE(Nil, "")
#include "../objecttype.h"

	// initialize the string set and symbol table
	String::init0();
	SymbolTable2::init();

	// set the class of the temporaryObjects set
	// to the newly allocated one
	GcObject::temporaryObjects->obj.klass = MapClass;
}

void addCoreClasses(BuiltinModule *m) {
#define OBJTYPE(x, c) m->add_builtin_class<x>(Classes::get<x>(), c);
#include "../objecttype.h"
	// primitive classes are needed to be initialized after
	// the Class itself is done.
#define OBJTYPE(x, c) m->add_builtin_class<x>(Classes::get<x>(), c);
	OBJTYPE(Number, "number")
	OBJTYPE(Boolean, "bool")
	OBJTYPE(Nil, "nil")
}

void Core::init(BuiltinModule *m) {
	addCoreClasses(m);

	addCoreFunctions(m);
	addCoreVariables(m);
}
