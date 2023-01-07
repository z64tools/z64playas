#include "z64playas.h"

void ZObject_WriteEntry(WrenVM* vm);
void ZObject_SetLutTable(WrenVM* vm);
void ZObject_BuildLutTable(WrenVM* vm);
void ZObject_Entry(WrenVM* vm);
void ZObject_Mtx(WrenVM* vm);
void ZObject_PopMtx(WrenVM* vm);
void ZObject_PushMtx(WrenVM* vm);
void ZObject_Branch(WrenVM* vm);

void Patch_AdvanceBy(WrenVM* vm);
void Patch_Offset(WrenVM* vm);
void Patch_WriteFloat(WrenVM* vm);
void Patch_Write32(WrenVM* vm);
void Patch_Write16(WrenVM* vm);
void Patch_Write8(WrenVM* vm);
void Patch_Hi32(WrenVM* vm);
void Patch_Lo32(WrenVM* vm);

extern unsigned char gMainModuleData[];
extern unsigned int gMainModuleSize;

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
    _log("ResolveModule: %s - %s", importer, name);
    
    if (!strcmp(name, "z64playas"))
        return name;
    
    return fmt("%s.mnf", name);
}

static void
Script_ModuleOnComplete(WrenVM* vm, const char* name, struct WrenLoadModuleResult result) {
    if (result.userData == NULL) {
        vfree(result.source);
    } else
        Memfile_Free(result.userData);
}

static WrenLoadModuleResult
Script_LoadModule(WrenVM* vm, const char* module) {
    static Memfile mem;
    
    if (!strcmp(module, "z64playas")) {
        return (WrenLoadModuleResult) {
                   .onComplete = Script_ModuleOnComplete,
                   .source = strdup((char*)gMainModuleData),
                   .userData = NULL
        };
    }
    
    _log("LoadModule: %s", module);
    
    Memfile_LoadStr(&mem, module);
    
    return (WrenLoadModuleResult) {
               .onComplete = Script_ModuleOnComplete,
               .source = mem.str,
               .userData = &mem
    };
}

static WrenForeignMethodFn
Script_ForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) {
    _log("ForeignMethod: %s %s %s", module, className, signature);
    
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
        
        if (!strcmp(signature, "hi32(_)"))
            return Patch_Hi32;
        
        if (!strcmp(signature, "lo32(_)"))
            return Patch_Lo32;
        
        if (!strcmp(signature, "writeFloat(_)"))
            return Patch_WriteFloat;
    }
    
    return NULL;
}

s32 Script_Run(const char* script, PlayAsState* state) {
    WrenInterpretResult res;
    WrenConfiguration config;
    WrenVM* vm;
    Memfile mem;
    
    wrenInitConfiguration(&config);
    
    config.writeFn = Script_Print;
    config.errorFn = Script_Error;
    config.resolveModuleFn = Script_ResolveModule;
    config.loadModuleFn = Script_LoadModule;
    config.bindForeignMethodFn = Script_ForeignMethod;
    config.initialHeapSize = MbToBin(16.0);
    config.userData = state;
    
    Memfile_LoadStr(&mem, script);
    
    vm = wrenNewVM(&config);
    res = wrenInterpret(vm, script, mem.data);
    
    wrenFreeVM(vm);
    Memfile_Free(&mem);
    
    return res;
}
