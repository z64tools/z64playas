#include <ext_lib.h>
#include <wren.h>

struct DataNode;
struct ObjectNode;

typedef enum {
    TYPE_DICTIONARY,
    TYPE_BRANCH,
    TYPE_MATRIX,
    TYPE_MATRIX_OPERATION,
} DataType;

typedef struct {
    char* object;
    u32   offset;
} Dictionary;

typedef struct {
    union {
        struct {
            f32 rx, ry, rz;
            f32 px, py, pz;
            f32 sx, sy, sz;
        };
        f32 f[9];
    };
} Matrix;

typedef struct {
    s8 push : 4;
    s8 pop  : 4;
} MatrixOperation;

typedef struct {
    char* name;
    struct ObjectNode* node;
} Branch;

typedef struct DataNode {
    struct DataNode* prev;
    struct DataNode* next;
    
    DataType type;
    union {
        MatrixOperation mtxOp;
        Dictionary      dict;
        Branch branch;
        Matrix mtx;
        void*  data;
    };
} DataNode;

typedef struct ObjectNode {
    struct ObjectNode* prev;
    struct ObjectNode* next;
    
    u32       offset;
    char*     name;
    DataNode* data;
} ObjectNode;

typedef enum {
    MTXMODE_NEW,
    MTXMODE_APPLY
} MatrixMode;

typedef float MtxF_t[4][4];
typedef union {
    f32 mf[4][4];
    struct {
        f32 xx, yx, zx, wx;
        f32 xy, yy, zy, wy;
        f32 xz, yz, zz, wz;
        f32 xw, yw, zw, ww;
    };
} MtxF;

typedef union  {
    s32 m[4][4];
    struct structBE {
        u16 intPart[4][4];
        u16 fracPart[4][4];
    };
    long long int force_structure_alignment;
} Mtx;

typedef struct {
    ObjectNode* objNode;
    u32 segment;
    
    struct {
        memfile_t file;
        u32       size;
        u32       offset;
    } table;
    
    struct {
        const char* tbl;
        toml_t      file;
        u32 offset;
        u32 advanceBy;
    } patch;
    
    memfile_t bank;
    memfile_t playas;
    memfile_t output;
    
    char* mnfTable;
    u32   mnfSize;
} PlayAsState;

void SkinMatrix_SetTranslate(MtxF* mf, f32 x, f32 y, f32 z);
void SkinMatrix_SetScale(MtxF* mf, f32 x, f32 y, f32 z);
void SkinMatrix_SetRotate(MtxF* mf, s16 x, s16 y, s16 z);

void Matrix_SetTranslateRotateYXZ(MtxF* cmf, f32 translateX, f32 translateY, f32 translateZ, s16 rotX, s16 rotY, s16 rotZ);
void Matrix_MtxFToMtx(MtxF* src, Mtx* dest);
void Matrix_Translate(MtxF* cmf, f32 x, f32 y, f32 z, MatrixMode mode);
void Matrix_Scale(MtxF* cmf, f32 x, f32 y, f32 z, MatrixMode mode);
void Matrix_Rotate(MtxF* cmf, s16 x, s16 y, s16 z, MatrixMode mode);

void guRTSF(MtxF* mf, float r, float p, float h, float x, float y, float z, float sx, float sy, float sz);

void PlayAs_Process(PlayAsState* state);
void PlayAs_Free(PlayAsState* state);

s32 Script_Run(const char* script, PlayAsState* state);
