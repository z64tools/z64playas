#include <ExtLib.h>
#include <wren.h>

typedef struct DictNode {
	struct DictNode* prev;
	struct DictNode* next;
	
	char* name;
	u32   offset;
	char* object;
} DictNode;

typedef struct LutDataNode {
	struct LutDataNode* prev;
	struct LutDataNode* next;
	
	char* call;
	s8    pop;
	f32   mtx[9];
} LutDataNode;

typedef struct LutNode {
	struct LutNode* prev;
	struct LutNode* next;
	
	char* name;
	LutDataNode* data;
} LutNode;

typedef struct {
	DictNode* dictNode;
	LutNode*  lutNode;
	u32 poolSize;
	u32 poolOffset;
} PlayAsState;

s32 Wren_Run(const char* script, PlayAsState* state);

void Method_ZObject_DictEntry(WrenVM* vm);

void Method_Lut_SetTable(WrenVM* vm);
void Method_Lut_Entry(WrenVM* vm);
void Method_Lut_WriteMtx(WrenVM* vm);
void Method_Lut_PopMtx(WrenVM* vm);
void Method_Lut_Call(WrenVM* vm);