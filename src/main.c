#include "z64playas.h"

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

const char* gToolName = PRNT_GREN "z64playas " PRNT_GRAY "0.9.2";

void PrintHelp(void) {
    printf(
        "Usage:"
        PRNT_RSET "\n--script   " PRNT_GRAY "// Script .mnf"
        PRNT_RSET "\n--input    " PRNT_GRAY "// Input .zobj"
        PRNT_RSET "\n--bank     " PRNT_GRAY "// Bank .zobj"
        PRNT_RSET "\n--output   " PRNT_GRAY "// Output .zobj"
        PRNT_RSET "\n--patch    " PRNT_GRAY "// Patch output .cfg"
        PRNT_RSET "\n--header    " PRNT_GRAY "// Header output .h"
        "\n"
    );
}

void SendHelp(const char* msg) {
    printf_warning("%s\n", msg);
    PrintHelp();
    exit(1);
}

s32 Main(s32 argc, const char* argv[]) {
    PlayAsState* state = New(PlayAsState);
    u32 argID;
    char* script = NULL;
    char* fnameInput = NULL;
    char* fnameBank = NULL;
    char* fnameOutput = NULL;
    char* fnamePatch = NULL;
    char* fnameHeader = NULL;
    
    printf_toolinfo(gToolName, "");
    
    if (argc == 1) {
        PrintHelp();
#ifdef _WIN32
        if (argc == 1) {
            printf_nl();
            printf_getchar("Press Enter to Exit!");
        }
#endif
        
        return 1;
    }
    
    if ((argID = ArgStr(argv, "silence"))) printf_SetSuppressLevel(PSL_NO_INFO);
    if ((argID = ArgStr(argv, "s")) || (argID = ArgStr(argv, "script"))) script = qFree(StrDup(argv[argID]));
    if ((argID = ArgStr(argv, "i")) || (argID = ArgStr(argv, "input"))) fnameInput = qFree(StrDup(argv[argID]));
    if ((argID = ArgStr(argv, "b")) || (argID = ArgStr(argv, "bank"))) fnameBank = qFree(StrDup(argv[argID]));
    if ((argID = ArgStr(argv, "o")) || (argID = ArgStr(argv, "output"))) fnameOutput = qFree(StrDup(argv[argID]));
    if ((argID = ArgStr(argv, "p")) || (argID = ArgStr(argv, "patch"))) fnamePatch = qFree(StrDup(argv[argID]));
    if ((argID = ArgStr(argv, "h")) || (argID = ArgStr(argv, "header"))) fnameHeader = qFree(StrDup(argv[argID]));
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
    if (fnameHeader && !StrEndCase(fnameHeader, ".h"))
        printf_error("Header output file extension does not seem to match '.h'. Please fix!");
    
    MemFile_LoadFile(&state->bank, fnameBank);
    MemFile_LoadFile(&state->playas, fnameInput);
    MemFile_LoadFile(&state->output, fnameInput);
    
    MemFile_Alloc(&state->patch.file, MbToBin(16));
    
    if ((argID = ArgStr(argv, "segment")))
        state->segment = Value_Hex(argv[argID]);
    else
        state->segment = 6;
    
    Script_Run(script, state);
    
    if (ArgStr(argv, "print-vars")) {
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
    
    if (ArgStr(argv, "print-lut"))
        printf_hex("Lut Hex Dump:", state->table.file.data, state->table.file.seekPoint, 0x1000);
    
    if (ArgStr(argv, "print-patch")) {
        printf("\n%s\n", state->patch.file.str);
    }
    
    if (MemFile_SaveFile(&state->output, fnameOutput)) printf_error("Could not save [%s]", fnameOutput);
    if (MemFile_SaveFile(&state->patch.file, fnamePatch)) printf_error("Could not save [%s]", fnamePatch);
    
    if (fnameHeader) {
        MemFile header = MemFile_Initialize();
        ObjectNode* node = state->objNode;
        DataNode* data;
        
        MemFile_Alloc(&header, MbToBin(16));
        
        while (node) {
            data = node->data;
            
            if (data && data->type == TYPE_DICTIONARY) {
                char* varName = StrDupX(node->name, strlen(node->name) + 0x20);
                
                MemFile_Printf(&header, "extern Gfx %s[];\n", varName, data->dict.offset);
                
                Free(varName);
            } else {
                MemFile_Printf(&header, "extern Gfx %s[];\n", node->name, node->offset);
            }
            
            node = node->next;
        }
        
        node = state->objNode;
        while (node) {
            data = node->data;
            
            if (data && data->type == TYPE_DICTIONARY) {
                char* varName = StrDupX(node->name, strlen(node->name) + 0x20);
                
                MemFile_Printf(&header, "asm(\"%-24s = 0x%08X\");\n", varName, data->dict.offset);
                
                Free(varName);
            } else {
                MemFile_Printf(&header, "asm(\"%-24s = 0x%08X\");\n", node->name, node->offset);
            }
            
            node = node->next;
        }
        
        if (MemFile_SaveFile_String(&header, fnameHeader)) printf_error("Could not save [%s]", fnameHeader);
        MemFile_Free(&header);
    }
    
    PlayAs_Free(state);
    Log_Free();
    
    printf_info("OK");
#ifdef _WIN32
    if (argc == 1) {
        printf_nl();
        printf_getchar("Press Enter to Exit!");
    }
#endif
}
