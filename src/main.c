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
    warn("%s\n", msg);
    PrintHelp();
    exit(1);
}

s32 main(s32 argc, const char* argv[]) {
    PlayAsState* state = new(PlayAsState);
    u32 argID;
    char* script = NULL;
    char* fnameInput = NULL;
    char* fnameBank = NULL;
    char* fnameOutput = NULL;
    char* fnamePatch = NULL;
    char* fnameHeader = NULL;
    
    info_title(gToolName, "");
    
    if (argc == 1) {
        PrintHelp();
#ifdef _WIN32
        if (argc == 1) {
            info_nl();
            info_getc("Press Enter to Exit!");
        }
#endif
        
        return 1;
    }
    
    if ((argID = strarg(argv, "silence"))) IO_SetLevel(PSL_NO_INFO);
    if ((argID = strarg(argv, "s")) || (argID = strarg(argv, "script"))) script = qxf(strdup(argv[argID]));
    if ((argID = strarg(argv, "i")) || (argID = strarg(argv, "input"))) fnameInput = qxf(strdup(argv[argID]));
    if ((argID = strarg(argv, "b")) || (argID = strarg(argv, "bank"))) fnameBank = qxf(strdup(argv[argID]));
    if ((argID = strarg(argv, "o")) || (argID = strarg(argv, "output"))) fnameOutput = qxf(strdup(argv[argID]));
    if ((argID = strarg(argv, "p")) || (argID = strarg(argv, "patch"))) fnamePatch = qxf(strdup(argv[argID]));
    if ((argID = strarg(argv, "h")) || (argID = strarg(argv, "header"))) fnameHeader = qxf(strdup(argv[argID]));
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
        if (!sys_stat(statList[i]))
            errr("Could not locate file \"%s\" !", statList[i]);
    }
    
    if (!striend(fnameOutput, ".zobj"))
        errr("Output file extension does not seem to match '.zobj'. Please fix!");
    if (!striend(fnamePatch, ".cfg"))
        errr("Patch output file extension does not seem to match '.cfg'. Please fix!");
    if (fnameHeader && !striend(fnameHeader, ".h"))
        errr("Header output file extension does not seem to match '.h'. Please fix!");
    
    Memfile_LoadBin(&state->bank, fnameBank);
    Memfile_LoadBin(&state->playas, fnameInput);
    Memfile_LoadBin(&state->output, fnameInput);
    
    Memfile_Alloc(&state->patch.file, MbToBin(16));
    
    if ((argID = strarg(argv, "segment")))
        state->segment = shex(argv[argID]);
    else
        state->segment = 6;
    
    Script_Run(script, state);
    
    if (strarg(argv, "print-vars")) {
        ObjectNode* node = state->objNode;
        
        while (node) {
            DataNode* subNode = node->data;
            info("" PRNT_YELW "%s:", node->name);
            
            while (subNode) {
                switch (subNode->type) {
                    case TYPE_DICTIONARY:
                        info("\t0x%08X %s", subNode->dict.offset, subNode->dict.object);
                        break;
                    case TYPE_BRANCH:
                        info("\t%s", subNode->branch);
                        break;
                    case TYPE_MATRIX:
                        info(
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
    
    if (strarg(argv, "print-lut"))
        info_hex("Lut Hex Dump:", state->table.file.data, state->table.file.seekPoint, 0x1000);
    
    if (strarg(argv, "print-patch")) {
        printf("\n%s\n", state->patch.file.str);
    }
    
    if (Memfile_SaveBin(&state->output, fnameOutput)) errr("Could not save [%s]", fnameOutput);
    if (Memfile_SaveBin(&state->patch.file, fnamePatch)) errr("Could not save [%s]", fnamePatch);
    
    if (fnameHeader) {
        Memfile header = Memfile_New();
        ObjectNode* node = state->objNode;
        DataNode* data;
        
        Memfile_Alloc(&header, MbToBin(16));
        
        while (node) {
            data = node->data;
            
            if (data && data->type == TYPE_DICTIONARY) {
                char* varName = strndup(node->name, strlen(node->name) + 0x20);
                
                Memfile_Fmt(&header, "extern Gfx %s[];\n", varName, data->dict.offset);
                
                vfree(varName);
            } else {
                Memfile_Fmt(&header, "extern Gfx %s[];\n", node->name, node->offset);
            }
            
            node = node->next;
        }
        
        node = state->objNode;
        while (node) {
            data = node->data;
            
            if (data && data->type == TYPE_DICTIONARY) {
                char* varName = strndup(node->name, strlen(node->name) + 0x20);
                
                Memfile_Fmt(&header, "asm(\"%-24s = 0x%08X\");\n", varName, data->dict.offset);
                
                vfree(varName);
            } else {
                Memfile_Fmt(&header, "asm(\"%-24s = 0x%08X\");\n", node->name, node->offset);
            }
            
            node = node->next;
        }
        
        if (Memfile_SaveStr(&header, fnameHeader)) errr("Could not save [%s]", fnameHeader);
        Memfile_Free(&header);
    }
    
    PlayAs_Free(state);
    
    info("OK");
#ifdef _WIN32
    if (argc == 1) {
        info_nl();
        info_getc("Press Enter to Exit!");
    }
#endif
}
