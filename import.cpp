#include "import.h"
#include "filesystem/path.h"
#include "objects/array.h"
#include "objects/builtin_module.h"
#include "objects/file.h"
#ifdef DEBUG
#include "printer.h"
#endif

ImportStatus Importer::import(const String2 &currentPath, const Value *parts,
                              int size) {
	ImportStatus ret;
	ret.fileName    = String::from("<bad>");
	ret.res         = ImportStatus::BAD_IMPORT;
	ret.toHighlight = 0;

	auto             it = 0;
	filesystem::path p  = filesystem::path((const char *)currentPath->strb());
	if(!p.exists()) {
		// we take current path from the bytecode source
		// file itself. so this can only happen in case of
		// the repl.
		p = filesystem::path::getcwd();
	} else {
		p = p.make_absolute().parent_path();
	}
	// if p is not a directory, make the iterator look like 1,
	// because all the conditions below returns it - 1
	if(!p.is_directory())
		it++;
	else {
		while(p.is_directory() && it != size) {
			p = p /
			    filesystem::path((const char *)parts[it].toString()->strb());
			it++;
		}
	}
	if(p.is_directory() && it == size) {
		// wcout << "Unable to import whole folder '" << paths << "'!" << "\n";
		ret.res         = ImportStatus::FOLDER_IMPORT;
		ret.toHighlight = size - 1;
		return ret;
	}
	p = filesystem::path(p.str() + ".n");
	if(!p.exists()) {
		// wcout << "Unable to open file : '" << path_ << "'!" << "";

		// check if it is a builtin module
		int idx = -1;
		if((idx = BuiltinModule::hasBuiltinModule(parts[0].toString())) != -1) {
			ret.res      = ImportStatus::BUILTIN_IMPORT;
			ret.fileName = nullptr;
			ret.builtin  = BuiltinModule::initBuiltinModule(idx);
		} else {
			ret.res      = ImportStatus::FILE_NOT_FOUND;
			ret.fileName = String::from(p.str().c_str());
		}
		ret.toHighlight = it - 1;
		return ret;
	}
	if(!p.is_file()) {
		ret.res         = ImportStatus::BAD_IMPORT;
		ret.fileName    = String::from(p.str().c_str());
		ret.toHighlight = it - 1;
		return ret;
	}
#ifdef DEBUG
	Printer::println("File path generated : ", p.str().c_str());
#endif
	p                     = p.make_absolute();
	std::string absolutep = p.str();
	FILE *      f         = File::fopen(absolutep.c_str(), "r");
	if(f == NULL) {
		ret.res         = ImportStatus::FOPEN_ERROR;
		ret.fileName    = String::from(strerror(errno));
		ret.toHighlight = it - 1;
		return ret;
	}
	// if the whole path was resolved, it was a valid module
	// import, else, there are some parts we need to resolve
	// at runtime
	ret.res         = it == size ? ImportStatus::IMPORT_SUCCESS
	                             : ImportStatus::PARTIAL_IMPORT;
	ret.fileName    = String::from(absolutep.c_str());
	ret.toHighlight = it;
#ifdef DEBUG
	Printer::println(absolutep.c_str(), " imported successfully!");
#endif
	fclose(f);
	return ret;
}
