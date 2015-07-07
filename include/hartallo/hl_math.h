#ifndef _HARTALLO_MATH_H_
#define _HARTALLO_MATH_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable: 4985) // warning C4985: 'ceil': attributes not present on previous declaration.
#endif
#	include <math.h>
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

HL_BEGIN_DECLS

#define HL_MATH_LN_10						2.3025850929940456840179914546844
#define HL_MATH_PI							3.1415926535897932384626433832795
#define HL_MATH_SQRT_2						1.4142135623730950488016887242097
#define HL_MATH_1_BY_SQRT_2					0.70710678118654752440084436210485
#define HL_MATH_SQRT_1_BY_2					0.70710678118654752440084436210485
#define HL_MATH_MOD_POW2_INT32(val, div)	((val) & ((div) - 1))  // div must be power of 2 // FIXME: rename log2...?
#define HL_MATH_MAX(x,y)					((x) > (y) ? (x) : (y))
#define HL_MATH_MAX_3(x,y,z)				(HL_MATH_MAX(x,HL_MATH_MAX(y,z)))
#define HL_MATH_MIN(x,y)					((x) > (y) ? (y) : (x))
#define HL_MATH_MIN_3(x,y,z)				(HL_MATH_MIN(x,HL_MATH_MIN(y,z)))
#define HL_MATH_MIN_POSITIVE(x, y)			(((x) >= 0 && (y)>=0) ? HL_MATH_MIN((x), (y)) : HL_MATH_MAX((x), (y)))	// (G-245)
#define HL_MATH_ABS(x)						(abs((x)))
#define HL_MATH_ABS_INT32(x)				HL_MATH_ABS((x))//FIXME:((x) ^ ((x) >> 31)) - ((x) >> 31)
#define HL_MATH_MV_DIFF(mv1, mv2)			(HL_MATH_ABS((mv1)[0] - (mv2)[0]) + HL_MATH_ABS((mv1)[1] - (mv2)[1])) // mvDiff( mv1, mv2 ) = Abs( mv1[ 0 ] - mv2[ 0 ] ) + Abs( mv1[ 1 ] - mv2[ 1 ] ) (G-251)
#define HL_MATH_CEIL(x)					(ceil((x)))
#define HL_MATH_CLIP3(x,y,z)				((z)<(x) ? (x) : ((z)>(y) ? (y) : (z)))
#define HL_MATH_CLIP1Y(_x,BitDepthY)			HL_MATH_CLIP3(0, (1 << (BitDepthY)) - 1, (_x))
#define HL_MATH_CLIP1C(_x,BitDepthC)			HL_MATH_CLIP3(0, (1 << (BitDepthC)) - 1, (_x))
#define HL_MATH_CLIP2(y,z)						HL_MATH_CLIP3(0,(y),(z))// // Clip2(max, val) = Clip3(0, max, val)
#define HL_MATH_TAP6FILTER(E, F, G, H, I, J)	((E) - 5*((F) + (I)) + 20*((G) + (H)) + (J))
#define HL_MATH_FLOOR(x)					(floor(x))
#define HL_MATH_INVERSE_RASTER_SCAN(a,b,c,d,e)	((e)==0 ? ((a)%((d)/(b)))*(b) : ((a)/((d)/(b)))*(c))
#define HL_MATH_LOG(x)							log((x))
#define HL_MATH_LOG2(x)							(1/log(2.0)) * log((x))
#define HL_MATH_LOGA0(x)						(log10((x)))
#define HL_MATH_MEDIAN(x,y,z)					((x)+(y)+(z)-HL_MATH_MIN((x),HL_MATH_MIN((y),(z)))-HL_MATH_MAX((x),HL_MATH_MAX((y),(z))))
#define HL_MATH_SIGN(x)							((x)>=0 ? 1 : -1)
#define HL_MATH_ROUND(x)						(HL_MATH_SIGN((x))*HL_MATH_FLOOR(HL_MATH_ABS((x))+0.5))
#define HL_MATH_SQRT(x)							sqrt((x))
#define HL_MATH_COS								cos
#define HL_MATH_SIN								sin
#define HL_MATH_TAN								tan
#define HL_MATH_ASIN							asin
#define HL_MATH_EXP								exp
#define HL_MATH_POW								pow


