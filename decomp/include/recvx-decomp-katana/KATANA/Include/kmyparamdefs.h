/*--------------------------------------------------------------------------*/

#if !defined( __kmiParamDefs_H )
#define __kmiParamDefs_H

/*----------------------------------------------------------------------------
	Parameter Control Word
----------------------------------------------------------------------------*/

#define KMY_PARAM_ENDOFLIST			(0x00000000)
#define KMY_PARAM_USERTILECLIP		(0x20000000)
#define KMY_PARAM_OBJECTLIST		(0x40000000)
#define KMY_PARAM_POLYGON			(0x80000000)
#define KMY_PARAM_MODIFIER			(0x80000000)
#define KMY_PARAM_SPRITE			(0xa0000000)
#define KMY_PARAM_VERTEX			(0xe0000000)
#define KMY_PARAM_ENDOFSTRIP		(0xf0000000)
#define KMY_PARAM_MASK				(0x0fffffff)

#define KMY_LIST_OPAQUE				(0x00000000)
#define KMY_LIST_OPAQUEMODIFIER		(0x01000000)
#define KMY_LIST_TRANS				(0x02000000)
#define KMY_LIST_TRANSMODIFIER		(0x03000000)
#define KMY_LIST_PUNCHTHROUGH		(0x04000000)
#define KMY_LIST_MASK				(0xf8ffffff)

#define KMY_GROUP_STRIP1			(0x00800000)
#define KMY_GROUP_STRIP2			(0x00840000)
#define KMY_GROUP_STRIP4			(0x00880000)
#define KMY_GROUP_STRIP6			(0x008c0000)
#define KMY_GROUP_STRIP_MASK		(0xff73ffff)

#define KMY_GROUP_CLIP_DISABLE		(0x00800000)
#define KMY_GROUP_CLIP_INSIDE		(0x00820000)
#define KMY_GROUP_CLIP_OUTSIDE		(0x00830000)
#define KMY_GROUP_CLIP_MASK			(0xff7cffff)

#define KMY_GROUP_MASK				(0xff70ffff)

#define KMY_OBJ_NORMAL_POLYGON		(0x00000000)
#define KMY_OBJ_SHADOW_POLYGON		(0x00000080)
#define KMY_OBJ_WTV_POLYGON			(0x000000c0)
#define KMY_OBJ_NORMAL_MODIFIER		(0x00000000)
#define KMY_OBJ_LAST_MODIFIER		(0x00000040)
#define KMY_OBJ_SHADOWMODE_MASK		(0xffffff3f)

#define KMY_OBJ_PACKED_COLOR		(0x00000000)
#define KMY_OBJ_FLOAT_COLOR			(0x00000010)
#define KMY_OBJ_INTENSITY1			(0x00000020)
#define KMY_OBJ_INTENSITY2			(0x00000030)
#define KMY_OBJ_COLORMODE_MASK		(0xffffffcf)

#define KMY_OBJ_TEXTURE				(0x00000008)
#define KMY_OBJ_TEXTURE_MASK		(0xfffffff7)
#define KMY_OBJ_OFFSET				(0x00000004)
#define KMY_OBJ_OFFSET_MASK			(0xfffffffb)
#define KMY_OBJ_GOURAUD				(0x00000002)
#define KMY_OBJ_GOURAUD_MASK		(0xfffffffd)
#define KMY_OBJ_16BITUV				(0x00000001)
#define KMY_OBJ_16BITUV_MASK		(0xfffffffe)

#define KMY_OBJ_MASK				(0xffffff00)

/*----------------------------------------------------------------------------
	ISP/TSP Instruction Word
----------------------------------------------------------------------------*/

#define KMY_COMPARE_NEVER			(0x00000000)
#define KMY_COMPARE_LESS			(0x20000000)
#define KMY_COMPARE_EQUAL			(0x40000000)
#define KMY_COMPARE_LESSEQUAL		(0x60000000)
#define KMY_COMPARE_GREATER			(0x80000000)
#define KMY_COMPARE_NOTEQUAL		(0xa0000000)
#define KMY_COMPARE_GREATEREQUAL	(0xc0000000)
#define KMY_COMPARE_ALWAYS			(0xe0000000)
#define KMY_COMPARE_MASK			(0x1fffffff)

