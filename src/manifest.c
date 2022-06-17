#include "z64playas.h"

static void
Wren_Print(WrenVM* vm, const char* str) {
	printf("%s", str);
}

static void
Wren_Error(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message) {
	switch (type) {
		case WREN_ERROR_COMPILE:
			fprintf(stderr, "[" PRNT_REDD "Compile Error" PRNT_RSET "] [%s::%d]\a\n", module, line);
			fprintf(stderr, "%s\n", message);
			break;
		case WREN_ERROR_RUNTIME:
			fprintf(stderr, "[" PRNT_REDD "Runtime Error" PRNT_RSET "]\a\n");
			fprintf(stderr, "%s\n", message);
			break;
		case WREN_ERROR_STACK_TRACE:
			fprintf(stderr, "[" PRNT_REDD "Stack Trace" PRNT_RSET "] [%s::%d]\a\n", module, line);
			fprintf(stderr, "%s\n", message);
			break;
	}
}

static const char*
Wren_ResolveModule(WrenVM* vm, const char* importer, const char* name) {
	Log("ResolveModule: %s - %s", importer, name);
	
	if (!strcmp(name, "z64playas"))
		return StrDup(HeapPrint("%sMainModule.mnf", Sys_AppDir()));
	
	return StrDup(HeapPrint("%s.mnf", name));
}

static void
Wren_ModuleOnComplete(WrenVM* vm, const char* name, struct WrenLoadModuleResult result) {
	MemFile_Free(result.userData);
}

static WrenLoadModuleResult
Wren_LoadModule(WrenVM* vm, const char* module) {
	static MemFile mem;
	
	Log("LoadModule: %s", module);
	
	MemFile_LoadFile_String(&mem, module);
	
	return (WrenLoadModuleResult) {
		       .onComplete = Wren_ModuleOnComplete,
		       .source = mem.str,
		       .userData = &mem
	};
}

static WrenForeignMethodFn
Wren_ForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) {
	Log("ForeignMethod: %s %s %s", module, className, signature);
	
	if (!strcmp(className, "ZObject")) {
		if (!strcmp(signature, "writeEntry(_,_,_)"))
			return Method_ZObject_DictEntry;
	}
	
	if (!strcmp(className, "LutTable")) {
		if (!strcmp(signature, "setProperties(_,_)"))
			return Method_Lut_SetTable;
		if (!strcmp(signature, "entry(_)"))
			return Method_Lut_Entry;
		if (!strcmp(signature, "writeMtx(_,_,_,_,_,_,_,_,_)"))
			return Method_Lut_WriteMtx;
		if (!strcmp(signature, "popMtx()"))
			return Method_Lut_PopMtx;
		if (!strcmp(signature, "call(_)"))
			return Method_Lut_Call;
	}
	
	return NULL;
}

s32 Wren_Run(const char* script, PlayAsState* state) {
	WrenInterpretResult res;
	WrenConfiguration config;
	WrenVM* vm;
	MemFile mem;
	
	wrenInitConfiguration(&config);
	
	config.writeFn = Wren_Print;
	config.errorFn = Wren_Error;
	config.resolveModuleFn = Wren_ResolveModule;
	config.loadModuleFn = Wren_LoadModule;
	config.bindForeignMethodFn = Wren_ForeignMethod;
	config.initialHeapSize = MbToBin(16.0);
	config.userData = state;
	
	MemFile_LoadFile_String(&mem, script);
	
	vm = wrenNewVM(&config);
	res = wrenInterpret(vm, script, mem.data);
	
	wrenFreeVM(vm);
	MemFile_Free(&mem);
	
	return res;
}