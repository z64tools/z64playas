#include "z64playas.h"
#include "dlcopy/zobj.h"
#include "dlcopy/displaylist.h"

static u32 PlayAs_FindObjectGroup(PlayAsState* state, const char* objGroupName) {
	MemFile* input = &state->playas;
	char* plTbl = MemMem(input->data, input->dataSize, "!PlayAsManifest0", strlen("!PlayAsManifest0"));
	
	if (!plTbl) printf_error("Provided zobj [%s] does not have embedded manifest information.", input->info.name);
	
	plTbl += strlen("!PlayAsManifest0");
	
	// Find the first entry marked by '!'
	while (plTbl[-1] != '!') plTbl++;
	
	while (true) {
		char* str = plTbl;
		u32* val = (void*)(plTbl + strlen(plTbl) + 1);
		
		if (*str == '-')
			break;
		
		if (!strcmp(str, objGroupName))
			return ReadBE(*val);
		
		plTbl = (void*)(val + 1);
	}
	
	return -1;
}

void PlayAs_Process(PlayAsState* state) {
	ObjectNode* node = state->objNode;
	DataNode* data;
	ZObj obj;
	ZObj bank;
	
	obj.buffer = state->output.data;
	obj.segmentNumber = state->segment;
	obj.size = state->output.dataSize;
	obj.memSize = state->output.memSize;
	
	bank.buffer = state->bank.data;
	bank.segmentNumber = state->segment;
	bank.size = state->bank.dataSize;
	bank.memSize = state->bank.memSize;
	
	while (node) {
		data = node->data;
		if (data && data->type == TYPE_DICTIONARY) {
			u32 offset = PlayAs_FindObjectGroup(state, data->dict.object);
			
			switch (offset) {
				case -1:
					if (data->dict.offset == 0) {
						offset = 0;
						break;
					}
					
					Log("Copy %08X", data->dict.offset);
					if (DisplayList_Copy(&bank, data->dict.offset | (state->segment << 24), &obj, &offset))
						printf_error("Failed to copy from DisplayList [%08X]\n%s", data->dict.offset, DisplayList_ErrMsg());
					printf_info("" PRNT_PRPL "DlCopy:  " PRNT_RSET "%08X -> %08X " PRNT_GRAY "\"%s\"", data->dict.offset, offset, data->dict.object);
					break;
					
				default:
					offset |= (state->segment << 24);
					printf_info("" PRNT_YELW "Repoint: " PRNT_RSET "%08X -> %08X " PRNT_GRAY "\"%s\"", data->dict.offset, offset, data->dict.object);
					break;
			}
			
			Log("Next");
			data->dict.offset = offset;
		}
		
		node = node->next;
	}
	
	state->output.data = obj.buffer;
	state->output.dataSize = obj.size;
	state->output.memSize = obj.memSize;
	
	state->output.dataSize = obj.size;
}

void PlayAs_Free(PlayAsState* state) {
	while (state->objNode) {
		while (state->objNode->data) {
			DataNode* node = state->objNode->data;
			switch (node->type) {
				case TYPE_BRANCH:
					Free(node->branch.name);
					break;
				case TYPE_DICTIONARY:
					Free(node->dict.object);
					break;
			}
			
			Node_Kill(state->objNode->data, node);
		}
		
		Free(state->objNode->name);
		Node_Kill(state->objNode, state->objNode);
	}
	
	MemFile_Free(&state->table.file);
	MemFile_Free(&state->patch.file);
	MemFile_Free(&state->bank);
	MemFile_Free(&state->output);
	MemFile_Free(&state->playas);
	Free(state);
}