#if 0 // FIXME: to be fully tested
#	define HL_MATH_IS_POSITIVE_INT32(x)			((x) & 0x80000000))
#	define HL_MATH_IS_NEGATIVE_INT32(x)			((x) & 0x80000000) != 0)
#else
#	define HL_MATH_IS_POSITIVE_INT32(x)			((x) >= 0)
#	define HL_MATH_IS_NEGATIVE_INT32(x)			((x) < 0)
#endif

#ifdef __GNUC__
#	define hl_math_atomic_inc(_ptr_) __sync_fetch_and_add((_ptr_), 1)
#	define hl_math_atomic_dec(_ptr_) __sync_fetch_and_sub((_ptr_), 1)
#elif defined(_MSC_VER)
#	define hl_math_atomic_inc(_ptr_) InterlockedIncrement((_ptr_))
#	define hl_math_atomic_dec(_ptr_) InterlockedDecrement((_ptr_))
#else
#	define hl_math_atomic_inc(_ptr_) ++(*(_ptr_))
#	define hl_math_atomic_dec(_ptr_) --(*(_ptr_))
#endif

// hlMath_Pow2Val[n] = (1 << n) with n in [0-31]
// FIXME: not faster
static const int32_t HL_MATH_POW2_VAL[32/*n*/] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000,
    0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000,
    0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
};

// hlMath_Pow2TrailingZeros[n] = number of trailing zeros with n in [0 - 256] and n Power of 2.
// IMPORTANT: Zero is a special case and returns '0'!
// FIXME: Faster?, try with bits_ctz
// FIXME: rename?
static const int32_t hlMath_Pow2TrailingZeros[257/* n */] = {
    0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0
    , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8
};

HL_ERROR_T hl_math_init_funcs();

hl_bool_t hl_math_allzero16_cpp(HL_ALIGNED(16) const int32_t (*block)[16]);
extern hl_bool_t (*hl_math_allzero16)(HL_ALIGNED(16) const int32_t (*block)[16]);

int32_t hl_math_nnz16_cpp(HL_ALIGNED(16) const int32_t block[16]);
extern int32_t (*hl_math_nnz16)(HL_ALIGNED(16) const int32_t block[16]);

int32_t hl_math_nz16_cpp(HL_ALIGNED(16) const int32_t block[16]);
extern int32_t (*hl_math_nz16)(HL_ALIGNED(16) const int32_t block[16]);

HL_ALWAYS_INLINE static void hl_math_mul2x2_cpp(
    const int32_t (* const m1)[2][2],
    const int32_t (* const m2)[2][2],
    int32_t (* r)[2][2])
{
    (*r)[0][0]= ((*m1)[0][0] * (*m2)[0][0]) + ((*m1)[0][1] * (*m2)[1][0]);
    (*r)[1][0]= ((*m1)[1][0] * (*m2)[0][0]) + ((*m1)[1][1] * (*m2)[1][0]);
    (*r)[0][1]= ((*m1)[0][0] * (*m2)[0][1]) + ((*m1)[0][1] * (*m2)[1][1]);
    (*r)[1][1]= ((*m1)[1][0] * (*m2)[0][1]) + ((*m1)[1][1] * (*m2)[1][1]);
}

#define HL_MATH_DOT_PRODUCT4X1_CPP(a, b, r) \
	*(r) = (((a)[0] * (b)[0]) + ((a)[1] * (b)[1]) + ((a)[2] * (b)[2]) + ((a)[3] * (b)[3]))
HL_ALWAYS_INLINE static void hl_math_dot_product4x1_cpp(HL_ALIGNED(16) const int32_t a[4], HL_ALIGNED(16) const int32_t b[4], int32_t* r)
{
    HL_MATH_DOT_PRODUCT4X1_CPP(a, b, r);
}
void (*hl_math_dot_product4x1)(HL_ALIGNED(16) const int32_t a[4], HL_ALIGNED(16) const int32_t b[4], int32_t* r);


// FIXME: add INTRIN and ASM versions using "_mm_sad_epu8"
int32_t hl_math_sad4x4_u8_cpp(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride);
extern int32_t (*hl_math_sad4x4_u8)(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride);

