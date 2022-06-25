#include <ExtLib.h>
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
			f32 px, py, pz;
			f32 rx, ry, rz;
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
		Dictionary dict;
		Branch branch;
		Matrix mtx;
		void*  data;
	};
} DataNode;

typedef struct ObjectNode {
	struct ObjectNode* prev;
	struct ObjectNode* next;
	
	u32   offset;
	char* name;
	DataNode* data;
} ObjectNode;

typedef enum {
	MTXMODE_NEW,
	MTXMODE_APPLY
} MatrixMode;

typedef float MtxF_t[4][4];
typedef union {
	MtxF_t mf;
	struct {
		f32 xx, yx, zx, wx;
		f32 xy, yy, zy, wy;
		f32 xz, yz, zz, wz;
		f32 xw, yw, zw, ww;
	};
} MtxF;

typedef union  {
	s32 m[4][4];
	struct StructBE {
		u16 intPart[4][4];
		u16 fracPart[4][4];
	};
	long long int force_structure_alignment;
} Mtx;

typedef struct {
	ObjectNode* objNode;
	u32 segment;
	
	struct {
		MemFile file;
		u32 size;
		u32 offset;
	} table;
	
	struct {
		MemFile file;
		u32 offset;
		u32 advanceBy;
	} patch;
	
	MemFile bank;
	MemFile playas;
	MemFile output;
} PlayAsState;

void Matrix_SetTranslateRotateYXZ(MtxF* cmf, f32 translateX, f32 translateY, f32 translateZ, s16 rotX, s16 rotY, s16 rotZ);
void Matrix_MtxFToMtx(MtxF* src, Mtx* dest);
void Matrix_Translate(MtxF* cmf, f32 x, f32 y, f32 z, MatrixMode mode);
void Matrix_Scale(MtxF* cmf, f32 x, f32 y, f32 z, MatrixMode mode);
void Matrix_Rotate(MtxF* cmf, s16 x, s16 y, s16 z, MatrixMode mode);

void PlayAs_Process(PlayAsState* state);
void PlayAs_Free(PlayAsState* state);

s32 Script_Run(const char* script, PlayAsState* state);