#define KMY_CULLING_NONE			(0x00000000)
#define KMY_CULLING_SMALL			(0x08000000)
#define KMY_CULLING_NEGATIVE		(0x10000000)
#define KMY_CULLING_POSITIVE		(0x18000000)
#define KMY_CULLING_MASK			(0xe7ffffff)

#define KMY_ZWRITE_DISABLE			(0x04000000)
#define KMY_ZWRITE_DISABLE_MASK		(0xfbffffff)
#define KMY_TEXTURE_ENABLE			(0x02000000)
#define KMY_TEXTURE_ENABLE_MASK		(0xfdffffff)
#define KMY_OFFSET_ENABLE			(0x01000000)
#define KMY_OFFSET_ENABLE_MASK		(0xfeffffff)
#define KMY_GOURAUD_ENABLE			(0x00800000)
#define KMY_GOURAUD_ENABLE_MASK		(0xff7fffff)
#define KMY_16BITUV_ENABLE			(0x00400000)
#define KMY_16BITUV_ENABLE_MASK		(0xffbfffff)
#define KMY_CACHE_BYPASS			(0x00200000)
#define KMY_CACHE_BYPASS_MASK		(0xffdfffff)
#define KMY_DCALC_CTRL				(0x00100000)
#define KMY_DCALC_CTRL_MASK			(0xffefffff)

#define KMY_VOLUME_NORMAL			(0x00000000)
#define KMY_VOLUME_INSIDE_LAST		(0x20000000)
#define KMY_VOLUME_OUTSIDE_LAST		(0x40000000)
#define KMY_VOLUME_MASK				(0x1fffffff)

/*----------------------------------------------------------------------------
	TSP Instruction Word
----------------------------------------------------------------------------*/

#define KMY_SRCBLEND_ZERO			(0x00000000)
#define KMY_SRCBLEND_ONE			(0x20000000)
#define KMY_SRCBLEND_OTHER			(0x40000000)
#define KMY_SRCBLEND_INVOTHER		(0x60000000)
#define KMY_SRCBLEND_SRCALPHA		(0x80000000)
#define KMY_SRCBLEND_INVSRCALPHA	(0xa0000000)
#define KMY_SRCBLEND_DSTALPHA		(0xc0000000)
#define KMY_SRCBLEND_INVDSTALPHA	(0xe0000000)
#define KMY_SRCBLEND_MASK			(0x1fffffff)

#define KMY_DSTBLEND_ZERO			(0x00000000)
#define KMY_DSTBLEND_ONE			(0x04000000)
#define KMY_DSTBLEND_OTHER			(0x08000000)
#define KMY_DSTBLEND_INVOTHER		(0x0c000000)
#define KMY_DSTBLEND_SRCALPHA		(0x10000000)
#define KMY_DSTBLEND_INVSRCALPHA	(0x14000000)
#define KMY_DSTBLEND_DSTALPHA		(0x18000000)
#define KMY_DSTBLEND_INVDSTALPHA	(0x1c000000)
#define KMY_DSTBLEND_MASK			(0xe3ffffff)

#define KMY_SRC_SELECT				(0x02000000)
#define KMY_SRC_SELECT_MASK			(0xfdffffff)
#define KMY_DST_SELECT				(0x01000000)
#define KMY_DST_SELECT_MASK			(0xfeffffff)

#define KMY_FOG_LOOKUP				(0x00000000)
#define KMY_FOG_PERVERTEX			(0x00400000)
#define KMY_FOG_NOFOG				(0x00800000)
#define KMY_FOG_LOOKUP2				(0x00c00000)
#define KMY_FOG_MASK				(0xff3fffff)

#define KMY_COLOR_CLAMP				(0x00200000)
#define KMY_COLOR_CLAMP_MASK		(0xffdfffff)
#define KMY_ALPHA_ENABLE			(0x00100000)
#define KMY_ALPHA_ENABLE_MASK		(0xffefffff)
#define KMY_IGNORE_TEXALPHA			(0x00080000)
#define KMY_IGNORE_TEXALPHA_MASK	(0xfff7ffff)

#define KMY_FLIP_U					(0x00040000)
#define KMY_FLIP_V					(0x00020000)
#define KMY_FLIP_MASK				(0xfff9ffff)

#define KMY_CLAMP_U					(0x00010000)
#define KMY_CLAMP_V					(0x00008000)
#define KMY_CLAMP_MASK				(0xfffe7fff)