// FIXME: add INTRIN and ASM versions
int32_t hl_math_sad4x4_u32_cpp(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride);

// FIXME: add INTRIN and ASM
int32_t hl_math_satd4x4_u8_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride);
extern int32_t (*hl_math_satd4x4_u8)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride);

// FIXME: add INTRIN and ASM versions
int32_t hl_math_ssd4x4_u8_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride);
extern int32_t (*hl_math_ssd4x4_u8)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride);

// FIXME: add INTRIN and ASM versions
int32_t hl_math_ssd4x4_u32_cpp(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride);
extern int32_t (*hl_math_ssd4x4_u32)(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride);

// FIXME: add INTRIN and ASM versions
int32_t hl_math_ssd4x4_u8x32_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride);
extern int32_t (*hl_math_ssd4x4_u8x32)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride);

// Mean Absolute Error (MAE)
// FIXME: add INTRIN and ASM versions
int32_t hl_math_mae4x4_u8_cpp(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]);
extern int32_t (*hl_math_mae4x4_u8)(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]);

// FIXME: add INTRIN and ASM versions
// Mean Squared Error (MSE)
int32_t hl_math_mse4x4_u8_cpp(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]);
extern int32_t (*hl_math_mse4x4_u8)(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]);

#define HL_MATH_MUL4X4_CPP(block1, block2, out) \
{ \
	int32_t i,j; \
	for (i=0;i<4;++i){ \
		for (j=0;j<4;++j){ \
			(out)[i][j]= (block1)[i][0]*(block2)[0][j]; \
			(out)[i][j]+= (block1)[i][1]*(block2)[1][j]; \
			(out)[i][j]+= (block1)[i][2]*(block2)[2][j]; \
			(out)[i][j]+= (block1)[i][3]*(block2)[3][j]; \
		} \
	} \
}
HL_ALWAYS_INLINE static void hl_math_mul4x4_cpp(HL_ALIGNED(16) const int32_t a[4][4], HL_ALIGNED(16) const int32_t b[4][4], HL_ALIGNED(16) int32_t r[4][4])
{
    HL_MATH_MUL4X4_CPP(a, b, r);
}
void (*hl_math_mul4x4)(HL_ALIGNED(16) const int32_t a[4][4], HL_ALIGNED(16) const int32_t b[4][4], HL_ALIGNED(16) int32_t r[4][4]);


#define hl_math_mul2x2(block1, block2, out) \
	(out)[0][0]= ((block1)[0][0] * (block2)[0][0]) + ((block1)[0][1] * (block2)[1][0]); \
	(out)[1][0]= ((block1)[1][0] * (block2)[0][0]) + ((block1)[1][1] * (block2)[1][0]); \
	(out)[0][1]= ((block1)[0][0] * (block2)[0][1]) + ((block1)[0][1] * (block2)[1][1]); \
	(out)[1][1]= ((block1)[1][0] * (block2)[0][1]) + ((block1)[1][1] * (block2)[1][1]);


// FIXME: SIMD
HL_ALWAYS_INLINE static int32_t hl_math_activity16x16(const int32_t block[16][4][4])
{
    int32_t i,j = 0,k,activity = 0;
    // FIXME: explicit
    // FIXME: SIMD
    for (k=0; k<16; ++k) {
        for (i=0; i<3; ++i) {
            activity += HL_MATH_ABS_INT32(block[k][i][j] - block[k][i+1][j]);
            activity += HL_MATH_ABS_INT32(block[k][i][j + 1] - block[k][i+1][j + 1]);
            activity += HL_MATH_ABS_INT32(block[k][i][j + 2] - block[k][i+1][j + 2]);
            activity += HL_MATH_ABS_INT32(block[k][i][j + 3] - block[k][i+1][j + 3]);
        }
        for(i=0; i<4; ++i) {
            activity += HL_MATH_ABS_INT32(block[k][i][j] - block[k][i][j+1]);
            activity += HL_MATH_ABS_INT32(block[k][i][j + 1] - block[k][i][j+2]);
            activity += HL_MATH_ABS_INT32(block[k][i][j + 2] - block[k][i][j+3]);
        }
    }
    return activity;
}

