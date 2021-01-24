#include "core.h"
#include "../engine.h"
#include "../import.h"
#include "../loader.h"
#include "../printer.h"
#include "buffer.h"
#include "bytecodecompilationctx.h"
#include "classcompilationctx.h"
#include "errors.h"
#include "fiber.h"
#include "functioncompilationctx.h"
#include "symtab.h"

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
	for(int i = 1; i < numargs; i++) {
		String *s = String::toString(args[i]);
		// if we're unable to convert the value,
		// bail
		if(s == NULL)
			return ValueNil;
		Printer::print(s);
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
	Buffer<char> buffer;
	char         c;
	while((c = getchar()) != '\n' && c != 0) {
		buffer.insert(c);
	}
	return String::from(buffer.data(), buffer.size());
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

void add_builtin_fn(const char *n, int arity, next_builtin_fn fn,
                    bool isva = false, bool cannest = false) {
	String2   s = String::from(n);
	Function2 f = Function::from(s, arity, fn, isva);
	f->cannest  = cannest;
	GcObject::CoreContext->add_public_fn(s, f);
}

void Core::addCoreFunctions() {
	add_builtin_fn("clock()", 0, next_core_clock);
	add_builtin_fn("type_of(_)", 1, next_core_type_of);
	add_builtin_fn("is_same_type(_,_)", 2, next_core_is_same_type);
	add_builtin_fn("yield()", 0, next_core_yield_0, false, true);  // can switch
	add_builtin_fn("yield(_)", 1, next_core_yield_1, false, true); // can switch
	add_builtin_fn("gc()", 0, next_core_gc);
	add_builtin_fn("input()", 0, next_core_input0);
	add_builtin_fn("exit()", 0, next_core_exit);
	add_builtin_fn("exit(_)", 1, next_core_exit1);
	add_builtin_fn("input(_)", 1, next_core_input1, false, true); // can nest
	add_builtin_fn(" import(_)", 1, next_core_import1, true,
	               true); // is va, can nest
	add_builtin_fn("import_file(_)", 1, next_core_import2, false,
	               true); // can nest
	// all of these can nest
	add_builtin_fn("print()", 0, next_core_print, true, true);
	add_builtin_fn("println()", 0, next_core_println, true, true);
	add_builtin_fn("fmt(_)", 1, next_core_format, true, true);
	add_builtin_fn(" printRepl(_)", 1, next_core_printRepl, true, true);
}

void addClocksPerSec() {
	String2 n = String::from("clocks_per_sec");
	GcObject::CoreContext->add_public_mem(n);
	int s = GcObject::CoreContext->get_mem_slot(n);
	GcObject::CoreContext->defaultConstructor->bcc->push(Value(CLOCKS_PER_SEC));
	GcObject::CoreContext->defaultConstructor->bcc->store_object_slot(s);
}

void Core::addCoreVariables() {
	addClocksPerSec();
}

void Core::init(BuiltinModule *m) {
	(void)m;
}
