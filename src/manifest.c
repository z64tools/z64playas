#include "z64playas.h"

static void
Script_Print(WrenVM* vm, const char* str) {
	printf("%s", str);
}

static void
Script_Error(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message) {
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
Script_ResolveModule(WrenVM* vm, const char* importer, const char* name) {
	Log("ResolveModule: %s - %s", importer, name);
	
	if (!strcmp(name, "z64playas"))
		return StrDup(HeapPrint("%sMainModule.mnf", Sys_AppDir()));
	
	return StrDup(HeapPrint("%s.mnf", name));
}

static void
Script_ModuleOnComplete(WrenVM* vm, const char* name, struct WrenLoadModuleResult result) {
	MemFile_Free(result.userData);
}

static WrenLoadModuleResult
Script_LoadModule(WrenVM* vm, const char* module) {
	static MemFile mem;
	
	Log("LoadModule: %s", module);
	
	MemFile_LoadFile_String(&mem, module);
	
	return (WrenLoadModuleResult) {
		       .onComplete = Script_ModuleOnComplete,
		       .source = mem.str,
		       .userData = &mem
	};
}

static WrenForeignMethodFn
Script_ForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) {
	Log("ForeignMethod: %s %s %s", module, className, signature);
	
	if (!strcmp(className, "ZObject")) {
		if (!strcmp(signature, "writeEntry(_,_,_)"))
			return ZObject_WriteEntry;
		
		if (!strcmp(signature, "setLutTable(_,_)"))
			return ZObject_SetLutTable;
		
		if (!strcmp(signature, "buildLutTable()"))
			return ZObject_BuildLutTable;
		
		if (!strcmp(signature, "entry(_)"))
			return ZObject_Entry;
		
		if (!strcmp(signature, "mtx(_,_,_,_,_,_,_,_,_)"))
			return ZObject_Mtx;
		
		if (!strcmp(signature, "popMtx()"))
			return ZObject_PopMtx;
		
		if (!strcmp(signature, "pushMtx()"))
			return ZObject_PushMtx;
		
		if (!strcmp(signature, "branch(_)"))
			return ZObject_Branch;
	}
	
	if (!strcmp(className, "Patch")) {
		if (!strcmp(signature, "advanceBy(_)"))
			return Patch_AdvanceBy;
		
		if (!strcmp(signature, "offset(_,_)"))
			return Patch_Offset;
		
		if (!strcmp(signature, "offset(_)"))
			return Patch_Offset;
		
		if (!strcmp(signature, "write32(_)"))
			return Patch_Write32;
		
		if (!strcmp(signature, "write16(_)"))
			return Patch_Write16;
		
		if (!strcmp(signature, "write8(_)"))
			return Patch_Write8;
	}
	
	return NULL;
}

s32 Script_Run(const char* script, PlayAsState* state) {
	WrenInterpretResult res;
	WrenConfiguration config;
	WrenVM* vm;
	MemFile mem;
	
	wrenInitConfiguration(&config);
	
	config.writeFn = Script_Print;
	config.errorFn = Script_Error;
	config.resolveModuleFn = Script_ResolveModule;
	config.loadModuleFn = Script_LoadModule;
	config.bindForeignMethodFn = Script_ForeignMethod;
	config.initialHeapSize = MbToBin(16.0);
	config.userData = state;
	
	MemFile_LoadFile_String(&mem, script);
	
	vm = wrenNewVM(&config);
	res = wrenInterpret(vm, script, mem.data);
	
	wrenFreeVM(vm);
	MemFile_Free(&mem);
	
	return res;
}