// Clip3
HL_ALWAYS_INLINE static void hl_math_clip3_4x1_cpp(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret)
{
    ((int32_t*)ret)[0] = HL_MATH_CLIP3(((const int32_t*)min)[0], ((const int32_t*)max)[0], ((const int32_t*)val)[0]);
    ((int32_t*)ret)[1] = HL_MATH_CLIP3(((const int32_t*)min)[1], ((const int32_t*)max)[1], ((const int32_t*)val)[1]);
    ((int32_t*)ret)[2] = HL_MATH_CLIP3(((const int32_t*)min)[2], ((const int32_t*)max)[2], ((const int32_t*)val)[2]);
    ((int32_t*)ret)[3] = HL_MATH_CLIP3(((const int32_t*)min)[3], ((const int32_t*)max)[3], ((const int32_t*)val)[3]);
}
extern void (*hl_math_clip3_4x1)(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);

// Clip2(max, val) = Clip3(0, max, val)
HL_ALWAYS_INLINE static void hl_math_clip2_4x1_cpp(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret)
{
    ((int32_t*)ret)[0] = HL_MATH_CLIP3(0, ((const int32_t*)max)[0], ((const int32_t*)val)[0]);
    ((int32_t*)ret)[1] = HL_MATH_CLIP3(0, ((const int32_t*)max)[1], ((const int32_t*)val)[1]);
    ((int32_t*)ret)[2] = HL_MATH_CLIP3(0, ((const int32_t*)max)[2], ((const int32_t*)val)[2]);
    ((int32_t*)ret)[3] = HL_MATH_CLIP3(0, ((const int32_t*)max)[3], ((const int32_t*)val)[3]);
}
extern void (*hl_math_clip2_4x1)(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);

// Clip1Y(x, BitDepth) / Clip1C(x, BitDepth) = Clip3(0, (1 << BitDepth) - 1, x) = Clip2((1 << (BitDepth)) - 1, x) = Clip2(MaxPixelVal, x)
// IMPORTANT: /!\ Only "BitDepth[0]" is used. Check comment on the intrinsic function for more information.
HL_ALWAYS_INLINE static void hl_math_clip1_4x1_cpp(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret)
{
    ((int32_t*)ret)[0] = HL_MATH_CLIP1Y(((const int32_t*)val)[0], ((const int32_t*)BitDepth)[0]);
    ((int32_t*)ret)[1] = HL_MATH_CLIP1Y(((const int32_t*)val)[1], ((const int32_t*)BitDepth)[0]);
    ((int32_t*)ret)[2] = HL_MATH_CLIP1Y(((const int32_t*)val)[2], ((const int32_t*)BitDepth)[0]);
    ((int32_t*)ret)[3] = HL_MATH_CLIP1Y(((const int32_t*)val)[3], ((const int32_t*)BitDepth)[0]);
}
extern void (*hl_math_clip1_4x1)(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret);

// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
HL_ALWAYS_INLINE static void hl_math_tap6filter4x1_u32_cpp(
    const hl_int128_t* E,
    const hl_int128_t* F,
    const hl_int128_t* G,
    const hl_int128_t* H,
    const hl_int128_t* I,
    const hl_int128_t* J,
    hl_int128_t* RET)
{
    ((int32_t*)RET)[0] = HL_MATH_TAP6FILTER(((const int32_t*)E)[0], ((const int32_t*)F)[0], ((const int32_t*)G)[0], ((const int32_t*)H)[0], ((const int32_t*)I)[0], ((const int32_t*)J)[0]);
    ((int32_t*)RET)[1] = HL_MATH_TAP6FILTER(((const int32_t*)E)[1], ((const int32_t*)F)[1], ((const int32_t*)G)[1], ((const int32_t*)H)[1], ((const int32_t*)I)[1], ((const int32_t*)J)[1]);
    ((int32_t*)RET)[2] = HL_MATH_TAP6FILTER(((const int32_t*)E)[2], ((const int32_t*)F)[2], ((const int32_t*)G)[2], ((const int32_t*)H)[2], ((const int32_t*)I)[2], ((const int32_t*)J)[2]);
    ((int32_t*)RET)[3] = HL_MATH_TAP6FILTER(((const int32_t*)E)[3], ((const int32_t*)F)[3], ((const int32_t*)G)[3], ((const int32_t*)H)[3], ((const int32_t*)I)[3], ((const int32_t*)J)[3]);
}
extern void (*hl_math_tap6filter4x1_u32)(const hl_int128_t* E, const hl_int128_t* F, const hl_int128_t* G, const hl_int128_t* H, const hl_int128_t* I, const hl_int128_t* J, hl_int128_t* RET);
extern void (*hl_math_tap6filter4x2_u16_partial)(const hl_int128_t* e, const hl_int128_t* f, const hl_int128_t* g, const hl_int128_t* h, const hl_int128_t* i, const hl_int128_t* j, hl_int128_t* ret_lo, hl_int128_t* ret_hi);

