#include "z64playas.h"

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

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
	MemFile_Free(&state->base);
	MemFile_Free(&state->output);
	Free(state);
}

s32 Main(s32 argc, char* argv[]) {
	PlayAsState* state;
	u32 parArg;
	char* script = NULL;
	char* fnameInput = NULL;
	char* fnameBase = NULL;
	char* fnameOutput = NULL;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Calloc(state, sizeof(PlayAsState));
	
	if (Arg("script")) script = argv[parArg];
	if (Arg("input")) fnameInput = argv[parArg];
	if (Arg("base")) fnameBase = argv[parArg];
	if (Arg("output")) fnameOutput = argv[parArg];
	if (script == NULL) printf_error("No script provided! Provide one with '--script file.mnf'");
	if (fnameInput == NULL) printf_error("No input zobj provided! Provide one with '--input file.zobj'");
	if (fnameBase == NULL) printf_error("No base zobj provided! Provide one with '--base file.zobj'");
	if (fnameOutput == NULL) printf_error("No fnameOutput provided! Provide one with '--fnameOutput file.zobj'");
	
	MemFile_LoadFile(&state->base, fnameBase);
	MemFile_LoadFile(&state->output, fnameBase);
	
	MemFile_Malloc(&state->patch.file, MbToBin(8));
	
	if (Arg("segment"))
		state->segment = Value_Hex(argv[parArg]);
	else
		state->segment = 0x06000000;
	
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
	
	MemFile_SaveFile(&state->output, fnameOutput);
	PlayAs_Free(state);
	Log_Free();
	
	printf_info("OK");
}