#define KMY_POINT_SAMPLE			(0x00000000)
#define KMY_BILINEAR				(0x00002000)
#define KMY_TRILINEAR_A				(0x00004000)
#define KMY_TRILINEAR_B				(0x00006000)
#define KMY_FILTER_MASK				(0xffff9fff)

#define KMY_SUPER_SAMPLE			(0x00001000)
#define KMY_SUPER_SAMPLE_MASK		(0xffffefff)

#define KMY_MIPMAP_D_025			(0x00000100)
#define KMY_MIPMAP_D_050			(0x00000200)
#define KMY_MIPMAP_D_075			(0x00000300)
#define KMY_MIPMAP_D_100			(0x00000400)
#define KMY_MIPMAP_D_125			(0x00000500)
#define KMY_MIPMAP_D_150			(0x00000600)
#define KMY_MIPMAP_D_175			(0x00000700)
#define KMY_MIPMAP_D_200			(0x00000800)
#define KMY_MIPMAP_D_225			(0x00000900)
#define KMY_MIPMAP_D_250			(0x00000a00)
#define KMY_MIPMAP_D_275			(0x00000b00)
#define KMY_MIPMAP_D_300			(0x00000c00)
#define KMY_MIPMAP_D_325			(0x00000d00)
#define KMY_MIPMAP_D_350			(0x00000e00)
#define KMY_MIPMAP_D_375			(0x00000f00)
#define KMY_MIPMAP_MASK				(0xfffff0ff)

#define KMY_SHADING_DECAL			(0x00000000)
#define KMY_SHADING_MODULATE		(0x00000040)
#define KMY_SHADING_DECALPLHA		(0x00000080)
#define KMY_SHADING_MODULATEALPHA	(0x000000c0)
#define KMY_SHADING_MASK			(0xffffff3f)

#define KMY_USIZE_8					(0x00000000)
#define KMY_USIZE_16				(0x00000008)
#define KMY_USIZE_32				(0x00000010)
#define KMY_USIZE_64				(0x00000018)
#define KMY_USIZE_128				(0x00000020)
#define KMY_USIZE_256				(0x00000028)
#define KMY_USIZE_512				(0x00000030)
#define KMY_USIZE_1024				(0x00000038)
#define KMY_USIZE_MASK				(0xffffffc7)

#define KMY_VSIZE_8					(0x00000000)
#define KMY_VSIZE_16				(0x00000001)
#define KMY_VSIZE_32				(0x00000002)
#define KMY_VSIZE_64				(0x00000003)
#define KMY_VSIZE_128				(0x00000004)
#define KMY_VSIZE_256				(0x00000005)
#define KMY_VSIZE_512				(0x00000006)
#define KMY_VSIZE_1024				(0x00000007)
#define KMY_VSIZE_MASK				(0xfffffff8)

/*----------------------------------------------------------------------------
	Texture Control Word
----------------------------------------------------------------------------*/

#define KMY_MIPMAP_ENABLE			(0x80000000)
#define KMY_MIPMAP_ENABLE_MASK		(0x7fffffff)
#define KMY_VQ_ENABLE				(0x40000000)
#define KMY_VQ_ENABLE_MASK			(0xbfffffff)

#define KMY_PIXELFORMAT_1555		(0x00000000)
#define KMY_PIXELFORMAT_565			(0x08000000)
#define KMY_PIXELFORMAT_4444		(0x10000000)
#define KMY_PIXELFORMAT_YUV			(0x18000000)
#define KMY_PIXELFORMAT_BUMP		(0x20000000)
#define KMY_PIXELFORMAT_PAL4		(0x28000000)
#define KMY_PIXELFORMAT_PAL8		(0x30000000)
#define KMY_PIXELFORMAT_MASK		(0xc7ffffff)

#define KMY_SCANORDER_ENABLE		(0x04000000)
#define KMY_SCANORDER_ENABLE_MASK	(0xfbffffff)
#define KMY_STRIDE_ENABLE			(0x02000000)
#define KMY_STRIDE_ENABLE_MASK		(0xfdffffff)

#define KMY_PALET_MASK				(0xf81fffff)
#define KMY_TEXADRS_MASK			(0xffe00000)

/*--------------------------------------------------------------------------*/
#endif