#include "z64playas.h"

void Method_ZObject_DictEntry(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	DictNode* node;
	
	Calloc(node, sizeof(*node));
	
	node->name = StrDup(wrenGetSlotString(vm, 1));
	node->offset = wrenGetSlotDouble(vm, 2);
	node->object = StrDup(wrenGetSlotString(vm, 3));
	
	Node_Add(state->dictNode, node);
}

void Method_Lut_SetTable(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	
	state->poolOffset = wrenGetSlotDouble(vm, 1);
	state->poolSize = wrenGetSlotDouble(vm, 2);
}

void Method_Lut_Entry(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	LutNode* node;
	
	Calloc(node, sizeof(*node));
	node->name = StrDup(wrenGetSlotString(vm, 1));
	Node_Add(state->lutNode, node);
}

void Method_Lut_WriteMtx(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	LutDataNode* node;
	LutNode* lut = state->lutNode;
	
	while (lut && lut->next)
		lut = lut->next;
	
	Calloc(node, sizeof(*node));
	for (s32 i = 0; i < 3 * 3; i++)
		node->mtx[i] = wrenGetSlotDouble(vm, 1 + i);
	Node_Add(lut->data, node);
}

void Method_Lut_PopMtx(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	LutDataNode* node;
	LutNode* lut = state->lutNode;
	
	while (lut && lut->next)
		lut = lut->next;
	
	Calloc(node, sizeof(*node));
	node->pop = 1;
	Node_Add(lut->data, node);
}

void Method_Lut_Call(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	LutDataNode* node;
	LutNode* lut = state->lutNode;
	
	while (lut && lut->next)
		lut = lut->next;
	
	Calloc(node, sizeof(*node));
	node->call = StrDup(wrenGetSlotString(vm, 1));
	Node_Add(lut->data, node);
}