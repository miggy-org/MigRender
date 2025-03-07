// filestructs.h - defines structures used for file I/O
//

#pragma once

#include "defines.h"
#include "matrix.h"

// file header identifier and current version
#define	MIG_IDENT				0x12344321
#define MIG_VERSION				1

_MIGRENDER_BEGIN

// file types
enum class MigType
{
	None = 0,
	Model,
	Object,
	Package
};

// header found at the start of every file, regardless of type (model, object, package, etc)
struct FILE_HEADER
{
	dword ident;
	dword version;
	MigType type;
	dword reserved;
};

// data block types
enum class BlockType
{
	None = 0,
	Model,
	Background,
	Camera,
	ImageMap,
	Group,
	DirLight,
	PtLight,
	SpotLight,
	Sphere,
	Polygon,
	PolyLattice,
	PolyCurves,
	PolyIndices,
	PolyNorms,
	Texture,
	PTexture,
	PTextureCoords,
	MetaData,
	PackageTOC,
	BaseObj,

	EndRange = 998,
	EndFile = 999
};

// header found before every data block
struct FILE_TAG
{
	BlockType type;
	dword size;
};

struct FILE_MODEL
{
	COLOR ambient;
};

struct FILE_BASEOBJ
{
	char name[40];
};

struct FILE_BACKGROUND
{
	COLOR cn;
	COLOR ce;
	COLOR cs;

	char image[20];
	ImageResize resize;

	CMatrix_f tm;
};

struct FILE_CAMERA
{
	double dist;
	double ulen;
	double vlen;

	CMatrix_f tm;
};

struct FILE_IMAGEMAP
{
	char title[20];
	char path[256];
	ImageFormat fmt;
};

struct FILE_GROUP
{
	CMatrix_f tm;
};

struct FILE_LIGHT
{
	COLOR col;
	double hlit;
	double sscale;

	CMatrix_f tm;

	CPt ori;
	CUnitVector dir;
	double fulldist;
	double dropdist;
	double droplen;
	double concentration;

	dword flags;
	dword reserved;
};

struct FILE_DIR_LIGHT
{
	FILE_LIGHT base;

	CUnitVector dir;
};

struct FILE_PT_LIGHT
{
	FILE_LIGHT base;

	CPt ori;
	double fulldist;
	double dropdist;
	double droplen;
};

struct FILE_SPOT_LIGHT
{
	FILE_LIGHT base;

	CPt ori;
	CUnitVector dir;
	double fulldist;
	double dropdist;
	double droplen;
	double concentration;
};

struct FILE_OBJECT
{
	COLOR diff;
	COLOR spec;
	COLOR refl;
	COLOR refr;
	COLOR glow;
	double index;

	CMatrix_f tm;

	dword flags;
	TextureFilter filter;
};

struct FILE_SPHERE
{
	FILE_OBJECT base;

	CPt ori;
	double rad;
};

struct FILE_POLYGON
{
	FILE_OBJECT base;

	int num_lattice;
	int num_curves;
	int num_ind;
	int num_norms;
};

struct FILE_TEXTURE
{
	TextureMapType type;
	TextureMapOp op;
	dword flags;
	char map[20];
	UVC uvMin;
	UVC uvMax;

	int btol;
	double bscale;
};

struct FILE_PTEXTURE
{
	FILE_TEXTURE base;

	int num_coords;
	dword reserved;
};

struct FILE_TOC_ENTRY
{
	char item[20];
	int offset;
};

_MIGRENDER_END
