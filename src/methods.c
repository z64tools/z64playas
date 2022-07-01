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
	dataNode->dict.offset = wrenGetSlotDouble(vm, 2);
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
	
	printf_info("REPOINT / COPY:");
	
	PlayAs_Process(state);
	
	printf_nl();
	printf_info("Look-up Table:");
	
	while (objNode) {
		DataNode* dataNode = objNode->data;
		u32 setDataSize = state->table.file.seekPoint;
		
		objNode->offset = (state->table.file.seekPoint + state->table.offset) | state->segment << 24;
		
		if (objNode->data && objNode->data->type != TYPE_DICTIONARY && objNode->data->type != TYPE_MATRIX)
			printf_info("" PRNT_BLUE "%s:", objNode->name);
		
		while (dataNode) {
			s32 isLast = dataNode->next == NULL ? true : false;
			ObjectNode* branchNode;
			ObjectNode* nNode;
			u32 offset = 0;
			MtxF mtxf;
			Mtx mtx64;
			
			switch (dataNode->type) {
				case TYPE_DICTIONARY:
					break;
				case TYPE_BRANCH:
					branchNode = dataNode->branch.node;
					offset = ReadBE(branchNode->offset | state->segment << 24);
					
					switch (branchNode->data->type) {
						case TYPE_DICTIONARY:
							offset = ReadBE(branchNode->data->dict.offset | state->segment << 24);
							
							nNode = state->objNode;
							while (nNode) {
								if (nNode->data && nNode->data->type == TYPE_DICTIONARY)
									if (nNode->data->dict.offset == ReadBE(offset))
										break;
								nNode = nNode->next;
							}
							
							if (nNode == NULL)
								offset = 0;
							
							printf_info(
								"" PRNT_GRAY "\t%08X " PRNT_YELW "Branch: " PRNT_RSET "%08X " PRNT_GRAY "\"%s\"",
								((state->table.file.seekPoint + state->table.offset) | state->segment << 24),
								ReadBE(offset),
								nNode ? nNode->data->dict.object : PRNT_REDD "NULL" PRNT_GRAY
							);
							
							if (!isLast) {
								if (!MemFile_Write(&state->table.file, "\xDE\x00\0\0", 4)) goto error;
							} else if (!MemFile_Write(&state->table.file, "\xDE\x01\0\0", 4)) goto error;
							
							if (!MemFile_Write(&state->table.file, &offset, 4)) goto error;
							break;
						case TYPE_BRANCH:
							printf_info(
								"" PRNT_GRAY "\t%08X " PRNT_YELW "Branch: " PRNT_RSET "%08X " PRNT_GRAY "%s",
								((state->table.file.seekPoint + state->table.offset) | state->segment << 24),
								ReadBE(offset),
								branchNode->name
							);
							
							if (!isLast) {
								if (!MemFile_Write(&state->table.file, "\xDE\x00\0\0", 4)) goto error;
							} else if (!MemFile_Write(&state->table.file, "\xDE\x01\0\0", 4)) goto error;
							
							if (!MemFile_Write(&state->table.file, &offset, 4)) goto error;
							break;
						case TYPE_MATRIX:
							nNode = state->objNode;
							while (nNode) {
								if (nNode->data && nNode->data->type == TYPE_MATRIX)
									if (nNode->offset == ReadBE(offset))
										break;
								nNode = nNode->next;
							}
							
							printf_info(
								"" PRNT_GRAY "\t%08X " PRNT_YELW "Branch: " PRNT_RSET "%08X " PRNT_GRAY "%s",
								((state->table.file.seekPoint + state->table.offset) | state->segment << 24),
								ReadBE(offset),
								nNode->name
							);
							
							if (!MemFile_Write(&state->table.file, "\xDA\x38\0\0", 4)) goto error;
							if (!MemFile_Write(&state->table.file, &offset, 4)) goto error;
							break;
						case TYPE_MATRIX_OPERATION:
							break;
					}
					
					break;
				case TYPE_MATRIX:
					//crustify
					guRTSF(
						&mtxf,
						dataNode->mtx.rx, dataNode->mtx.ry, dataNode->mtx.rz,
						dataNode->mtx.px, dataNode->mtx.py, dataNode->mtx.pz,
						dataNode->mtx.sx, dataNode->mtx.sy, dataNode->mtx.sz
					);
					Matrix_Rotate(
						&mtxf,
						dataNode->mtx.rx, dataNode->mtx.ry, dataNode->mtx.rz,
						MTXMODE_APPLY
					);
					//uncrustify
					
#if 0
					printf("%16d %16d %16d\n", (s32)dataNode->mtx.rx, (s32)dataNode->mtx.ry, (s32)dataNode->mtx.rz);
					printf(
						"MtxF\n%16.6f %16.6f %16.6f %16.6f\n%16.6f %16.6f %16.6f %16.6f\n%16.6f %16.6f %16.6f %16.6f\n%16.6f %16.6f %16.6f %16.6f\n",
						mtxf.xx,
						mtxf.xy,
						mtxf.xz,
						mtxf.xw,
						mtxf.yx,
						mtxf.yy,
						mtxf.yz,
						mtxf.yw,
						mtxf.zx,
						mtxf.zy,
						mtxf.zz,
						mtxf.zw,
						mtxf.wx,
						mtxf.wy,
						mtxf.wz,
						mtxf.ww
					);
#endif
					
					Matrix_MtxFToMtx(&mtxf, &mtx64);
					if (!MemFile_Write(&state->table.file, &mtx64, sizeof(mtx64))) goto error;
					
					break;
				case TYPE_MATRIX_OPERATION:
					if (dataNode->mtxOp.pop) {
						printf_info("" PRNT_GRAY "\t%08X " PRNT_PRPL "MatrixPop();", ((state->table.file.seekPoint + state->table.offset) | state->segment << 24));
						
						if (!MemFile_Write(&state->table.file, "\xD8\x38\x00\x02" "\x00\x00\x00\x40", 8)) goto error;
					}
					break;
			}
			
			dataNode = dataNode->next;
			
			continue;
error:
			printf_error("Ran out of space while writing '%s' to look-up table! Please increase table size!", objNode->name);
		}
		
		if (setDataSize == state->table.file.seekPoint)
			objNode->offset = 0;
		
		objNode = objNode->next;
	}
	
	printf_nl();
	
	memset(&state->output.cast.u8[state->table.offset], 0, state->table.size);
	MemFile_Seek(&state->output, state->table.offset);
	MemFile_Append(&state->output, &state->table.file);
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
	char* fmt;
	
	if (Value_ValidateHex(value)) {
		val = Value_Hex(value);
		
		switch (size & 0xF) {
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
				
				// 0 is dictionary only
				if (val == 0)
					val = node->data->dict.offset;
				
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
	
	if (size & 0x10) {
		s16 lo = val & 0xFFFF;
		size = 2;
		val = val >> 16;
		
		if (lo < 0)
			val++;
	}
	
	if (size & 0x20) {
		size = 2;
		val = val & 0xFFFF;
	}
	
	switch (size) {
		case 1:
			fmt = "0x%02X";
			break;
		case 2:
			fmt = "0x%04X";
			break;
		case 4:
			fmt = "0x%08X";
			break;
	}
	
	Config_Print(&state->patch.file, "\t");
	Config_WriteStr(&state->patch.file, xFmt("0x%08X", state->patch.offset), xFmt(fmt, val), NO_QUOTES, NO_COMMENT);
	state->patch.offset += state->patch.advanceBy ? state->patch.advanceBy : size;
}

void Patch_WriteFloat(WrenVM* vm) {
	PlayAsState* state = wrenGetUserData(vm);
	f32 f32;
	u32* value = (u32*)&f32;
	
	f32 = wrenGetSlotDouble(vm, 1);
	
	Config_Print(&state->patch.file, "\t");
	Config_WriteHex(&state->patch.file, xFmt("0x%08X", state->patch.offset), *value, NO_COMMENT);
	state->patch.offset += state->patch.advanceBy ? state->patch.advanceBy : 4;
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
	
	Patch_Write(state, 2 | 0x10, value);
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
	
	Patch_Write(state, 2 | 0x20, value);
}