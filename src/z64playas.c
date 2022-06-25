#include "z64playas.h"
#include "dlcopy/zobj.h"
#include "dlcopy/displaylist.h"

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

u32 PlayAs_FindObjectGroup(PlayAsState* state, const char* objGroupName) {
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
	Free(state);
}

void PlayAs_Process(PlayAsState* state) {
	ObjectNode* node = state->objNode;
	DataNode* data;
	ZObj obj;
	ZObj bank;
	
	obj.buffer = state->output.data;
	obj.segmentNumber = state->segment;
	obj.size = state->output.dataSize;
	
	bank.buffer = state->bank.data;
	bank.segmentNumber = state->segment;
	bank.size = state->bank.dataSize;
	
	while (node) {
		data = node->data;
		if (data && data->type == TYPE_DICTIONARY) {
			u32 offset = PlayAs_FindObjectGroup(state, data->dict.object);
			
			switch (offset) {
				case 0:
					printf_error("Object Group Missing [%s]", data->dict.object);
					break;
					
				case -1:
					if (data->dict.baseOffset == 0)
						break;
					
					Log("Copy %08X", data->dict.baseOffset | (state->segment << 24));
					if (DisplayList_Copy(&bank, data->dict.baseOffset | (state->segment << 24), &obj, &offset))
						printf_error("Failed to copy from DisplayList [%08X]", data->dict.baseOffset);
					Log("Copy OK!");
					break;
					
				default:
					printf_info("0x%08X <-- \"%s\"", offset, node->name);
					break;
			}
			
			data->dict.offset = offset;
		}
		
		node = node->next;
	}
}

void PrintHelp(void) {
	printf(
		"Usage:"
		PRNT_RSET "\n--script   " PRNT_GRAY "// Manifest"
		PRNT_RSET "\n--input    " PRNT_GRAY "// Input .zobj"
		PRNT_RSET "\n--bank     " PRNT_GRAY "// Bank .zobj"
		PRNT_RSET "\n--output   " PRNT_GRAY "// Output .zobj"
		"\n"
	);
}

void SendHelp(const char* msg) {
	printf_warning("%s\n", msg);
	PrintHelp();
	exit(1);
}

s32 Main(s32 argc, char* argv[]) {
	PlayAsState* state;
	u32 parArg;
	char* script = NULL;
	char* fnameInput = NULL;
	char* fnameBank = NULL;
	char* fnameOutput = NULL;
	
	// ZObj a;
	// ZObj b;
	// u32 offset;
	// ZObj_New(&b, 6);
	//
	// ZObj_Read(&a, "input.zobj", 6);
	//
	// DisplayList_Copy(&a, 0x0601B998, &b, &offset);
	//
	// return 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Calloc(state, sizeof(PlayAsState));
	
	if (Arg("s") || Arg("script")) script = argv[parArg];
	if (Arg("i") || Arg("input")) fnameInput = argv[parArg];
	if (Arg("b") || Arg("bank")) fnameBank = argv[parArg];
	if (Arg("o") || Arg("output")) fnameOutput = argv[parArg];
	if (script == NULL) SendHelp("No script provided! Provide one with '--script file.mnf'");
	if (fnameInput == NULL) SendHelp("No input zobj provided! Provide one with '--input file.zobj'");
	if (fnameBank == NULL) SendHelp("No bank zobj provided! Provide one with '--bank file.zobj'");
	if (fnameOutput == NULL) SendHelp("No fnameOutput provided! Provide one with '--fnameOutput file.zobj'");
	
	MemFile_LoadFile(&state->bank, fnameBank);
	MemFile_LoadFile(&state->playas, fnameInput);
	
	MemFile_Alloc(&state->patch.file, MbToBin(16));
	MemFile_Alloc(&state->output, state->playas.dataSize * 16);
	MemFile_Append(&state->output, &state->playas);
	
	if (Arg("segment"))
		state->segment = Value_Hex(argv[parArg]);
	else
		state->segment = 6;
	
	Script_Run(script, state);
	
	if (Arg("print-vars")) {
		ObjectNode* node = state->objNode;
		
		while (node) {
			DataNode* subNode = node->data;
			printf_info("" PRNT_YELW "%s:", node->name);
			
			while (subNode) {
				switch (subNode->type) {
					case TYPE_DICTIONARY:
						printf_info("\t0x%08X %s", subNode->dict.baseOffset, subNode->dict.object);
						break;
					case TYPE_BRANCH:
						printf_info("\t%s", subNode->branch);
						break;
					case TYPE_MATRIX:
						printf_info(
							"\t%.0f %.0f %.0f / %.0f %.0f %.0f / %.0f %.0f %.0f",
							subNode->mtx.f[0],
							subNode->mtx.f[1],
							subNode->mtx.f[2],
							subNode->mtx.f[3],
							subNode->mtx.f[4],
							subNode->mtx.f[5],
							subNode->mtx.f[6],
							subNode->mtx.f[7],
							subNode->mtx.f[8]
						);
						break;
					case TYPE_MATRIX_OPERATION:
						break;
				}
				
				subNode = subNode->next;
			}
			
			node = node->next;
		}
	}
	
	if (Arg("print-hex")) {
		printf("\n");
		for (s32 i = 1; i <= state->table.file.seekPoint; i++) {
			printf("%02X", state->table.file.cast.u8[i - 1]);
			if (i % 4 == 0)
				printf(" ");
			if (i % 16 == 0)
				printf("\n");
		}
	}
	
	if (Arg("print-patch")) {
		printf("\n%s\n", state->patch.file.str);
	}
	
	PlayAs_Process(state);
	
	MemFile_SaveFile(&state->output, fnameOutput);
	PlayAs_Free(state);
	Log_Free();
	
	printf_info("OK");
}