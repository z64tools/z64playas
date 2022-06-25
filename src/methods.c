#include "z64playas.h"

void ZObject_WriteEntry(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode;
	DataNode* dataNode;
	
	Calloc(objNode, sizeof(*objNode));
	Calloc(dataNode, sizeof(*dataNode));
	Node_Add(objNode->data, dataNode);
	Node_Add(state->objNode, objNode);
	
	objNode->name = StrDup(wrenGetSlotString(vm, 1));
	objNode->data = dataNode;
	
	dataNode->type = TYPE_DICTIONARY;
	dataNode->dict.baseOffset = wrenGetSlotDouble(vm, 2);
	dataNode->dict.object = StrDup(wrenGetSlotString(vm, 3));
}

void ZObject_SetLutTable(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	
	state->table.offset = wrenGetSlotDouble(vm, 1);
	state->table.size = wrenGetSlotDouble(vm, 2);
	
	MemFile_Alloc(&state->table.file, state->table.size);
}

void ZObject_BuildLutTable(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode = state->objNode;
	
	while (objNode) {
		DataNode* dataNode = objNode->data;
		
		objNode->offset = state->table.file.seekPoint + state->table.offset + state->segment;
		
		while (dataNode) {
			s32 isLast = dataNode->next == NULL ? true : false;
			ObjectNode* branchNode;
			u32 offset;
			MtxF mtxf;
			Mtx mtx64;
			
			switch (dataNode->type) {
				case TYPE_DICTIONARY:
					break;
				case TYPE_BRANCH:
					branchNode = dataNode->branch.node;
					offset = ReadBE(branchNode->offset);
					
					switch (branchNode->data->type) {
						case TYPE_DICTIONARY:
							if (!isLast)
								MemFile_Write(&state->table.file, "\xDE\x00\0\0", 4);
							else
								MemFile_Write(&state->table.file, "\xDE\x01\0\0", 4);
							
							offset = ReadBE(branchNode->data->dict.baseOffset);
							MemFile_Write(&state->table.file, &offset, 4);
							break;
						case TYPE_BRANCH:
							if (!isLast)
								MemFile_Write(&state->table.file, "\xDE\x00\0\0", 4);
							else
								MemFile_Write(&state->table.file, "\xDE\x01\0\0", 4);
							
							MemFile_Write(&state->table.file, &offset, 4);
							break;
						case TYPE_MATRIX:
							MemFile_Write(&state->table.file, "\xDA\x38\0\0", 4);
							MemFile_Write(&state->table.file, &offset, 4);
							break;
						case TYPE_MATRIX_OPERATION:
							break;
					}
					
					break;
				case TYPE_MATRIX:
					Matrix_Scale(&mtxf, dataNode->mtx.sx, dataNode->mtx.sy, dataNode->mtx.sz, MTXMODE_NEW);
					Matrix_Translate(&mtxf, dataNode->mtx.px, dataNode->mtx.py, dataNode->mtx.pz, MTXMODE_APPLY);
					Matrix_Rotate(&mtxf, dataNode->mtx.rx, dataNode->mtx.ry, dataNode->mtx.rz, MTXMODE_APPLY);
					MemFile_Write(&state->table.file, &mtx64, sizeof(mtx64));
					
					break;
				case TYPE_MATRIX_OPERATION:
					if (dataNode->mtxOp.pop)
						MemFile_Write(&state->table.file, "\xD8\x38\x00\x02" "\x00\x00\x00\x40", 8);
					break;
			}
			
			dataNode = dataNode->next;
		}
		
		objNode = objNode->next;
	}
}

void ZObject_Entry(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode;
	
	Calloc(objNode, sizeof(*objNode));
	Node_Add(state->objNode, objNode);
	
	objNode->name = StrDup(wrenGetSlotString(vm, 1));
}

void ZObject_Mtx(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode = state->objNode;
	DataNode* dataNode;
	
	while (objNode && objNode->next)
		objNode = objNode->next;
	
	Calloc(dataNode, sizeof(*dataNode));
	Node_Add(objNode->data, dataNode);
	
	dataNode->type = TYPE_MATRIX;
	for (s32 i = 0; i < 3 * 3; i++)
		dataNode->mtx.f[i] = wrenGetSlotDouble(vm, 1 + i);
}

void ZObject_PopMtx(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode = state->objNode;
	DataNode* dataNode;
	
	while (objNode && objNode->next)
		objNode = objNode->next;
	
	Calloc(dataNode, sizeof(*dataNode));
	Node_Add(objNode->data, dataNode);
	
	dataNode->type = TYPE_MATRIX_OPERATION;
	dataNode->mtxOp.pop = true;
}

void ZObject_PushMtx(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode = state->objNode;
	DataNode* dataNode;
	
	while (objNode && objNode->next)
		objNode = objNode->next;
	
	Calloc(dataNode, sizeof(*dataNode));
	Node_Add(objNode->data, dataNode);
	
	dataNode->type = TYPE_MATRIX_OPERATION;
	dataNode->mtxOp.push = true;
}

