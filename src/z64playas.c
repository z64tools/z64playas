#include "z64playas.h"

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

s32 Main(s32 argc, char* argv[]) {
	PlayAsState* state;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	
	Calloc(state, sizeof(PlayAsState));
	
	Wren_Run("manifest.mnf", state);
	
	LutNode* node = state->lutNode;
	
	while (node) {
		LutDataNode* subNode = node->data;
		printf_info("" PRNT_YELW "%s:", node->name);
		
		while (subNode) {
			if (subNode->call)
				printf_info("\t%s", subNode->call);
			else
				printf_info(
					"\t%.0f %.0f %.0f / %.0f %.0f %.0f / %.0f %.0f %.0f",
					subNode->mtx[0],
					subNode->mtx[1],
					subNode->mtx[2],
					subNode->mtx[3],
					subNode->mtx[4],
					subNode->mtx[5],
					subNode->mtx[6],
					subNode->mtx[7],
					subNode->mtx[8]
				);
			
			subNode = subNode->next;
		}
		
		node = node->next;
	}
}