#include "z64playas.h"

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

void PrintHelp(void) {
	printf(
		"Usage:"
		PRNT_RSET "\n--script   " PRNT_GRAY "// Manifest"
		PRNT_RSET "\n--input    " PRNT_GRAY "// Input .zobj"
		PRNT_RSET "\n--bank     " PRNT_GRAY "// Bank .zobj"
		PRNT_RSET "\n--output   " PRNT_GRAY "// Output .zobj"
		PRNT_RSET "\n--patch    " PRNT_GRAY "// Output .mnf"
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
	char* fnamePatch = NULL;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	Calloc(state, sizeof(PlayAsState));
	
#if 0
	ZObj a;
	ZObj b;
	u32 offset;
	
	ZObj_New(&b, 6);
	ZObj_Read(&a, "input.zobj", 6);
	
	if (DisplayList_Copy(&a, 0x0601B998, &b, &offset))
		printf_error("Error");
	
	return 0;
#endif
	
	if (Arg("silence")) printf_SetSuppressLevel(PSL_NO_INFO);
	if (Arg("s") || Arg("script")) script = qFree(StrDupX(argv[parArg], 0x80));
	if (Arg("i") || Arg("input")) fnameInput = qFree(StrDupX(argv[parArg], 0x80));
	if (Arg("b") || Arg("bank")) fnameBank = qFree(StrDupX(argv[parArg], 0x80));
	if (Arg("o") || Arg("output")) fnameOutput = qFree(StrDupX(argv[parArg], 0x80));
	if (Arg("p") || Arg("patch")) fnamePatch = qFree(StrDupX(argv[parArg], 0x80));
	if (script == NULL) SendHelp("No script provided!");
	if (fnameInput == NULL) SendHelp("No input provided!");
	if (fnameBank == NULL) SendHelp("No bank provided!");
	if (fnameOutput == NULL) SendHelp("No output provided!");
	if (fnamePatch == NULL) SendHelp("No patch output provided!");
	
	char* statList[] = {
		script,
		fnameInput,
		fnameBank,
	};
	
	foreach(i, statList) {
		if (!Sys_Stat(statList[i]))
			printf_error("Could not locate file \"%s\" !", statList[i]);
	}
	
	if (!StrEndCase(fnameOutput, ".zobj"))
		printf_error("Output file extension does not seem to match '.zobj'. Please fix!");
	if (!StrEndCase(fnamePatch, ".cfg"))
		printf_error("Patch output file extension does not seem to match '.cfg'. Please fix!");
	
	MemFile_LoadFile(&state->bank, fnameBank);
	MemFile_LoadFile(&state->playas, fnameInput);
	MemFile_LoadFile(&state->output, fnameInput);
	
	MemFile_Alloc(&state->patch.file, MbToBin(16));
	
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
						printf_info("\t0x%08X %s", subNode->dict.offset, subNode->dict.object);
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
	
	if (Arg("print-lut"))
		printf_hex("Lut Hex Dump:", state->table.file.data, state->table.file.seekPoint, 0x1000);
	
	if (Arg("print-patch")) {
		printf("\n%s\n", state->patch.file.str);
	}
	
	MemFile_SaveFile(&state->output, fnameOutput);
	MemFile_SaveFile(&state->patch.file, fnamePatch);
	
	PlayAs_Free(state);
	Log_Free();
	
	printf_info("OK");
}