#include "z64playas.h"
#include <zobj.h>
#include <displaylist.h>

static u32 PlayAs_FindObjectGroup(PlayAsState* state, const char* objGroupName) {
    char* plTbl = state->mnfTable;
    
    Log("Searching '%s'", objGroupName);
    
    plTbl += strlen("!PlayAsManifest0") + 2;
    
    while (plTbl < state->mnfTable + state->mnfSize) {
        char* str = plTbl;
        u32* val = (void*)(plTbl + strlen(plTbl) + 1);
        
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
    char* mnfTable = MemMem(state->playas.data, state->playas.size, "!PlayAsManifest0", strlen("!PlayAsManifest0"));
    
    state->mnfSize = mnfTable - state->playas.str;
    if (mnfTable == NULL)
        printf_error("No playas data found in '%s'", state->playas.info.name);
    
    state->mnfTable = MemDup(mnfTable, state->mnfSize);
    state->playas.size -= state->mnfSize;
    
    obj.buffer = state->output.data;
    obj.segmentNumber = state->segment;
    obj.limit = state->output.size;
    obj.capacity = state->output.memSize;
    
    bank.buffer = state->bank.data;
    bank.segmentNumber = state->segment;
    bank.limit = state->bank.size;
    bank.capacity = state->bank.memSize;
    
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
    state->output.size = obj.limit;
    state->output.memSize = obj.capacity;
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
