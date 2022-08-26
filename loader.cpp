#include "loader.h"
#include "engine.h"
#include "objects/fiber.h"
#include "objects/functioncompilationctx.h"
#include "parser.h"
#include "printer.h"
#include <stdexcept>

Loader *Loader::create() {
	Loader *l      = Gc::alloc<Loader>();
	l->isCompiling = false;
	l->replModule  = ValueNil;
	return l;
}

GcObject *Loader::compile_and_load(const String2 &fileName, bool execute) {
	return compile_and_load(fileName->strb(), execute);
}

String *generateModuleName(const Utf8Source inp) {
	size_t       len      = inp.len();
	int          last     = len - 1;
	utf8_int32_t lastchar = inp + last;
	// find the '.'
	while(lastchar != '.' && lastchar != '\0') {
		lastchar = inp + (--last);
	}
	if((inp + last) == '\0')
		return String::from(inp.source, utf8size(inp.source));
	// escape the '.'
	--last;
	Utf8Source a = inp;
	a += last;
	int first = last;
	lastchar  = inp + first;
	while(lastchar != '\\' && lastchar != '/' && lastchar != '\0') {
		lastchar = inp + (--first);
	}
	if(lastchar != '\0')
		first++;
	Utf8Source s = inp;
	s += first;

	return String::from(s.source, (a - s) + 1);
}

GcObject *Loader::compile_and_load(const void *fileName, bool execute) {
	String2 modName = generateModuleName(Utf8Source(fileName));
	return compile_and_load_with_name(fileName, modName, execute);
}

GcObject *Loader::compile_and_load_with_name(const void *fileName,
                                             String *modName, bool execute) {
	String2   fname = String::from(fileName);
	GcObject *ret   = NULL;
#ifdef DEBUG
	StatementPrinter sp(Printer::StdOutStream);
#endif
	if(ExecutionEngine::isModuleRegistered(fname))
		return ExecutionEngine::getRegisteredModule(fname);
	Scanner s(fileName);
	try {
		::new(&parser) Parser(s);
		::new(&generator) CodeGenerator();
		isCompiling  = true;
		Array2 decls = parser.parseAllDeclarations();
		parser.releaseAll();
#ifdef DEBUG
		for(int i = 0; i < decls->size; i++) {
			sp.print(decls->values[i].toStatement());
			Printer::print("\n");
		}
		Printer::println("Parsed successfully!");
#endif
		ClassCompilationContext2 ctx =
		    ClassCompilationContext::create(NULL, modName);
		generator.compile(ctx, decls);
		isCompiling = false;
		if(execute) {
			Value v;
			if(ExecutionEngine::registerModule(
			       fname, ctx->get_default_constructor()->f, &v))
				ret = v.toGcObject();
		} else {
			ret = (GcObject *)ctx->get_class();
		}
	} catch(ParseException &pe) {
		if(pe.getToken().source.source != NULL) {
			Printer::LnErr(pe.getToken(), pe.what());
			pe.getToken().highlight(false, "", Token::HighlightType::ERR);
		}
	} catch(std::runtime_error &r) {
		Printer::println(r.what());
	}
	return ret;
}

Value Loader::compile_and_load_from_source(const void              *source,
                                           ClassCompilationContext *modulectx,
                                           Value mod, bool execute) {
#ifdef DEBUG
	StatementPrinter sp(Printer::StdOutStream);
#endif
	Scanner s(source, modulectx->get_class()->name->strb());
	replModule = mod;
	try {
		::new(&parser) Parser(s);
		::new(&generator) CodeGenerator();
		isCompiling  = true;
		Array2 decls = parser.parseAllDeclarations();
		parser.releaseAll();
#ifdef DEBUG
		for(int i = 0; i < decls->size; i++) {
			sp.print(decls->values[i].toStatement());
			Printer::print("\n");
		}
#endif
		int slots = modulectx->get_class()->numSlots;
		modulectx->reset_default_constructor();
		generator.compile(modulectx, decls);
		isCompiling = false;
		// if after compilation we have some new slots, we need
		// to reallocate the object
		if(mod.isObject()) {
			if(slots != modulectx->get_class()->numSlots) {
				Object *bak = Gc::allocObject(modulectx->get_class());
				// copy the old values
				Object *old = mod.toObject();
				for(int i = 0; i < slots; i++) {
					bak->slots()[i] = old->slots()[i];
				}
				// make it the new module
				replModule = mod = bak;
			}
		} else {
			replModule = mod = Gc::allocObject(modulectx->get_class());
		}
		if(execute) {
			ExecutionEngine::execute(
			    mod, modulectx->get_default_constructor()->f, &mod, true);
		}
	} catch(ParseException &pe) {
		if(pe.getToken().source.source != NULL) {
			Printer::LnErr(pe.getToken(), pe.what());
			pe.getToken().highlight(false, "", Token::HighlightType::ERR);
		}
	} catch(std::runtime_error &r) {
		Printer::println(r.what());
	}
	return mod;
}