void ZObject_Branch(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	ObjectNode* objNode = state->objNode;
	DataNode* dataNode;
	char* caller;
	
	Log("BranchTo: %s", wrenGetSlotString(vm, 1));
	
	while (objNode && objNode->next)
		objNode = objNode->next;
	
	caller = objNode->name;
	Calloc(dataNode, sizeof(*dataNode));
	Node_Add(objNode->data, dataNode);
	
	dataNode->type = TYPE_BRANCH;
	dataNode->branch.name = StrDup(wrenGetSlotString(vm, 1));
	
	objNode = state->objNode;
	while (objNode) {
		if (!strcmp(objNode->name, dataNode->branch.name)) {
			dataNode->branch.node = objNode;
			
			break;
		}
		
		objNode = objNode->next;
	}
	
	if (dataNode->branch.node == NULL) {
		fprintf(stderr, "["PRNT_REDD "Runtime Error!"PRNT_RSET "]\n");
		fprintf(stderr, "\aNo reference found of '" PRNT_YELW "%s" PRNT_RSET "' for '" PRNT_YELW "%s" PRNT_RSET "' to branch to!\n", dataNode->branch.name, caller);
		fprintf(stderr, "Make sure that it exists and it's defined before the entry that calls it.\n");
		exit(1);
	}
}

void Patch_AdvanceBy(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	
	state->patch.advanceBy = wrenGetSlotDouble(vm, 1);
}

void Patch_Offset(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	u32 arg = 1;
	u32 argC = wrenGetSlotCount(vm) - 1;
	
	switch (argC) {
		case 2:
			for (s32 i = 1; i <= 2; i++) {
				if (wrenGetSlotType(vm, i) == WREN_TYPE_STRING)
					arg = i;
			}
			
			Config_WriteSection(&state->patch.file, wrenGetSlotString(vm, arg), NO_COMMENT);
			
			// Flip Argument for case 1
			if (arg == 2) arg = 1;
			else arg = 2;
		case 1:
			state->patch.offset = wrenGetSlotDouble(vm, arg);
	}
}

static void Patch_Write(PlayAsState* state, u32 size, char* value) {
	u32 val = 0;
	
	if (Value_ValidateHex(value)) {
		val = Value_Hex(value);
		
		switch (size) {
			case 1:
				if (!IsBetween(val, 0, __UINT8_MAX__)) {
					fprintf(stderr, "[" PRNT_YELW "Warning" PRNT_RSET "]\n");
					fprintf(stderr, "Integer Overflow: patch.write8(0x%X)\n", val);
				}
				Clamp(val, 0, __UINT8_MAX__);
				
				break;
			case 2:
				if (!IsBetween(val, 0, __UINT16_MAX__)) {
					fprintf(stderr, "[" PRNT_YELW "Warning" PRNT_RSET "]\n");
					fprintf(stderr, "Integer Overflow: patch.write16(0x%X)\n", val);
				}
				Clamp(val, 0, __UINT16_MAX__);
				
				break;
			case 4:
				if (!IsBetween(val, 0, __UINT32_MAX__)) {
					fprintf(stderr, "[" PRNT_YELW "Warning" PRNT_RSET "]\n");
					fprintf(stderr, "Integer Overflow: patch.write32(0x%X)\n", val);
				}
				Clamp(val, 0, __UINT32_MAX__);
				
				break;
		}
	} else {
		ObjectNode* node = state->objNode;
		
		switch (size) {
			case 1:
				fprintf(stderr, "Offset of variable is not 8 value: patch.write8(0x%X)\n", val);
				
				return;
			case 2:
				fprintf(stderr, "Offset of variable is not 16 value: patch.write16(0x%X)\n", val);
				
				return;
		}
		
		while (node) {
			if (!strcmp(node->name, value)) {
				val = node->offset;
				break;
			}
			
			node = node->next;
		}
		
		if (node == NULL) {
			fprintf(stderr, "[" PRNT_YELW "Warning" PRNT_RSET "]\n");
			fprintf(stderr, "Could not find variable: patch.write(\"%s\")\n", value);
			
			return;
		}
	}
	
	Config_Print(&state->patch.file, "\t");
	Config_WriteHex(&state->patch.file, xFmt("0x%08X", state->patch.offset), (u32)val, NO_COMMENT);
	state->patch.offset += state->patch.advanceBy ? state->patch.advanceBy : size;
}

void Patch_Write32(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	char* value;
	
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			value = xStrDup(wrenGetSlotString(vm, 1));
			break;
		default:
			value = xFmt("0x%X", (u64)wrenGetSlotDouble(vm, 1));
			break;
	}
	
	Patch_Write(state, 4, value);
}

void Patch_Write16(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	char* value;
	
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			value = xStrDup(wrenGetSlotString(vm, 1));
			break;
		default:
			value = xFmt("0x%X", (u64)wrenGetSlotDouble(vm, 1));
			break;
	}
	
	Patch_Write(state, 2, value);
}

void Patch_Write8(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	char* value;
	
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			value = xStrDup(wrenGetSlotString(vm, 1));
			break;
		default:
			value = xFmt("0x%X", (u64)wrenGetSlotDouble(vm, 1));
			break;
	}
	
	Patch_Write(state, 1, value);
}

void Patch_Hi32(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	char* value;
	
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			value = xStrDup(wrenGetSlotString(vm, 1));
			break;
		default:
			value = xFmt("0x%X", (u64)wrenGetSlotDouble(vm, 1));
			break;
	}
	
	Patch_Write(state, 2, xFmt("0x%X", Value_Hex(value) >> 16));
}

void Patch_Lo32(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	char* value;
	
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			value = xStrDup(wrenGetSlotString(vm, 1));
			break;
		default:
			value = xFmt("0x%X", (u64)wrenGetSlotDouble(vm, 1));
			break;
	}
	
	Patch_Write(state, 2, xFmt("0x%X", Value_Hex(value) & 0xFFFF));
}