// RET[idx] = Clip2(MAX[0], (M1[idx] + M2[idx]))
#define _HL_MATH_ADDCLIP(_m1_, _m2_, _max_, _ret_, _idx_) (_ret_)[(_idx_)] = (_m1_)[_idx_] + (_m2_)[(_idx_)]; (_ret_)[(_idx_)] = HL_MATH_CLIP2((_max_)[0], (_ret_)[(_idx_)])
// for(idx=0...3) { RET[idx] = Clip2(MAX[0], (M1[idx] + M2[idx])) }
#define _HL_MATH_ADDCLIP_3(_m1_, _m2_, _max_, _ret_)_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),0);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),1);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),2);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),3);
// for(idx=0...7) { RET[idx] = Clip2(MAX[0], (M1[idx] + M2[idx])) }
#define _HL_MATH_ADDCLIP_7(_m1_, _m2_, _max_, _ret_) \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),0);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),1);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),2);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),3); \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),4);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),5);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),6);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),7); \
// for(idx=0...15) { RET[idx] = Clip2(MAX[0], (M1[idx] + M2[idx])) }
#define _HL_MATH_ADDCLIP_15(_m1_, _m2_, _max_, _ret_) \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),0);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),1);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),2);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),3); \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),4);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),5);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),6);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),7); \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),8);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),9);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),10);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),11); \
	_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),12);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),13);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),14);_HL_MATH_ADDCLIP((_m1_),(_m2_),(_max_),(_ret_),15); \
 
// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4"
HL_ALWAYS_INLINE static void hl_math_addclip_4x4_cpp(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    _HL_MATH_ADDCLIP_3(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,max,ret);
}
extern void (*hl_math_addclip_4x4)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

// RET = Clip2(255, (M1 + M2))
// Strides must be multiple of "4"
HL_ALWAYS_INLINE static void hl_math_addclip_4x4_u8xi32_cpp(
    HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride
)
{
	static const int32_t __max[4] = { 255, 255, 255, 255 };
    _HL_MATH_ADDCLIP_3(m1,m2,__max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,__max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,__max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_3(m1,m2,__max,ret);
}
extern void (*hl_math_addclip_4x4_u8xi32)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride);

// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4" and more than 8
HL_ALWAYS_INLINE static void hl_math_addclip_8x8_cpp(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_7(m1,m2,max,ret);
}
extern void (*hl_math_addclip_8x8)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4" and more than 16
HL_ALWAYS_INLINE static void hl_math_addclip_16x16_cpp(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_ADDCLIP_15(m1,m2,max,ret);
}
extern void (*hl_math_addclip_16x16)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

void hl_math_sub4x4_u8x32_cpp(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
extern void (*hl_math_sub4x4_u8x32)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

void hl_math_sub4x4_u8x8_cpp(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const uint8_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
extern void (*hl_math_sub4x4_u8x8)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const uint8_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

int32_t hl_math_homogeneousity8x8_u8_cpp(const uint8_t* non_hz_vt_edge_start, int32_t m1_stride);
extern int32_t (*hl_math_homogeneousity8x8_u8)(const uint8_t* non_hz_vt_edge_start, int32_t m1_stride);

HL_END_DECLS

#endif /* _HARTALLO_MATH_H_ */
