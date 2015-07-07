// FIXME: add DOubango Telecom Copyright
/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/
#include "hartallo/h264/hl_codec_264_rc.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/hl_frame.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_debug.h"

#define HL_CODEC_264_RC_CIF_NUM_MB_LUMA		396 // Number of macroblocks (luminance) in CIF picture
#define HL_CODEC_264_RC_CIF_NPIX			101376// Number of pixels (luminance only) in CIF picture.
#define HL_CODEC_264_RC_QCIF_NPIX			25344// Number of pixels (luminance only) in QCIF picture.

#if 1 /*=== Implementation based on JM reference ===*/

#if !defined(RC_MODEL_HISTORY)
#	define RC_MODEL_HISTORY			21
#endif
#if !defined(MAX_RC_MODE)
#	define MAX_RC_MODE               3
#endif
#if !defined(RC_MAX_TEMPORAL_LEVELS)
#	define RC_MAX_TEMPORAL_LEVELS    5
#endif

#ifdef FALSE
#  define Boolean int
#else
typedef enum {
    FALSE,
    TRUE
} Boolean;
#endif

typedef enum {
    FRAME_CODING         = 0,
    FIELD_CODING         = 1,
    ADAPTIVE_CODING      = 2,
    FRAME_MB_PAIR_CODING = 3
} CodingType;

/* generic rate control variables */
typedef struct rc_generic {
    // RC flags
    int   TopFieldFlag;
    int   FieldControl;
    int   FieldFrame;
    int   NoGranularFieldRC;
    // bits stats
    int   NumberofHeaderBits;
    int   NumberofTextureBits;
    int   NumberofBasicUnitHeaderBits;
    int   NumberofBasicUnitTextureBits;
    // frame stats
    int   NumberofGOP;
    int   NumberofCodedBFrame;
    // MAD stats
    int64_t TotalMADBasicUnit;
    int   *MADofMB;
    // buffer and budget
    int64_t CurrentBufferFullness; //LIZG 25/10/2002
    int64_t RemainingBits;
    // bit allocations for RC_MODE_3
    int   RCPSliceBits;
    int   RCISliceBits;
    int   RCBSliceBits[RC_MAX_TEMPORAL_LEVELS];
    int   temporal_levels;
    int   hierNb[RC_MAX_TEMPORAL_LEVELS];
    int   NPSlice;
    int   NISlice;
} RCGeneric;

typedef struct rc_quadratic {
    float  bit_rate;
    float  frame_rate;
    float  PrevBitRate;           //LIZG  25/10/2002
    double GAMMAP;                //LIZG, JVT019r1
    double BETAP;                 //LIZG, JVT019r1
    double GOPTargetBufferLevel;
    double TargetBufferLevel;     //LIZG 25/10/2002
    double AveWp;
    double AveWb;
    int    MyInitialQp;
    int    PAverageQp;
    double PreviousPictureMAD;
    double MADPictureC1;
    double MADPictureC2;
    double PMADPictureC1;
    double PMADPictureC2;
    double PPictureMAD [RC_MODEL_HISTORY];
    double PictureMAD  [RC_MODEL_HISTORY];
    double ReferenceMAD[RC_MODEL_HISTORY];
    double m_rgQp      [RC_MODEL_HISTORY];
    double m_rgRp      [RC_MODEL_HISTORY];
    double Pm_rgQp     [RC_MODEL_HISTORY];
    double Pm_rgRp     [RC_MODEL_HISTORY];

    double m_X1;
    double m_X2;
    double Pm_X1;
    double Pm_X2;
    int    Pm_Qp;
    int    Pm_Hp;

    int    MADm_windowSize;
    int    m_windowSize;
    int    m_Qc;

    int    PPreHeader;
    int    PrevLastQP; // QP of the second-to-last coded frame in the primary layer
    int    CurrLastQP; // QP of the last coded frame in the primary layer
    int    NumberofBFrames;
    // basic unit layer rate control
    int    TotalFrameQP;
    int    NumberofBasicUnit;
    int    PAveHeaderBits1;
    int    PAveHeaderBits2;
    int    PAveHeaderBits3;
    int    PAveFrameQP;
    int    TotalNumberofBasicUnit;
    int    CodedBasicUnit;

    int    NumberofCodedPFrame;
    int    TotalQpforPPicture;
    int    NumberofPPicture;

    double CurrentFrameMAD;
    double CurrentBUMAD;
    double TotalBUMAD;
    double PreviousFrameMAD;
    double PreviousWholeFrameMAD;

    int    DDquant;
    unsigned int    MBPerRow;
    int    QPLastPFrame;
    int    QPLastGOP;

    // adaptive field/frame coding
    int    FieldQPBuffer;
    int    FrameQPBuffer;
    int    FrameAveHeaderBits;
    int    FieldAveHeaderBits;
    double *BUPFMAD;
    double *BUCFMAD;
    double *FCBUCFMAD;
    double *FCBUPFMAD;

    Boolean GOPOverdue;
    int64_t   Pprev_bits;

    // rate control variables
    int    Xp, Xb;
    int    Target;
    int    TargetField;
    int    Np, Nb, bits_topfield;
    // HRD consideration
    int    UpperBound1, UpperBound2, LowerBound;
    double Wp, Wb; // complexity weights
    double DeltaP;
    int    TotalPFrame;
    int    PMaxQpChange;

    int    bitdepth_qp_scale; // support negative QPs (bitdepth > 8-bits per component)

    // simple encoder buffer simulation
    int    enc_buf_curr;

} RCQuadratic;

typedef enum {
    RC_MODE_0 = 0,
    RC_MODE_1 = 1,
    RC_MODE_2 = 2,
    RC_MODE_3 = 3
} RCModeType;

static const float THETA = 1.3636F;
static const float OMEGA = 0.9F;
static const float MINVALUE = 4.0F;

static RCModeType __RCUpdateMode = RC_MODE_0;
static const int __NumberBFrames = 0;
static double __RCISliceBitRatio = 1.0;
static double __RCBSliceBitRatio[RC_MAX_TEMPORAL_LEVELS] = { 0.5, 0.25, 0.25, 0.25, 0.25 };
static const int __MbInterlace = 0;
static const CodingType __PicInterlace = FRAME_CODING;
static const int __channel_type = 0;
static const int __HierarchicalCoding = 0;
static const int __p_curr_frm_struct_layer = 0;
static const double __RCBoverPRatio = 0.45;
static const double __RCIoverPRatio = 3.80;
static const int __RDPictureDecision = 1;
static const int __RCMaxQPChange = 4;

typedef struct hl_codec_264_rc_gop_s {
    HL_DECLARE_OBJECT;

}
hl_codec_264_rc_gop_t;

static HL_ERROR_T _rc_init_gop_params(hl_codec_264_t* p_codec);
static HL_ERROR_T _rc_init_GOP(hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int np, int nb);
static HL_ERROR_T _rc_init_frame(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5);
static void _updateQPInterlace( RCQuadratic *p_quad, RCGeneric *p_gen );
static void _updateQPNonPicAFF( const hl_codec_264_nal_sps_t *active_sps, RCQuadratic *p_quad );
static void _updateQPInterlaceBU( RCQuadratic *p_quad, RCGeneric *p_gen );
static void _updateModelQPFrame( RCQuadratic *p_quad, int m_Bits );
static void _updateBottomField( hl_codec_264_t *p_codec, RCQuadratic *p_quad );
static int _updateFirstP( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield );
static int _updateFirstBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield );
static int _updateNegativeTarget( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield, int m_Qp );
static void _updateLastBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield );
static void _updateModelQPBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, int m_Qp );
static int _Qstep2QP( double Qstep, int qp_offset );
static double _QP2Qstep( int QP );
static void _predictCurrPicMAD( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen );
static HL_ERROR_T _rc_allocate_memory( hl_codec_264_t* p_codec );
static HL_ERROR_T _rc_alloc_quadratic( hl_codec_264_t* p_codec, RCQuadratic **p_quad );
static void _rc_free_quadratic(RCQuadratic **p_quad);
static void _rc_free_generic(RCGeneric **p_quad);
static HL_ERROR_T _rc_copy_quadratic( hl_codec_264_t* p_codec, RCQuadratic *dst, RCQuadratic *src );
static HL_ERROR_T _rc_copy_generic(hl_codec_264_t* p_codec, RCGeneric *dst, RCGeneric *src);
static HL_ERROR_T _rc_init_sequence(hl_codec_264_t* p_codec);
static HL_ERROR_T _rc_init_seq(hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen);
static void _rc_store_slice_header_bits( hl_codec_264_t* p_codec, int32_t len );
static void _rc_update_pict_frame(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, RCQuadratic *p_quad, RCGeneric *p_gen, int nbits);
static int32_t _updateComplexity( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, Boolean is_updated, Boolean is_B_slice, int nbits );
static void _updatePparams( RCQuadratic *p_quad, RCGeneric *p_gen, int complexity );
static void _updateBparams( RCQuadratic *p_quad, RCGeneric *p_gen, int complexity );
static void _rc_update_picture( hl_codec_264_t* p_codec, int bits, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5 );
static void _updateRCModel (hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5);
static double _ComputeFrameMAD(hl_codec_264_t* p_codec);

static HL_ERROR_T _hl_codec_264_rc_gop_create(hl_codec_264_t* p_codec, hl_codec_264_rc_gop_t** pp_rc_gop)
{
    extern const hl_object_def_t *hl_codec_264_rc_gop_def_t;
    if (!pp_rc_gop || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_rc_gop = hl_object_create(hl_codec_264_rc_gop_def_t);
    if (!*pp_rc_gop) {
        return HL_ERROR_OUTOFMEMMORY;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_init(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err;
    const hl_codec_264_nal_sps_t* pc_sps;

    if (p_codec->encoder.rc.b_initialized) {
        return HL_ERROR_SUCCESS;
    }

    pc_sps = p_codec->sps.pc_active;

    p_codec->encoder.rc.size = p_codec->encoder.pc_frame->data_size[0];
    p_codec->encoder.rc.qp = p_codec->pc_base->qp;
    p_codec->encoder.rc.bit_rate = p_codec->pc_base->rc_bitrate;
    p_codec->encoder.rc.frame_rate = (p_codec->pc_base->fps.den / p_codec->pc_base->fps.num);
    p_codec->encoder.rc.bitdepth_luma_qp_scale = 0;
    p_codec->encoder.rc.NumberofCodedMacroBlocks = 0;
    p_codec->encoder.rc.SeinitialQP = 0;
    p_codec->encoder.rc.idr_period = p_codec->pc_base->gop_size;
    p_codec->encoder.rc.intra_period = p_codec->pc_base->gop_size;
    p_codec->encoder.rc.no_frames = p_codec->pc_base->gop_size;
    p_codec->encoder.rc.number = 0;
    p_codec->encoder.rc.curr_frm_idx = 0;
    p_codec->encoder.rc.RDPictureDecision = __RDPictureDecision;
    p_codec->encoder.rc.BasicUnit = (p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits);

    err = _rc_allocate_memory(p_codec);
    if (err) {
        return err;
    }

    err = _rc_init_sequence(p_codec);
    if (err) {
        return err;
    }

    p_codec->encoder.rc.b_initialized = err ? HL_FALSE : HL_TRUE;

    return err;
}

HL_ERROR_T hl_codec_264_rc_deinit(hl_codec_264_t* p_codec)
{
    if (p_codec->encoder.rc.p_rc_gen) {
        _rc_free_generic(&p_codec->encoder.rc.p_rc_gen);
    }
    if (p_codec->encoder.rc.p_rc_gen_init) {
        _rc_free_generic(&p_codec->encoder.rc.p_rc_gen_init);
    }
    if (p_codec->encoder.rc.p_rc_gen_best) {
        _rc_free_generic(&p_codec->encoder.rc.p_rc_gen_best);
    }

    if (p_codec->encoder.rc.p_rc_quad) {
        _rc_free_quadratic(&p_codec->encoder.rc.p_rc_quad);
    }
    if (p_codec->encoder.rc.p_rc_quad_init) {
        _rc_free_quadratic(&p_codec->encoder.rc.p_rc_quad_init);
    }
    if (p_codec->encoder.rc.p_rc_quad_best) {
        _rc_free_quadratic(&p_codec->encoder.rc.p_rc_quad_best);
    }

    p_codec->encoder.rc.b_initialized = HL_FALSE;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_start_gop(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (!p_codec->encoder.rc.b_initialized) {
        HL_DEBUG_ERROR("[RC] Not initialized yet");
        return HL_ERROR_INVALID_STATE;
    }

    err = _rc_init_gop_params(p_codec);
    if (err) {
        return err;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_end_gop(hl_codec_264_t* p_codec)
{
    p_codec->encoder.rc.i_gop_idx++;

    // FIXME: is it correct?
    p_codec->encoder.rc.number = 0;
    p_codec->encoder.rc.curr_frm_idx = 0;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_start_frame(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    HL_CODEC_264_SLICE_TYPE_T SliceTypeModulo5 = (p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA) ? HL_CODEC_264_SLICE_TYPE_I : HL_CODEC_264_SLICE_TYPE_P;

    if (!p_codec->encoder.rc.b_initialized) {
        HL_DEBUG_ERROR("[RC] Not initialized yet");
        return HL_ERROR_INVALID_STATE;
    }

    err = _rc_init_frame(p_codec, SliceTypeModulo5);
    if (err) {
        return err;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_store_slice_header_bits( hl_codec_264_t* p_codec, int32_t len )
{
    _rc_store_slice_header_bits(p_codec, len);
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_rc_handle_mb( struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t* p_qp )
{
    int basicunit;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;
    *p_qp = p_codec->encoder.rc.qp;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    if (basicunit != pc_sps->uPicSizeInMapUnits) {
        HL_DEBUG_ERROR("MB level RC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_rc_store_mad_and_bitscount( hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t i_mad, int32_t i_hdr_bitscount,  int32_t i_data_bitscount)
{
    int basicunit;
    RCGeneric *p_gen = p_codec->encoder.rc.p_rc_gen;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    p_gen->MADofMB[p_mb->u_addr] = i_mad;
    p_gen->NumberofTextureBits += i_data_bitscount;
    p_gen->NumberofHeaderBits  += i_hdr_bitscount;

    if (basicunit < (int)pc_sps->uPicSizeInMapUnits) {
        p_gen->TotalMADBasicUnit += p_gen->MADofMB[p_mb->u_addr];
        p_gen->NumberofBasicUnitHeaderBits  += i_hdr_bitscount;
        p_gen->NumberofBasicUnitTextureBits += i_data_bitscount;
    }
    return HL_ERROR_SUCCESS;
}

// To be called once the frame is completely encoded.
HL_ERROR_T hl_codec_264_rc_end_frame(hl_codec_264_t* p_codec, int32_t nBits)
{
    HL_CODEC_264_SLICE_TYPE_T SliceTypeModulo5 = (p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA) ? HL_CODEC_264_SLICE_TYPE_I : HL_CODEC_264_SLICE_TYPE_P;
    _rc_update_pict_frame(p_codec, SliceTypeModulo5, p_codec->encoder.rc.p_rc_quad, p_codec->encoder.rc.p_rc_gen, nBits); // Top then Bottom field
    _rc_update_picture( p_codec, nBits, SliceTypeModulo5 ); // To be called when both Top and Bottom field are encoded

    HL_DEBUG_INFO("[RC] gop_idx=%d frame_idx=%d, qp=%d, bits=%d", p_codec->encoder.rc.i_gop_idx, p_codec->encoder.rc.curr_frm_idx, p_codec->encoder.rc.qp, nBits);

    p_codec->encoder.rc.number++;
    p_codec->encoder.rc.curr_frm_idx++;
    return HL_ERROR_SUCCESS;
}

// FIXME: remove
HL_ERROR_T hl_codec_264_rc_pict_start(hl_codec_264_t* p_codec)
{
    return HL_ERROR_SUCCESS;
}

// FIXME: remove
HL_ERROR_T hl_codec_264_rc_pict_end(hl_codec_264_t* p_codec)
{
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _rc_copy_quadratic( hl_codec_264_t* p_codec, RCQuadratic *dst, RCQuadratic *src )
{
    const hl_codec_264_nal_sps_t* pc_sps;
    int rcBufSize, basicunit;
    double   *tmpBUPFMAD, *tmpBUCFMAD, *tmpFCBUPFMAD, *tmpFCBUCFMAD;

    pc_sps = p_codec->sps.pc_active;

    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    rcBufSize = pc_sps->uPicSizeInMapUnits / basicunit;
    /* buffer original addresses for which memory has been allocated */
    tmpBUPFMAD = dst->BUPFMAD;
    tmpBUCFMAD = dst->BUCFMAD;
    tmpFCBUPFMAD = dst->FCBUPFMAD;
    tmpFCBUCFMAD = dst->FCBUCFMAD;

    /* copy object */
    memcpy( (void *)dst, (void *)src, sizeof(RCQuadratic) );

    /* restore original addresses */
    dst->BUPFMAD   = tmpBUPFMAD;
    dst->BUCFMAD   = tmpBUCFMAD;
    dst->FCBUPFMAD = tmpFCBUPFMAD;
    dst->FCBUCFMAD = tmpFCBUCFMAD;

    /* copy MADs */
    memcpy( (void *)dst->BUPFMAD,   (void *)src->BUPFMAD,   (rcBufSize) * sizeof (double) );
    memcpy( (void *)dst->BUCFMAD,   (void *)src->BUCFMAD,   (rcBufSize) * sizeof (double) );
    memcpy( (void *)dst->FCBUPFMAD, (void *)src->FCBUPFMAD, (rcBufSize) * sizeof (double) );
    memcpy( (void *)dst->FCBUCFMAD, (void *)src->FCBUCFMAD, (rcBufSize) * sizeof (double) );

    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _rc_copy_generic(hl_codec_264_t* p_codec, RCGeneric *dst, RCGeneric *src)
{
    const hl_codec_264_nal_sps_t* pc_sps;
    int *tmpMADofMB;

    pc_sps = p_codec->sps.pc_active;

    /* buffer original addresses for which memory has been allocated */
    tmpMADofMB = dst->MADofMB;

    /* copy object */

    // FIXME: use optimized memcpy()

    // This could be written as: *dst = *src;
    memcpy( (void *)dst, (void *)src, sizeof(RCGeneric) );

    /* restore original addresses */
    dst->MADofMB = tmpMADofMB;

    /* copy MADs */
    memcpy( (void *)dst->MADofMB, (void *)src->MADofMB, pc_sps->uPicSizeInMapUnits * sizeof (int) );

    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _rc_alloc_generic(hl_codec_264_t* p_codec, RCGeneric **p_quad)
{
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;

    *p_quad = (RCGeneric *) hl_memory_malloc ( sizeof( RCGeneric ) );
    if (!*p_quad) {
        HL_DEBUG_ERROR("rc_alloc_generic: rc_alloc_generic");
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*p_quad)->MADofMB = (int *) hl_memory_calloc (pc_sps->uPicSizeInMapUnits, sizeof (int));
    if (!(*p_quad)->MADofMB) {
        HL_DEBUG_ERROR("rc_alloc_generic: (*p_quad)->MADofMB");
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*p_quad)->FieldFrame = 1;
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _rc_alloc_quadratic( hl_codec_264_t* p_codec, RCQuadratic **p_quad )
{
    const hl_codec_264_nal_sps_t* pc_sps;
    int rcBufSize, basicunit;
    RCQuadratic *lprc;
    const hl_codec_264_layer_t* pc_layer;

    pc_layer = p_codec->layers.pc_active;
    pc_sps = p_codec->sps.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    rcBufSize = pc_sps->uPicSizeInMapUnits / basicunit;

    (*p_quad) = (RCQuadratic *) hl_memory_malloc ( sizeof( RCQuadratic ) );
    if (!(*p_quad)) {
        HL_DEBUG_ERROR("rc_alloc_quadratic: (*p_quad)");
        return HL_ERROR_OUTOFMEMMORY;
    }
    lprc = *p_quad;

    lprc->PreviousFrameMAD = 1.0;
    lprc->CurrentFrameMAD = 1.0;
    lprc->Pprev_bits = 0;
    lprc->Target = 0;
    lprc->TargetField = 0;
    lprc->LowerBound = 0;
    lprc->UpperBound1 = INT_MAX;
    lprc->UpperBound2 = INT_MAX;
    lprc->Wp = 0.0;
    lprc->Wb = 0.0;
    lprc->AveWb = 0.0;
    lprc->PAveFrameQP   = p_codec->encoder.rc.qp + p_codec->encoder.rc.bitdepth_luma_qp_scale;
    lprc->m_Qc          = lprc->PAveFrameQP;
    lprc->FieldQPBuffer = lprc->PAveFrameQP;
    lprc->FrameQPBuffer = lprc->PAveFrameQP;
    lprc->PAverageQp    = lprc->PAveFrameQP;
    lprc->MyInitialQp   = lprc->PAveFrameQP;
    lprc->AveWb         = 0.0;

    lprc->BUPFMAD = (double*) hl_memory_calloc ((rcBufSize), sizeof (double));
    if (!lprc->BUPFMAD) {
        HL_DEBUG_ERROR("rc_alloc_quadratic: lprc->BUPFMAD");
        return HL_ERROR_OUTOFMEMMORY;
    }

    lprc->BUCFMAD = (double*) hl_memory_calloc ((rcBufSize), sizeof (double));
    if (!lprc->BUCFMAD) {
        HL_DEBUG_ERROR("rc_alloc_quadratic: lprc->BUCFMAD");
        return HL_ERROR_OUTOFMEMMORY;
    }

    lprc->FCBUCFMAD = (double*) hl_memory_calloc ((rcBufSize), sizeof (double));
    if (!lprc->FCBUCFMAD) {
        HL_DEBUG_ERROR("rc_alloc_quadratic: lprc->FCBUCFMAD");
        return HL_ERROR_OUTOFMEMMORY;
    }

    lprc->FCBUPFMAD = (double*) hl_memory_calloc ((rcBufSize), sizeof (double));
    if (!lprc->FCBUPFMAD) {
        HL_DEBUG_ERROR("rc_alloc_quadratic: lprc->FCBUPFMAD");
        return HL_ERROR_OUTOFMEMMORY;
    }

    return HL_ERROR_SUCCESS;
}

static void _rc_free_generic(RCGeneric **p_quad)
{
    hl_memory_free(&(*p_quad)->MADofMB);
    hl_memory_free(p_quad);
}

static void _rc_free_quadratic(RCQuadratic **p_quad)
{
    hl_memory_free(&(*p_quad)->BUPFMAD);
    hl_memory_free(&(*p_quad)->BUCFMAD);
    hl_memory_free(&(*p_quad)->FCBUCFMAD);
    hl_memory_free(&(*p_quad)->FCBUPFMAD);
    hl_memory_free(p_quad);
}

static HL_ERROR_T _rc_init_gop_params(hl_codec_264_t* p_codec)
{
    int np, nb;
    RCQuadratic *p_quad = p_codec->encoder.rc.p_rc_quad;
    RCGeneric *p_gen = p_codec->encoder.rc.p_rc_gen;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_layer_t* pc_layer;

    // FIXME: "curr_frm_idx" not updated
    // FIXME: "no_frames" not updated (=gop_size)

    pc_layer = p_codec->layers.pc_active;

    switch( __RCUpdateMode ) {
    case RC_MODE_1:
    case RC_MODE_3:
        if ( !(p_codec->encoder.rc.curr_frm_idx) ) {
            /* number of P frames */
            np = (int) (p_codec->encoder.rc.no_frames + __NumberBFrames) / (1 + __NumberBFrames) - 1 + 1; // approximate but good enough (hack...)
            /* number of B frames */
            nb = np * __NumberBFrames;
            err = _rc_init_GOP(p_codec, p_quad, p_gen, np, nb);
            if (err) {
                return err;
            }
        }
        break;
    case RC_MODE_0:
    case RC_MODE_2:
        if (p_codec->encoder.rc.idr_period == 0) {
            if ( !(p_codec->encoder.rc.curr_frm_idx) ) {
                /* number of P frames */
                np = (int) (p_codec->encoder.rc.no_frames + __NumberBFrames) / (1 + __NumberBFrames) - 1 + 1; // approximate but good enough (hack...)
                /* number of B frames */
                nb = np * __NumberBFrames;
                err = _rc_init_GOP(p_codec, p_quad, p_gen, np, nb);
                if (err) {
                    return err;
                }
            }
        }
        else if ( /*pc_layer->pc_slice_hdr->IdrFlag*/ p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA ) {
            int M = __NumberBFrames + 1;
            int n = p_codec->encoder.rc.idr_period;

            /* last GOP may contain less frames */
            if ((p_codec->encoder.rc.curr_frm_idx / p_codec->encoder.rc.idr_period) >= (p_codec->encoder.rc.no_frames / p_codec->encoder.rc.idr_period)) {
                n = p_codec->encoder.rc.no_frames - p_codec->encoder.rc.curr_frm_idx;
            }

            /* number of P frames */
            np = (p_codec->encoder.rc.curr_frm_idx == 0) ? 1 + ((n - 2) / M) : (n - 1) / M;
            /* number of B frames */
            nb = n - np - 1;
            err = _rc_init_GOP(p_codec, p_quad, p_gen, np, nb);
            if (err) {
                return err;
            }
        }
        break;
    default:
        break;
    }

    return err;
}

static HL_ERROR_T _rc_init_GOP(hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int np, int nb)
{
    int OverBits, GOPDquant, RCMinQP, RCMaxQP;
    int64_t AllocatedBits;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_layer_t* pc_layer;

    int basicunit;

    pc_sps = p_codec->sps.pc_active;
    pc_layer = p_codec->layers.pc_active;

    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    RCMinQP = p_codec->pc_base->rc_qp_min >= HL_CODEC_264_QP_MIN ? p_codec->pc_base->rc_qp_min : HL_CODEC_264_QP_MIN;
    RCMaxQP = (p_codec->pc_base->rc_qp_max >= HL_CODEC_264_QP_MIN && p_codec->pc_base->rc_qp_max <= HL_CODEC_264_QP_MAX) ? p_codec->pc_base->rc_qp_max : HL_CODEC_264_QP_MAX;


    // bit allocation for RC_MODE_3
    switch( __RCUpdateMode ) {
    case RC_MODE_3: {
            int sum = 0, tmp, level, levels = 0, num_frames[RC_MAX_TEMPORAL_LEVELS];
            float numer, denom;
            int gop = __NumberBFrames + 1;
            int i_period = p_codec->encoder.rc.intra_period == 0 ? p_codec->encoder.rc.idr_period : (p_codec->encoder.rc.idr_period == 0 ? p_codec->encoder.rc.intra_period : HL_MATH_MIN(p_codec->encoder.rc.intra_period, p_codec->encoder.rc.idr_period) );

            memset( num_frames, 0, RC_MAX_TEMPORAL_LEVELS * sizeof(int) );
            // are there any B frames?
            if ( __NumberBFrames ) {
                if ( __HierarchicalCoding == 1 ) { // two layers: even/odd
                    levels = 2;
                    num_frames[0] = __NumberBFrames >> 1;
                    num_frames[1] = (__NumberBFrames - num_frames[0]) >= 0 ? (__NumberBFrames - num_frames[0]) : 0;
                }
                else if ( __HierarchicalCoding == 2 ) { // binary hierarchical structure
                    // check if gop is power of two
                    tmp = gop;
                    while ( tmp ) {
                        sum += tmp & 1;
                        tmp >>= 1;
                    }
                    if( !(sum == 1) ) {
                        HL_DEBUG_ERROR("assert( sum == 1 )");
                        return HL_ERROR_INVALID_OPERATION;
                    }

                    // determine number of levels
                    levels = 0;
                    tmp = gop;
                    while ( tmp > 1 ) {
                        tmp >>= 1; // divide by 2
                        num_frames[levels] = 1 << levels;
                        levels++;
                    }
                    if( !(levels >= 1 && levels <= RC_MAX_TEMPORAL_LEVELS) ) {
                        HL_DEBUG_ERROR("assert( levels >= 1 && levels <= RC_MAX_TEMPORAL_LEVELS )");
                        return HL_ERROR_INVALID_OPERATION;
                    }
                }
                else if ( __HierarchicalCoding == 3 ) {
                    HL_DEBUG_ERROR("RCUpdateMode=3 and HierarchicalCoding == 3 are currently not supported"); // This error message should be moved elsewhere and have proper memory deallocation
                    return HL_ERROR_INVALID_OPERATION;
                }
                else { // all frames of the same priority - level
                    levels = 1;
                    num_frames[0] = __NumberBFrames;
                }
                p_gen->temporal_levels = levels;
            }
            else {
                for ( level = 0; level < RC_MAX_TEMPORAL_LEVELS; level++ ) {
                    __RCBSliceBitRatio[level] = 0.0F;
                }
                p_gen->temporal_levels = 0;
            }
            // calculate allocated bits for each type of frame
            numer = (float)(( (!i_period ? 1 : i_period) * gop) * ((double)p_codec->encoder.rc.bit_rate / p_codec->encoder.rc.frame_rate));
            denom = 0.0F;

            for ( level = 0; level < levels; level++ ) {
                denom += (float)(num_frames[level] * __RCBSliceBitRatio[level]);
                p_gen->hierNb[level] = num_frames[level] * np;
            }
            denom += 1.0F;
            if ( i_period >= 1 ) {
                denom *= (float)i_period;
                denom += (float)__RCISliceBitRatio - 1.0F;
            }

            // set bit targets for each type of frame
            p_gen->RCPSliceBits = (int) HL_MATH_FLOOR( numer / denom + 0.5F );
            p_gen->RCISliceBits = i_period ? (int)(__RCISliceBitRatio * p_gen->RCPSliceBits + 0.5) : 0;

            for ( level = 0; level < levels; level++ ) {
                p_gen->RCBSliceBits[level] = (int)HL_MATH_FLOOR(__RCBSliceBitRatio[level] * p_gen->RCPSliceBits + 0.5);
            }

            p_gen->NISlice = i_period ? ( p_codec->encoder.rc.no_frames / i_period ) : 0;
            p_gen->NPSlice = (p_codec->encoder.rc.no_frames / (1 + __NumberBFrames)) - p_gen->NISlice; // approximate but good enough for sufficient number of frames
        }
        break;
    default:
        break;
    }

    /* check if the last GOP over uses its budget. If yes, the initial QP of the I frame in
    the coming  GOP will be increased.*/

    OverBits=-(int)(p_gen->RemainingBits);

    /*initialize the lower bound and the upper bound for the target bits of each frame, HRD consideration*/
    p_quad->LowerBound  = (int)(p_gen->RemainingBits + p_quad->bit_rate / p_quad->frame_rate);
    p_quad->UpperBound1 = (int)(p_gen->RemainingBits + (p_quad->bit_rate * 2.048));

    /*compute the total number of bits for the current GOP*/
    AllocatedBits = (int64_t) HL_MATH_FLOOR((1 + np + nb) * p_quad->bit_rate / p_quad->frame_rate + 0.5);
    p_gen->RemainingBits += AllocatedBits;
    p_quad->Np = np;
    p_quad->Nb = nb;

    p_quad->GOPOverdue=FALSE;

    /*field coding*/
    //p_gen->NoGranularFieldRC = ( __PicInterlace || !__MbInterlace || p_codec->encoder.rc.basicunit != pc_sps->uPicSizeInMapUnits );
    if ( !__PicInterlace && __MbInterlace && basicunit == pc_sps->uPicSizeInMapUnits ) {
        p_gen->NoGranularFieldRC = 0;
    }
    else {
        p_gen->NoGranularFieldRC = 1;
    }

    /*Compute InitialQp for each GOP*/
    p_quad->TotalPFrame=np;
    p_gen->NumberofGOP++;
    if(p_gen->NumberofGOP==1) {
        p_quad->MyInitialQp = p_codec->encoder.rc.SeinitialQP + p_quad->bitdepth_qp_scale;
        p_quad->CurrLastQP = p_quad->MyInitialQp - 1; //recent change -0;
        p_quad->QPLastGOP   = p_quad->MyInitialQp;

        p_quad->PAveFrameQP   = p_quad->MyInitialQp;
        p_quad->m_Qc          = p_quad->PAveFrameQP;
        p_quad->FieldQPBuffer = p_quad->PAveFrameQP;
        p_quad->FrameQPBuffer = p_quad->PAveFrameQP;
        p_quad->PAverageQp    = p_quad->PAveFrameQP;
    }
    else {
        /*adaptive field/frame coding*/
        if( __PicInterlace == ADAPTIVE_CODING || __MbInterlace ) {
            if (p_gen->FieldFrame == 1) {
                p_quad->TotalQpforPPicture += p_quad->FrameQPBuffer;
                p_quad->QPLastPFrame = p_quad->FrameQPBuffer;
            }
            else {
                p_quad->TotalQpforPPicture += p_quad->FieldQPBuffer;
                p_quad->QPLastPFrame = p_quad->FieldQPBuffer;
            }
        }
        /*compute the average QP of P frames in the previous GOP*/
        p_quad->PAverageQp=(int)(1.0 * p_quad->TotalQpforPPicture / p_quad->NumberofPPicture+0.5);

        GOPDquant=(int)((1.0*(np+nb+1)/15.0) + 0.5);
        if(GOPDquant>2) {
            GOPDquant=2;
        }

        p_quad->PAverageQp -= GOPDquant;

        if (p_quad->PAverageQp > (p_quad->QPLastPFrame - 2)) {
            p_quad->PAverageQp--;
        }

        // QP is constrained by QP of previous GOP
        p_quad->PAverageQp = HL_MATH_CLIP3(p_quad->QPLastGOP - 2, p_quad->QPLastGOP + 2, p_quad->PAverageQp);
        // Also clipped within range.
        p_quad->PAverageQp = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale,  p_quad->PAverageQp);


        p_quad->MyInitialQp = p_quad->PAverageQp;
        p_quad->Pm_Qp       = p_quad->PAverageQp;
        p_quad->PAveFrameQP = p_quad->PAverageQp;
        p_quad->QPLastGOP   = p_quad->MyInitialQp;
        p_quad->PrevLastQP = p_quad->CurrLastQP;
        p_quad->CurrLastQP = p_quad->MyInitialQp - 1;
    }

    p_quad->TotalQpforPPicture=0;
    p_quad->NumberofPPicture=0;
    p_quad->NumberofBFrames=0;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T _rc_init_pict(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, RCQuadratic *p_quad, RCGeneric *p_gen, int fieldpic,int topfield,int targetcomputation, float mult)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int tmp_T, basicunit;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_layer_t* pc_layer;

    pc_sps = p_codec->sps.pc_active;
    pc_layer = p_codec->layers.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    /* compute the total number of basic units in a frame */
    if(__MbInterlace) {
        p_quad->TotalNumberofBasicUnit = pc_sps->uPicSizeInMapUnits / p_codec->encoder.rc.BasicUnit;
    }
    else {
        p_quad->TotalNumberofBasicUnit = pc_sps->uPicSizeInMapUnits / basicunit;
    }

    p_codec->encoder.rc.NumberofCodedMacroBlocks = 0;

    /* Normally, the bandwidth for the VBR case is estimated by
       a congestion control algorithm. A bandwidth curve can be predefined if we only want to
       test the proposed algorithm */
    if (__channel_type == 1) {
        if (p_quad->NumberofCodedPFrame == 58) {
            p_quad->bit_rate *= 1.5;
        }
        else if (p_quad->NumberofCodedPFrame == 59) {
            p_quad->PrevBitRate = p_quad->bit_rate;
        }
    }

    /* predefine a target buffer level for each frame */
    if ((fieldpic||topfield) && targetcomputation) {
        if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number !=0)) ) {
            /* Since the available bandwidth may vary at any time, the total number of
            bits is updated picture by picture*/
            if (p_quad->PrevBitRate != p_quad->bit_rate) {
                p_gen->RemainingBits +=(int) HL_MATH_FLOOR((p_quad->bit_rate-p_quad->PrevBitRate)*(p_quad->Np + p_quad->Nb)/p_quad->frame_rate+0.5);
            }

            /* predefine the  target buffer level for each picture.
            frame layer rate control */
            if (p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
                if (p_quad->NumberofPPicture == 1) {
                    p_quad->TargetBufferLevel = (double) p_gen->CurrentBufferFullness;
                    p_quad->DeltaP = (p_gen->CurrentBufferFullness - p_quad->GOPTargetBufferLevel) / (p_quad->TotalPFrame-1);
                    p_quad->TargetBufferLevel -= p_quad->DeltaP;
                }
                else if (p_quad->NumberofPPicture>1) {
                    p_quad->TargetBufferLevel -= p_quad->DeltaP;
                }
            }
            /* basic unit layer rate control */
            else {
                if (p_quad->NumberofCodedPFrame>0) {
                    /* adaptive frame/field coding */
                    if (((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace))&&(p_gen->FieldControl==1)) {
                        memcpy((void *)p_quad->FCBUPFMAD,(void *)p_quad->FCBUCFMAD, p_quad->TotalNumberofBasicUnit * sizeof(double));
                    }
                    else {
                        memcpy((void *)p_quad->BUPFMAD,(void *)p_quad->BUCFMAD, p_quad->TotalNumberofBasicUnit * sizeof(double));
                    }
                }

                if (p_gen->NumberofGOP==1) {
                    if (p_quad->NumberofPPicture==1) {
                        p_quad->TargetBufferLevel = (double) p_gen->CurrentBufferFullness;
                        p_quad->DeltaP = (p_gen->CurrentBufferFullness - p_quad->GOPTargetBufferLevel)/(p_quad->TotalPFrame - 1);
                        p_quad->TargetBufferLevel -= p_quad->DeltaP;
                    }
                    else if (p_quad->NumberofPPicture>1) {
                        p_quad->TargetBufferLevel -= p_quad->DeltaP;
                    }
                }
                else if (p_gen->NumberofGOP>1) {
                    if (p_quad->NumberofPPicture==0) {
                        p_quad->TargetBufferLevel = (double) p_gen->CurrentBufferFullness;
                        p_quad->DeltaP = (p_gen->CurrentBufferFullness - p_quad->GOPTargetBufferLevel) / p_quad->TotalPFrame;
                        p_quad->TargetBufferLevel -= p_quad->DeltaP;
                    }
                    else if (p_quad->NumberofPPicture>0) {
                        p_quad->TargetBufferLevel -= p_quad->DeltaP;
                    }
                }
            }

            if (p_quad->NumberofCodedPFrame==1) {
                p_quad->AveWp = p_quad->Wp;
            }

            if ((p_quad->NumberofCodedPFrame<8)&&(p_quad->NumberofCodedPFrame>1)) {
                p_quad->AveWp = (p_quad->AveWp + p_quad->Wp * (p_quad->NumberofCodedPFrame-1))/p_quad->NumberofCodedPFrame;
            }
            else if (p_quad->NumberofCodedPFrame>1) {
                p_quad->AveWp = (p_quad->Wp + 7 * p_quad->AveWp) / 8;
            }

            // compute the average complexity of B frames
            if (__NumberBFrames>0) {
                // compute the target buffer level
                p_quad->TargetBufferLevel += (p_quad->AveWp * (__NumberBFrames + 1)*p_quad->bit_rate\
                                              /(p_quad->frame_rate*(p_quad->AveWp+p_quad->AveWb*__NumberBFrames))-p_quad->bit_rate/p_quad->frame_rate);
            }
        }
        else if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B) ) {
            /* update the total number of bits if the bandwidth is changed*/
            if (p_quad->PrevBitRate != p_quad->bit_rate) {
                p_gen->RemainingBits +=(int) HL_MATH_FLOOR((p_quad->bit_rate-p_quad->PrevBitRate) * (p_quad->Np + p_quad->Nb) / p_quad->frame_rate+0.5);
            }
            if (p_gen->NumberofCodedBFrame == 1) {
                if(p_quad->NumberofCodedPFrame == 1) {
                    p_quad->AveWp = p_quad->Wp;
                }
                p_quad->AveWb = p_quad->Wb;
            }
            else if (p_gen->NumberofCodedBFrame > 1) {
                //compute the average weight
                if (p_gen->NumberofCodedBFrame<8) {
                    p_quad->AveWb = (p_quad->AveWb + p_quad->Wb*(p_gen->NumberofCodedBFrame-1)) / p_gen->NumberofCodedBFrame;
                }
                else {
                    p_quad->AveWb = (p_quad->Wb + 7 * p_quad->AveWb) / 8;
                }
            }
        }
        /* Compute the target bit for each frame */
        if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || ( (p_codec->encoder.rc.number != 0) && (__RCUpdateMode == RC_MODE_1 || __RCUpdateMode == RC_MODE_3 ) ) ) {
            /* frame layer rate control */
            if (p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits || (__RCUpdateMode == RC_MODE_3) ) {
                if (p_quad->NumberofCodedPFrame>0) {
                    if (__RCUpdateMode == RC_MODE_3) {
                        int level_idx = ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B) && __HierarchicalCoding) ? (__p_curr_frm_struct_layer - 1) : 0;
                        int bitrate = ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B)) ? p_gen->RCBSliceBits[ level_idx ]
                                      : ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) ? p_gen->RCPSliceBits : p_gen->RCISliceBits );
                        int level, denom = p_gen->NISlice * p_gen->RCISliceBits + p_gen->NPSlice * p_gen->RCPSliceBits;

                        if ( __HierarchicalCoding ) {
                            for ( level = 0; level < p_gen->temporal_levels; level++ ) {
                                denom += p_gen->hierNb[ level ] * p_gen->RCBSliceBits[ level ];
                            }
                        }
                        else {
                            denom += p_gen->hierNb[0] * p_gen->RCBSliceBits[0];
                        }
                        // target due to remaining bits
                        p_quad->Target = (int) HL_MATH_FLOOR( (float)(1.0 * bitrate * p_gen->RemainingBits) / (float)denom + 0.5F );
                        // target given original taget rate and buffer considerations
                        tmp_T  = HL_MATH_MAX(0, (int) HL_MATH_FLOOR( (double)bitrate - p_quad->GAMMAP * (p_gen->CurrentBufferFullness-p_quad->TargetBufferLevel) + 0.5) );
                        // translate Target rate from B or I "domain" to P domain since the P RC model is going to be used to select the QP
                        // for hierarchical coding adjust the target QP to account for different temporal levels
                        switch( pc_layer->pc_slice_hdr->SliceTypeModulo5 ) {
                        case HL_CODEC_264_SLICE_TYPE_B:
                            p_quad->Target = (int) HL_MATH_FLOOR( (float)p_quad->Target / __RCBoverPRatio + 0.5F);
                            break;
                        case HL_CODEC_264_SLICE_TYPE_I:
                            p_quad->Target = (int) HL_MATH_FLOOR( (float)p_quad->Target / (__RCIoverPRatio * 4.0) + 0.5F); // 4x accounts for the fact that header bits reduce the percentage of texture
                            break;
                        case HL_CODEC_264_SLICE_TYPE_P:
                        default:
                            break;
                        }
                    }
                    else {
                        p_quad->Target = (int) HL_MATH_FLOOR( p_quad->Wp * p_gen->RemainingBits / (p_quad->Np * p_quad->Wp + p_quad->Nb * p_quad->Wb) + 0.5);
                        tmp_T  = HL_MATH_MAX(0, (int) HL_MATH_FLOOR(p_quad->bit_rate / p_quad->frame_rate - p_quad->GAMMAP * (p_gen->CurrentBufferFullness-p_quad->TargetBufferLevel) + 0.5));
                        p_quad->Target = (int) HL_MATH_FLOOR(p_quad->BETAP * (p_quad->Target - tmp_T) + tmp_T + 0.5);
                    }
                }
            }
            /* basic unit layer rate control */
            else {
                if (((p_gen->NumberofGOP == 1)&&(p_quad->NumberofCodedPFrame>0))
                        || (p_gen->NumberofGOP > 1)) {
                    p_quad->Target = (int) (HL_MATH_FLOOR( p_quad->Wp * p_gen->RemainingBits / (p_quad->Np * p_quad->Wp + p_quad->Nb * p_quad->Wb) + 0.5));
                    tmp_T  = HL_MATH_MAX(0, (int) (HL_MATH_FLOOR(p_quad->bit_rate / p_quad->frame_rate - p_quad->GAMMAP * (p_gen->CurrentBufferFullness-p_quad->TargetBufferLevel) + 0.5)));
                    p_quad->Target = (int) (HL_MATH_FLOOR(p_quad->BETAP * (p_quad->Target - tmp_T) + tmp_T + 0.5));
                }
            }
            p_quad->Target = (int)(mult * p_quad->Target);

            /* HRD consideration */
            if ( __RCUpdateMode != RC_MODE_3 || (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) ) {
                p_quad->Target = HL_MATH_CLIP3(p_quad->LowerBound, p_quad->UpperBound2, p_quad->Target);
            }
            if ((topfield) || (fieldpic && ((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)))) {
                p_quad->TargetField=p_quad->Target;
            }
        }
    }

    if (fieldpic || topfield) {
        /* frame layer rate control */
        p_gen->NumberofHeaderBits  = 0;
        p_gen->NumberofTextureBits = 0;

        /* basic unit layer rate control */
        if (p_codec->encoder.rc.BasicUnit < (int)pc_sps->uPicSizeInMapUnits) {
            p_quad->TotalFrameQP = 0;
            p_gen->NumberofBasicUnitHeaderBits  = 0;
            p_gen->NumberofBasicUnitTextureBits = 0;
            p_gen->TotalMADBasicUnit = 0;
            if (p_gen->FieldControl==0) {
                p_quad->NumberofBasicUnit = p_quad->TotalNumberofBasicUnit;
            }
            else {
                p_quad->NumberofBasicUnit = p_quad->TotalNumberofBasicUnit >> 1;
            }
        }
    }

    if( ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) && p_codec->encoder.rc.BasicUnit < (int)pc_sps->uPicSizeInMapUnits && p_gen->FieldControl == 1 ) {
        /* top field at basic unit layer rate control */
        if (topfield) {
            p_quad->bits_topfield=0;
            p_quad->Target=(int)(p_quad->TargetField*0.6);
        }
        /* bottom field at basic unit layer rate control */
        else {
            p_quad->Target=p_quad->TargetField-p_quad->bits_topfield;
            p_gen->NumberofBasicUnitHeaderBits=0;
            p_gen->NumberofBasicUnitTextureBits=0;
            p_gen->TotalMADBasicUnit=0;
            p_quad->NumberofBasicUnit=p_quad->TotalNumberofBasicUnit >> 1;
        }
    }

    return err;
}

static HL_ERROR_T _rc_init_frame(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5)
{
    const hl_codec_264_nal_sps_t* pc_sps;

    int basicunit;

    pc_sps = p_codec->sps.pc_active;

    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    switch( __RCUpdateMode ) {
    case RC_MODE_0:
    case RC_MODE_1:
    case RC_MODE_2:
    case RC_MODE_3:

        // update the number of MBs in the basic unit for MBAFF coding
        if( (__MbInterlace) && (basicunit < (int)pc_sps->uPicSizeInMapUnits) && ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && p_codec->encoder.rc.number) ) ) {
            p_codec->encoder.rc.BasicUnit = basicunit << 1;
        }
        else {
            p_codec->encoder.rc.BasicUnit = basicunit;
        }

        if ( p_codec->encoder.rc.RDPictureDecision ) {
            _rc_copy_quadratic( p_codec, p_codec->encoder.rc.p_rc_quad_init, p_codec->encoder.rc.p_rc_quad ); // store rate allocation quadratic...
            _rc_copy_generic( p_codec, p_codec->encoder.rc.p_rc_gen_init, p_codec->encoder.rc.p_rc_gen ); // ...and generic model
        }
        _rc_init_pict(p_codec, SliceTypeModulo5, p_codec->encoder.rc.p_rc_quad, p_codec->encoder.rc.p_rc_gen, 1, 0, 1, 1.0F);

        if( pc_sps->frame_mbs_only_flag) {
            p_codec->encoder.rc.p_rc_gen->TopFieldFlag=0;
        }

        p_codec->encoder.rc.qp = p_codec->encoder.rc.updateQP(p_codec, SliceTypeModulo5, p_codec->encoder.rc.p_rc_quad, p_codec->encoder.rc.p_rc_gen, 0) - p_codec->encoder.rc.p_rc_quad->bitdepth_qp_scale;
        break;
    default:
        break;
    }

    return HL_ERROR_SUCCESS;
}

static int _updateQPRC0(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield)
{
    int m_Bits, RCMinQP, RCMaxQP, BFrameNumber, StepSize, SumofBasicUnit, MaxQpChange, m_Qp, m_Hp;
    const hl_codec_264_nal_sps_t* pc_sps;

    int basicunit;

    pc_sps = p_codec->sps.pc_active;

    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;
    RCMinQP = p_codec->pc_base->rc_qp_min >= HL_CODEC_264_QP_MIN ? p_codec->pc_base->rc_qp_min : HL_CODEC_264_QP_MIN;
    RCMaxQP = (p_codec->pc_base->rc_qp_max >= HL_CODEC_264_QP_MIN && p_codec->pc_base->rc_qp_max <= HL_CODEC_264_QP_MAX) ? p_codec->pc_base->rc_qp_max : HL_CODEC_264_QP_MAX;

    /* frame layer rate control */
    if ( p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits ) {
        /* fixed quantization parameter is used to coded I frame, the first P frame and the first B frame
        the quantization parameter is adjusted according the available channel bandwidth and
        the type of video */
        /*top field*/
        if ((topfield) || (p_gen->FieldControl==0)) {
            if ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_I)) {
                p_quad->m_Qc = p_quad->MyInitialQp;
                return p_quad->m_Qc;
            }
            else if ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B)) {
                if (__NumberBFrames==1) {
                    if ((__PicInterlace==ADAPTIVE_CODING) || (__MbInterlace)) {
                        _updateQPInterlace( p_quad, p_gen );
                    }

                    p_quad->m_Qc = HL_MATH_MIN(p_quad->PrevLastQP, p_quad->CurrLastQP) + 2;
                    p_quad->m_Qc = HL_MATH_MAX(p_quad->m_Qc, HL_MATH_MAX(p_quad->PrevLastQP, p_quad->CurrLastQP));
                    p_quad->m_Qc = HL_MATH_MAX(p_quad->m_Qc, p_quad->CurrLastQP + 1);
                    p_quad->m_Qc = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // Clipping
                }
                else {
                    BFrameNumber = __NumberBFrames ? (p_quad->NumberofBFrames + 1) % __NumberBFrames : p_quad->NumberofBFrames;
                    if (BFrameNumber == 0) {
                        BFrameNumber = __NumberBFrames;
                    }

                    /*adaptive field/frame coding*/
                    if (BFrameNumber==1) {
                        if ((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
                            _updateQPInterlace( p_quad, p_gen );
                        }
                    }

                    if ((p_quad->CurrLastQP-p_quad->PrevLastQP)<=(-2*__NumberBFrames-3)) {
                        StepSize=-3;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames-2)) {
                        StepSize=-2;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames-1)) {
                        StepSize=-1;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames)) {
                        StepSize=0;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames+1)) {
                        StepSize=1;
                    }
                    else {
                        StepSize=2;
                    }

                    p_quad->m_Qc  = p_quad->PrevLastQP + StepSize;
                    p_quad->m_Qc += HL_MATH_CLIP3( -2 * (BFrameNumber - 1), 2*(BFrameNumber-1),
                                                   (BFrameNumber-1)*(p_quad->CurrLastQP-p_quad->PrevLastQP)/(__NumberBFrames-1));
                    p_quad->m_Qc  = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // Clipping
                }
                return p_quad->m_Qc;
            }
            else if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) && p_quad->NumberofPPicture == 0 ) {
                p_quad->m_Qc=p_quad->MyInitialQp;

                if (p_gen->FieldControl==0) {
                    _updateQPNonPicAFF( pc_sps, p_quad );
                }
                return p_quad->m_Qc;
            }
            else {
                /*adaptive field/frame coding*/
                if ( ( __PicInterlace == ADAPTIVE_CODING || __MbInterlace ) && p_gen->FieldControl == 0 ) {
                    _updateQPInterlaceBU( p_quad, p_gen );
                }

                p_quad->m_X1 = p_quad->Pm_X1;
                p_quad->m_X2 = p_quad->Pm_X2;
                p_quad->MADPictureC1 = p_quad->PMADPictureC1;
                p_quad->MADPictureC2 = p_quad->PMADPictureC2;
                p_quad->PreviousPictureMAD = p_quad->PPictureMAD[0];

                MaxQpChange = p_quad->PMaxQpChange;
                m_Qp = p_quad->Pm_Qp;
                m_Hp = p_quad->PPreHeader;

                /* predict the MAD of current picture*/
                p_quad->CurrentFrameMAD = p_quad->MADPictureC1*p_quad->PreviousPictureMAD + p_quad->MADPictureC2;

                /*compute the number of bits for the texture*/
                if (p_quad->Target < 0) {
                    p_quad->m_Qc=m_Qp+MaxQpChange;
                    p_quad->m_Qc = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // Clipping
                }
                else {
                    m_Bits = p_quad->Target-m_Hp;
                    m_Bits = HL_MATH_MAX(m_Bits, (int)(p_quad->bit_rate/(MINVALUE*p_quad->frame_rate)));

                    _updateModelQPFrame( p_quad, m_Bits );

                    p_quad->m_Qc = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // clipping
                    p_quad->m_Qc = HL_MATH_CLIP3(m_Qp-MaxQpChange, m_Qp+MaxQpChange, p_quad->m_Qc); // control variation
                }

                if ( p_gen->FieldControl == 0 ) {
                    _updateQPNonPicAFF( pc_sps, p_quad );
                }

                return p_quad->m_Qc;
            }
        }
        /*bottom field*/
        else {
            if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) && p_gen->NoGranularFieldRC == 0 ) {
                _updateBottomField( p_codec, p_quad );
            }
            return p_quad->m_Qc;
        }
    }
    /*basic unit layer rate control*/
    else {
        /*top field of I frame*/
        if ((SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_I)) {
            p_quad->m_Qc = p_quad->MyInitialQp;
            return p_quad->m_Qc;
        }
        else if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B) ) {
            /*top field of B frame*/
            if ((topfield)||(p_gen->FieldControl==0)) {
                if (__NumberBFrames==1) {
                    /*adaptive field/frame coding*/
                    if ((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
                        _updateQPInterlace( p_quad, p_gen );
                    }

                    if (p_quad->PrevLastQP==p_quad->CurrLastQP) {
                        p_quad->m_Qc=p_quad->PrevLastQP+2;
                    }
                    else {
                        p_quad->m_Qc = ((p_quad->PrevLastQP+p_quad->CurrLastQP) >> 1) + 1;
                    }
                    p_quad->m_Qc = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // Clipping
                }
                else {
                    BFrameNumber = __NumberBFrames ? (p_quad->NumberofBFrames + 1) % __NumberBFrames : p_quad->NumberofBFrames;
                    if (BFrameNumber == 0) {
                        BFrameNumber=__NumberBFrames;
                    }

                    /*adaptive field/frame coding*/
                    if (BFrameNumber==1) {
                        if((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
                            _updateQPInterlace( p_quad, p_gen );
                        }
                    }

                    if ((p_quad->CurrLastQP-p_quad->PrevLastQP)<=(-2*__NumberBFrames-3)) {
                        StepSize=-3;
                    }
                    else  if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames-2)) {
                        StepSize=-2;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames-1)) {
                        StepSize=-1;
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames)) {
                        StepSize=0;    //0
                    }
                    else if ((p_quad->CurrLastQP-p_quad->PrevLastQP)==(-2*__NumberBFrames+1)) {
                        StepSize=1;    //1
                    }
                    else {
                        StepSize=2;    //2
                    }
                    p_quad->m_Qc=p_quad->PrevLastQP+StepSize;
                    p_quad->m_Qc +=
                        HL_MATH_CLIP3( -2*(BFrameNumber-1), 2*(BFrameNumber-1), (BFrameNumber-1)*(p_quad->CurrLastQP-p_quad->PrevLastQP)/(__NumberBFrames-1) );
                    p_quad->m_Qc = HL_MATH_CLIP3(RCMinQP + p_quad->bitdepth_qp_scale, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // Clipping
                }
                return p_quad->m_Qc;
            }
            /*bottom field of B frame*/
            else {
                return p_quad->m_Qc;
            }
        }
        else if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) ) {
            if ( (p_gen->NumberofGOP == 1) && (p_quad->NumberofPPicture == 0) ) {
                if((p_gen->FieldControl==0)||((p_gen->FieldControl==1) && (p_gen->NoGranularFieldRC==0))) {
                    return _updateFirstP( p_codec, p_quad, p_gen, topfield );
                }
            }
            else {
                p_quad->m_X1=p_quad->Pm_X1;
                p_quad->m_X2=p_quad->Pm_X2;
                p_quad->MADPictureC1=p_quad->PMADPictureC1;
                p_quad->MADPictureC2=p_quad->PMADPictureC2;

                m_Qp=p_quad->Pm_Qp;

                if (p_gen->FieldControl==0) {
                    SumofBasicUnit=p_quad->TotalNumberofBasicUnit;
                }
                else {
                    SumofBasicUnit=p_quad->TotalNumberofBasicUnit>>1;
                }

                /*the average QP of the previous frame is used to coded the first basic unit of the current frame or field*/
                if (p_quad->NumberofBasicUnit==SumofBasicUnit) {
                    return _updateFirstBU( p_codec, p_quad, p_gen, topfield );
                }
                else {
                    /*compute the number of remaining bits*/
                    p_quad->Target -= (p_gen->NumberofBasicUnitHeaderBits + p_gen->NumberofBasicUnitTextureBits);
                    p_gen->NumberofBasicUnitHeaderBits  = 0;
                    p_gen->NumberofBasicUnitTextureBits = 0;
                    if (p_quad->Target<0) {
                        return _updateNegativeTarget( p_codec, p_quad, p_gen, topfield, m_Qp );
                    }
                    else {
                        /*predict the MAD of current picture*/
                        _predictCurrPicMAD( p_codec, p_quad, p_gen );

                        /*compute the total number of bits for the current basic unit*/
                        _updateModelQPBU( p_codec, p_quad, m_Qp );

                        p_quad->TotalFrameQP +=p_quad->m_Qc;
                        p_quad->Pm_Qp=p_quad->m_Qc;
                        p_quad->NumberofBasicUnit--;
                        if ( p_quad->NumberofBasicUnit == 0 && (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) ) {
                            _updateLastBU( p_codec, p_quad, p_gen, topfield );
                        }

                        return p_quad->m_Qc;
                    }
                }
            }
        }
    }
    return p_quad->m_Qc;
}

static void _updateQPInterlace( RCQuadratic *p_quad, RCGeneric *p_gen )
{
    if(p_gen->FieldControl==0) {
        /*previous choice is frame coding*/
        if(p_gen->FieldFrame==1) {
            p_quad->PrevLastQP=p_quad->CurrLastQP;
            p_quad->CurrLastQP=p_quad->FrameQPBuffer;
        }
        /*previous choice is field coding*/
        else {
            p_quad->PrevLastQP=p_quad->CurrLastQP;
            p_quad->CurrLastQP=p_quad->FieldQPBuffer;
        }
    }
}

static void _updateQPNonPicAFF( const hl_codec_264_nal_sps_t *active_sps, RCQuadratic *p_quad )
{
    if(active_sps->frame_mbs_only_flag) {
        p_quad->TotalQpforPPicture +=p_quad->m_Qc;
        p_quad->PrevLastQP=p_quad->CurrLastQP;
        p_quad->CurrLastQP=p_quad->m_Qc;
        p_quad->Pm_Qp=p_quad->m_Qc;
    }
    /*adaptive field/frame coding*/
    else {
        p_quad->FrameQPBuffer=p_quad->m_Qc;
    }
}

static void _updateQPInterlaceBU( RCQuadratic *p_quad, RCGeneric *p_gen )
{
    /*previous choice is frame coding*/
    if(p_gen->FieldFrame==1) {
        p_quad->TotalQpforPPicture +=p_quad->FrameQPBuffer;
        p_quad->Pm_Qp=p_quad->FrameQPBuffer;
    }
    /*previous choice is field coding*/
    else {
        p_quad->TotalQpforPPicture +=p_quad->FieldQPBuffer;
        p_quad->Pm_Qp=p_quad->FieldQPBuffer;
    }
}
static void _updateModelQPFrame( RCQuadratic *p_quad, int m_Bits )
{
    double dtmp, m_Qstep;

    dtmp = p_quad->CurrentFrameMAD * p_quad->m_X1 * p_quad->CurrentFrameMAD * p_quad->m_X1
           + 4 * p_quad->m_X2 * p_quad->CurrentFrameMAD * m_Bits;
    if ((p_quad->m_X2 == 0.0) || (dtmp < 0) || ((sqrt (dtmp) - p_quad->m_X1 * p_quad->CurrentFrameMAD) <= 0.0)) { // fall back 1st order mode
        m_Qstep = (float) (p_quad->m_X1 * p_quad->CurrentFrameMAD / (double) m_Bits);
    }
    else { // 2nd order mode
        m_Qstep = (float) ((2 * p_quad->m_X2 * p_quad->CurrentFrameMAD) / (sqrt (dtmp) - p_quad->m_X1 * p_quad->CurrentFrameMAD));
    }

    p_quad->m_Qc = _Qstep2QP(m_Qstep, p_quad->bitdepth_qp_scale);
}

static void _updateBottomField( hl_codec_264_t *p_codec, RCQuadratic *p_quad )
{
    /*field coding*/
    if(__PicInterlace==FIELD_CODING) {
        p_quad->TotalQpforPPicture +=p_quad->m_Qc;
        p_quad->PrevLastQP=p_quad->CurrLastQP+1;
        p_quad->CurrLastQP=p_quad->m_Qc;//+0 Recent change 13/1/2003
        p_quad->Pm_Qp=p_quad->m_Qc;
    }
    /*adaptive field/frame coding*/
    else {
        p_quad->FieldQPBuffer=p_quad->m_Qc;
    }
}

static int _updateFirstP( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield )
{
    /*top field of the first P frame*/
    p_quad->m_Qc=p_quad->MyInitialQp;
    p_gen->NumberofBasicUnitHeaderBits=0;
    p_gen->NumberofBasicUnitTextureBits=0;
    p_quad->NumberofBasicUnit--;

    /*bottom field of the first P frame*/
    if ((!topfield)&&(p_quad->NumberofBasicUnit==0)) {
        /*frame coding or field coding*/
        if ((p_codec->sps.pc_active->frame_mbs_only_flag)||(__PicInterlace==FIELD_CODING)) {
            p_quad->TotalQpforPPicture +=p_quad->m_Qc;
            p_quad->PrevLastQP=p_quad->CurrLastQP;
            p_quad->CurrLastQP=p_quad->m_Qc;
            p_quad->PAveFrameQP=p_quad->m_Qc;
            p_quad->PAveHeaderBits3=p_quad->PAveHeaderBits2;
        }
        /*adaptive frame/field coding*/
        else if ((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
            if (p_gen->FieldControl==0) {
                p_quad->FrameQPBuffer=p_quad->m_Qc;
                p_quad->FrameAveHeaderBits=p_quad->PAveHeaderBits2;
            }
            else {
                p_quad->FieldQPBuffer=p_quad->m_Qc;
                p_quad->FieldAveHeaderBits=p_quad->PAveHeaderBits2;
            }
        }
    }
    p_quad->Pm_Qp=p_quad->m_Qc;
    p_quad->TotalFrameQP +=p_quad->m_Qc;
    return p_quad->m_Qc;
}

static int _updateFirstBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield )
{
    /*adaptive field/frame coding*/
    if(((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace))&&(p_gen->FieldControl==0)) {
        /*previous choice is frame coding*/
        if(p_gen->FieldFrame==1) {
            if(p_quad->NumberofPPicture>0) {
                p_quad->TotalQpforPPicture +=p_quad->FrameQPBuffer;
            }
            p_quad->PAveFrameQP=p_quad->FrameQPBuffer;
            p_quad->PAveHeaderBits3=p_quad->FrameAveHeaderBits;
        }
        /*previous choice is field coding*/
        else {
            if(p_quad->NumberofPPicture>0) {
                p_quad->TotalQpforPPicture +=p_quad->FieldQPBuffer;
            }
            p_quad->PAveFrameQP=p_quad->FieldQPBuffer;
            p_quad->PAveHeaderBits3=p_quad->FieldAveHeaderBits;
        }
    }

    if(p_quad->Target<=0) {
        p_quad->m_Qc = p_quad->PAveFrameQP + 2;
        if (p_codec->pc_base->rc_qp_max >= 0) {
            if(p_quad->m_Qc > (p_codec->pc_base->rc_qp_max + p_quad->bitdepth_qp_scale)) {
                p_quad->m_Qc = p_codec->pc_base->rc_qp_max + p_quad->bitdepth_qp_scale;
            }
        }

        if (topfield||(p_gen->FieldControl==0)) {
            p_quad->GOPOverdue=TRUE;
        }
    }
    else {
        p_quad->m_Qc=p_quad->PAveFrameQP;
    }
    p_quad->TotalFrameQP +=p_quad->m_Qc;
    p_quad->NumberofBasicUnit--;
    p_quad->Pm_Qp = p_quad->PAveFrameQP;

    return p_quad->m_Qc;
}

static int _updateNegativeTarget( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield, int m_Qp )
{
    int PAverageQP, basicunit;
    int32_t RCMaxQP;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    if(p_quad->GOPOverdue==TRUE) {
        p_quad->m_Qc=m_Qp+2;
    }
    else {
        p_quad->m_Qc=m_Qp+p_quad->DDquant;    //2
    }

    RCMaxQP = (p_codec->pc_base->rc_qp_max >= HL_CODEC_264_QP_MIN && p_codec->pc_base->rc_qp_max <= HL_CODEC_264_QP_MAX) ? p_codec->pc_base->rc_qp_max : HL_CODEC_264_QP_MAX;

    p_quad->m_Qc = HL_MATH_MIN(p_quad->m_Qc, RCMaxQP + p_quad->bitdepth_qp_scale);  // clipping
    if(basicunit>=(int)p_quad->MBPerRow) {
        p_quad->m_Qc = HL_MATH_MIN(p_quad->m_Qc, p_quad->PAveFrameQP + 6);
    }
    else {
        p_quad->m_Qc = HL_MATH_MIN(p_quad->m_Qc, p_quad->PAveFrameQP + 3);
    }

    p_quad->TotalFrameQP +=p_quad->m_Qc;
    p_quad->NumberofBasicUnit--;
    if(p_quad->NumberofBasicUnit==0) {
        if((!topfield)||(p_gen->FieldControl==0)) {
            /*frame coding or field coding*/
            if((pc_sps->frame_mbs_only_flag)||(__PicInterlace==FIELD_CODING)) {
                PAverageQP=(int)((double)p_quad->TotalFrameQP/(double)p_quad->TotalNumberofBasicUnit+0.5);
                if (p_quad->NumberofPPicture == (p_codec->encoder.rc.intra_period - 2)) {
                    p_quad->QPLastPFrame = PAverageQP;
                }

                p_quad->TotalQpforPPicture +=PAverageQP;
                if(p_quad->GOPOverdue==TRUE) {
                    p_quad->PrevLastQP=p_quad->CurrLastQP+1;
                    p_quad->CurrLastQP=PAverageQP;
                }
                else {
                    if((p_quad->NumberofPPicture==0)&&(p_gen->NumberofGOP>1)) {
                        p_quad->PrevLastQP=p_quad->CurrLastQP;
                        p_quad->CurrLastQP=PAverageQP;
                    }
                    else if(p_quad->NumberofPPicture>0) {
                        p_quad->PrevLastQP=p_quad->CurrLastQP+1;
                        p_quad->CurrLastQP=PAverageQP;
                    }
                }
                p_quad->PAveFrameQP=PAverageQP;
                p_quad->PAveHeaderBits3=p_quad->PAveHeaderBits2;
            }
            /*adaptive field/frame coding*/
            else if((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
                if(p_gen->FieldControl==0) {
                    PAverageQP=(int)((double)p_quad->TotalFrameQP/(double)p_quad->TotalNumberofBasicUnit+0.5);
                    p_quad->FrameQPBuffer=PAverageQP;
                    p_quad->FrameAveHeaderBits=p_quad->PAveHeaderBits2;
                }
                else {
                    PAverageQP=(int)((double)p_quad->TotalFrameQP/(double)p_quad->TotalNumberofBasicUnit+0.5);
                    p_quad->FieldQPBuffer=PAverageQP;
                    p_quad->FieldAveHeaderBits=p_quad->PAveHeaderBits2;
                }
            }
        }
    }
    if(p_quad->GOPOverdue==TRUE) {
        p_quad->Pm_Qp=p_quad->PAveFrameQP;
    }
    else {
        p_quad->Pm_Qp=p_quad->m_Qc;
    }

    return p_quad->m_Qc;
}

static void _updateLastBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, int topfield )
{
    int PAverageQP;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;

    if((!topfield)||(p_gen->FieldControl==0)) {
        /*frame coding or field coding*/
        if((pc_sps->frame_mbs_only_flag)||(__PicInterlace==FIELD_CODING)) {
            PAverageQP=(int)((double)p_quad->TotalFrameQP/(double) p_quad->TotalNumberofBasicUnit+0.5);
            if (p_quad->NumberofPPicture == (p_codec->encoder.rc.intra_period - 2)) {
                p_quad->QPLastPFrame = PAverageQP;
            }

            p_quad->TotalQpforPPicture +=PAverageQP;
            p_quad->PrevLastQP=p_quad->CurrLastQP;
            p_quad->CurrLastQP=PAverageQP;
            p_quad->PAveFrameQP=PAverageQP;
            p_quad->PAveHeaderBits3=p_quad->PAveHeaderBits2;
        }
        else if((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) {
            if(p_gen->FieldControl==0) {
                PAverageQP=(int)((double) p_quad->TotalFrameQP/(double)p_quad->TotalNumberofBasicUnit+0.5);
                p_quad->FrameQPBuffer=PAverageQP;
                p_quad->FrameAveHeaderBits=p_quad->PAveHeaderBits2;
            }
            else {
                PAverageQP=(int)((double) p_quad->TotalFrameQP/(double) p_quad->TotalNumberofBasicUnit+0.5);
                p_quad->FieldQPBuffer=PAverageQP;
                p_quad->FieldAveHeaderBits=p_quad->PAveHeaderBits2;
            }
        }
    }
}

static void _updateModelQPBU( hl_codec_264_t* p_codec, RCQuadratic *p_quad, int m_Qp )
{
    double dtmp, m_Qstep;
    int m_Bits, RCMinQP, RCMaxQP, basicunit;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    RCMinQP = p_codec->pc_base->rc_qp_min >= HL_CODEC_264_QP_MIN ? p_codec->pc_base->rc_qp_min : HL_CODEC_264_QP_MIN;
    RCMaxQP = (p_codec->pc_base->rc_qp_max >= HL_CODEC_264_QP_MIN && p_codec->pc_base->rc_qp_max <= HL_CODEC_264_QP_MAX) ? p_codec->pc_base->rc_qp_max : HL_CODEC_264_QP_MAX;

    /*compute the total number of bits for the current basic unit*/
    m_Bits =(int)(p_quad->Target * p_quad->CurrentFrameMAD * p_quad->CurrentFrameMAD / p_quad->TotalBUMAD);
    /*compute the number of texture bits*/
    m_Bits -=p_quad->PAveHeaderBits2;

    m_Bits = HL_MATH_MAX(m_Bits,(int)(p_quad->bit_rate/(MINVALUE*p_quad->frame_rate*p_quad->TotalNumberofBasicUnit)));

    dtmp = p_quad->CurrentFrameMAD * p_quad->CurrentFrameMAD * p_quad->m_X1 * p_quad->m_X1 \
           + 4 * p_quad->m_X2 * p_quad->CurrentFrameMAD * m_Bits;
    if ((p_quad->m_X2 == 0.0) || (dtmp < 0) || ((sqrt (dtmp) - p_quad->m_X1 * p_quad->CurrentFrameMAD) <= 0.0)) { // fall back 1st order mode
        m_Qstep = (float)(p_quad->m_X1 * p_quad->CurrentFrameMAD / (double) m_Bits);
    }
    else { // 2nd order mode
        m_Qstep = (float) ((2 * p_quad->m_X2 * p_quad->CurrentFrameMAD) / (sqrt (dtmp) - p_quad->m_X1 * p_quad->CurrentFrameMAD));
    }

    p_quad->m_Qc = _Qstep2QP(m_Qstep, p_quad->bitdepth_qp_scale);
    p_quad->m_Qc = HL_MATH_MIN(m_Qp+p_quad->DDquant,  p_quad->m_Qc); // control variation

    if (basicunit>=(int)p_quad->MBPerRow) {
        p_quad->m_Qc = HL_MATH_MIN(p_quad->PAveFrameQP+6, p_quad->m_Qc);
    }
    else {
        p_quad->m_Qc = HL_MATH_MIN(p_quad->PAveFrameQP+3, p_quad->m_Qc);
    }

    p_quad->m_Qc = HL_MATH_CLIP3(m_Qp-p_quad->DDquant, RCMaxQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc); // clipping
    if (basicunit>=(int)p_quad->MBPerRow) {
        p_quad->m_Qc = HL_MATH_MAX(p_quad->PAveFrameQP-6, p_quad->m_Qc);
    }
    else {
        p_quad->m_Qc = HL_MATH_MAX(p_quad->PAveFrameQP-3, p_quad->m_Qc);
    }

    p_quad->m_Qc = HL_MATH_MAX(RCMinQP + p_quad->bitdepth_qp_scale, p_quad->m_Qc);
}

static int _Qstep2QP( double Qstep, int qp_offset )
{
    int q_per = 0, q_rem = 0;

    if( Qstep < _QP2Qstep(HL_CODEC_264_QP_MIN)) {
        return HL_CODEC_264_QP_MIN;
    }
    else if (Qstep > _QP2Qstep(HL_CODEC_264_QP_MAX + qp_offset) ) {
        return (HL_CODEC_264_QP_MAX + qp_offset);
    }

    while( Qstep > _QP2Qstep(5) ) {
        Qstep /= 2.0;
        q_per++;
    }

    if (Qstep <= 0.65625) {
        //Qstep = 0.625;
        q_rem = 0;
    }
    else if (Qstep <= 0.75) {
        //Qstep = 0.6875;
        q_rem = 1;
    }
    else if (Qstep <= 0.84375) {
        //Qstep = 0.8125;
        q_rem = 2;
    }
    else if (Qstep <= 0.9375) {
        //Qstep = 0.875;
        q_rem = 3;
    }
    else if (Qstep <= 1.0625) {
        //Qstep = 1.0;
        q_rem = 4;
    }
    else {
        //Qstep = 1.125;
        q_rem = 5;
    }

    return (q_per * 6 + q_rem);
}

static double _QP2Qstep( int QP )
{
    int i;
    double Qstep;
    static const double QP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

    Qstep = QP2QSTEP[QP % 6];
    for( i=0; i<(QP/6); i++) {
        Qstep *= 2;
    }

    return Qstep;
}

static void _predictCurrPicMAD( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen )
{
    int i;
    if (((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace))&&(p_gen->FieldControl==1)) {
        p_quad->CurrentFrameMAD=p_quad->MADPictureC1*p_quad->FCBUPFMAD[p_quad->TotalNumberofBasicUnit-p_quad->NumberofBasicUnit]+p_quad->MADPictureC2;
        p_quad->TotalBUMAD=0;
        for (i=p_quad->TotalNumberofBasicUnit-1; i>=(p_quad->TotalNumberofBasicUnit-p_quad->NumberofBasicUnit); i--) {
            p_quad->CurrentBUMAD=p_quad->MADPictureC1*p_quad->FCBUPFMAD[i]+p_quad->MADPictureC2;
            p_quad->TotalBUMAD +=p_quad->CurrentBUMAD*p_quad->CurrentBUMAD;
        }
    }
    else {
        p_quad->CurrentFrameMAD=p_quad->MADPictureC1*p_quad->BUPFMAD[p_quad->TotalNumberofBasicUnit-p_quad->NumberofBasicUnit]+p_quad->MADPictureC2;
        p_quad->TotalBUMAD=0;
        for (i=p_quad->TotalNumberofBasicUnit-1; i>=(p_quad->TotalNumberofBasicUnit-p_quad->NumberofBasicUnit); i--) {
            p_quad->CurrentBUMAD=p_quad->MADPictureC1*p_quad->BUPFMAD[i]+p_quad->MADPictureC2;
            p_quad->TotalBUMAD +=p_quad->CurrentBUMAD*p_quad->CurrentBUMAD;
        }
    }
}

static HL_ERROR_T _rc_allocate_memory( hl_codec_264_t* p_codec )
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    switch (__RCUpdateMode) {
    case RC_MODE_0:
    case RC_MODE_1:
    case RC_MODE_2:
    case RC_MODE_3:
        if (!p_codec->encoder.rc.p_rc_gen) {
            err = _rc_alloc_generic( p_codec, &p_codec->encoder.rc.p_rc_gen );
            if (err) {
                return err;
            }
        }
        if (!p_codec->encoder.rc.p_rc_quad) {
            err = _rc_alloc_quadratic( p_codec, &p_codec->encoder.rc.p_rc_quad );
            if (err) {
                return err;
            }
        }

        if ( p_codec->encoder.rc.RDPictureDecision || __MbInterlace == ADAPTIVE_CODING || __PicInterlace == ADAPTIVE_CODING ) {
            if (!p_codec->encoder.rc.p_rc_gen_init) {
                err = _rc_alloc_generic( p_codec, &p_codec->encoder.rc.p_rc_gen_init );
                if (err) {
                    return err;
                }
            }
            if (!p_codec->encoder.rc.p_rc_quad_init) {
                err = _rc_alloc_quadratic( p_codec, &p_codec->encoder.rc.p_rc_quad_init );
                if (err) {
                    return err;
                }
            }
            if (!p_codec->encoder.rc.p_rc_gen_best) {
                err = _rc_alloc_generic( p_codec, &p_codec->encoder.rc.p_rc_gen_best );
                if (err) {
                    return err;
                }
            }
            if (!p_codec->encoder.rc.p_rc_quad_best) {
                err = _rc_alloc_quadratic( p_codec, &p_codec->encoder.rc.p_rc_quad_best );
                if (err) {
                    return err;
                }
            }
        }
        break;
    default:
        break;
    }

    return err;
}

static HL_ERROR_T _rc_init_sequence(hl_codec_264_t* p_codec)
{
    switch( __RCUpdateMode ) {
    case RC_MODE_0:
    case RC_MODE_1:
    case RC_MODE_2:
    case RC_MODE_3:
        return _rc_init_seq(p_codec, p_codec->encoder.rc.p_rc_quad, p_codec->encoder.rc.p_rc_gen);
        break;
    default:
        break;
    }
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _rc_init_seq(hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen)
{
    double L1,L2,L3,bpp;
    int qp, i, basicunit;
    const hl_codec_264_nal_sps_t* pc_sps;

    switch ( __RCUpdateMode ) {
    case RC_MODE_0:
        p_codec->encoder.rc.updateQP = _updateQPRC0;
        break;
    case RC_MODE_1:
        HL_DEBUG_ERROR("'RC_MODE_1' not implemented")
        return HL_ERROR_NOT_IMPLEMENTED;
    case RC_MODE_2:
        HL_DEBUG_ERROR("'RC_MODE_2' not implemented")
        return HL_ERROR_NOT_IMPLEMENTED;
    case RC_MODE_3:
        HL_DEBUG_ERROR("'RC_MODE_3' not implemented")
        return HL_ERROR_NOT_IMPLEMENTED;
        break;
    default:
        p_codec->encoder.rc.updateQP = _updateQPRC0;
        break;
    }

    pc_sps = p_codec->sps.pc_active;
    basicunit = p_codec->pc_base->rc_basicunit > 0 ? p_codec->pc_base->rc_basicunit : pc_sps->uPicSizeInMapUnits;

    p_quad->Xp=0;
    p_quad->Xb=0;

    p_quad->bit_rate = (float) p_codec->encoder.rc.bit_rate;
    p_quad->frame_rate = (float) p_codec->encoder.rc.frame_rate;
    p_quad->PrevBitRate = p_quad->bit_rate;

    /*compute the total number of MBs in a frame*/
    if (basicunit > (int32_t)pc_sps->uPicSizeInMapUnits) {
        basicunit = pc_sps->uPicSizeInMapUnits;
    }
    if (basicunit < (int32_t)pc_sps->uPicSizeInMapUnits) {
        p_quad->TotalNumberofBasicUnit = pc_sps->uPicSizeInMapUnits/basicunit;
    }
    else {
        p_quad->TotalNumberofBasicUnit = 1;
    }

    /*initialize the parameters of fluid flow traffic model*/
    p_gen->CurrentBufferFullness = 0;
    p_quad->GOPTargetBufferLevel = (double) p_gen->CurrentBufferFullness;

    /*initialize the previous window size*/
    p_quad->m_windowSize    = 0;
    p_quad->MADm_windowSize = 0;
    p_gen->NumberofCodedBFrame = 0;
    p_quad->NumberofCodedPFrame = 0;
    p_gen->NumberofGOP         = 0;
    /*remaining # of bits in GOP */
    p_gen->RemainingBits = 0;
    /*control parameter */
    if (__NumberBFrames>0) {
        p_quad->GAMMAP=0.25;
        p_quad->BETAP=0.9;
    }
    else {
        p_quad->GAMMAP=0.5;
        p_quad->BETAP=0.5;
    }

    /*quadratic rate-distortion model*/
    p_quad->PPreHeader=0;

    p_quad->Pm_X1 = p_quad->bit_rate * 1.0;
    p_quad->Pm_X2 = 0.0;
    /* linear prediction model for P picture*/
    p_quad->PMADPictureC1 = 1.0;
    p_quad->PMADPictureC2 = 0.0;

    // Initialize values
    for (i=0; i<RC_MODEL_HISTORY; i++) {
        p_quad->Pm_rgQp[i] = 0;
        p_quad->Pm_rgRp[i] = 0.0;
        p_quad->PPictureMAD[i] = 0.0;
    }

    //Define the largest variation of quantization parameters
    p_quad->PMaxQpChange = __RCMaxQPChange;

    /*basic unit layer rate control*/
    p_quad->PAveHeaderBits1 = 0;
    p_quad->PAveHeaderBits3 = 0;
    p_quad->DDquant = (p_quad->TotalNumberofBasicUnit>=9 ? 1 : 2);

    p_quad->MBPerRow = pc_sps->uPicWidthInMbs;

    /*adaptive field/frame coding*/
    p_gen->FieldControl=0;

    if (p_codec->encoder.rc.SeinitialQP==0) {
        /*compute the initial QP*/
        bpp = 1.0*p_quad->bit_rate /(p_quad->frame_rate * p_codec->encoder.rc.size);

        if (p_codec->encoder.rc.size <= HL_CODEC_264_RC_QCIF_NPIX) {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        }
        else if (p_codec->encoder.rc.size <= HL_CODEC_264_RC_CIF_NPIX) {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        }
        else {
            L1 = 0.6;
            L2 = 1.4;
            L3 = 2.4;
        }
        if (bpp<= L1) {
            qp = 35;
        }
        else if(bpp<=L2) {
            qp = 25;
        }
        else if(bpp<=L3) {
            qp = 20;
        }
        else {
            qp = 10;
        }
        p_codec->encoder.rc.SeinitialQP = qp;
    }

    // high bit-depth
    p_quad->bitdepth_qp_scale = p_codec->encoder.rc.bitdepth_luma_qp_scale;

    return HL_ERROR_SUCCESS;
}


static void _rc_store_slice_header_bits( hl_codec_264_t* p_codec, int32_t len )
{
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;

    switch (__RCUpdateMode) {
    case RC_MODE_0:
    case RC_MODE_1:
    case RC_MODE_2:
    case RC_MODE_3:
        p_codec->encoder.rc.p_rc_gen->NumberofHeaderBits +=len;

        // basic unit layer rate control
        if(p_codec->encoder.rc.BasicUnit < (int32_t)pc_sps->uPicSizeInMapUnits) {
            p_codec->encoder.rc.p_rc_gen->NumberofBasicUnitHeaderBits +=len;
        }
        break;
    default:
        break;
    }
}

static void _rc_update_pict(hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, int nbits)
{
    int delta_bits = (nbits - (int)floor(p_quad->bit_rate / p_quad->frame_rate + 0.5F) );
    // remaining # of bits in GOP
    p_gen->RemainingBits -= nbits;
    p_gen->CurrentBufferFullness += delta_bits;

    // update the lower bound and the upper bound for the target bits of each frame, HRD consideration
    p_quad->LowerBound  -= (int) delta_bits;
    p_quad->UpperBound1 -= (int) delta_bits;
    p_quad->UpperBound2  = (int)(OMEGA * p_quad->UpperBound1);

    // update the parameters of quadratic R-D model
    if( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && p_codec->encoder.rc.curr_frm_idx) ) {
        _updateRCModel(p_codec, p_quad, p_gen, SliceTypeModulo5);
        if ( __RCUpdateMode == RC_MODE_3 ) {
            p_quad->PreviousWholeFrameMAD = _ComputeFrameMAD(p_codec);
        }
    }
}

static void _rc_update_picture( hl_codec_264_t* p_codec, int bits, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5 )
{
    _rc_update_pict(p_codec, p_codec->encoder.rc.p_rc_quad, p_codec->encoder.rc.p_rc_gen, SliceTypeModulo5, bits);
}

static void _rc_update_pict_frame(hl_codec_264_t* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, RCQuadratic *p_quad, RCGeneric *p_gen, int nbits)
{
    /* update the complexity weight of I, P, B frame */
    int complexity = 0;
    Boolean is_B_slice = (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B);

    switch( __RCUpdateMode ) {
    case RC_MODE_0:
    case RC_MODE_2:
    default:
        complexity = _updateComplexity( p_codec, p_quad, p_gen, (Boolean) (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P), is_B_slice, nbits );
        if ( SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P ) {
            if( p_gen->NoGranularFieldRC == 0 || p_gen->FieldControl == 0 ) {
                _updatePparams( p_quad, p_gen, complexity );
            }
            else {
                p_gen->NoGranularFieldRC = 0;
            }
        }
        else if ( SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B ) {
            _updateBparams( p_quad, p_gen, complexity );
        }
        break;
    case RC_MODE_1:
        complexity = _updateComplexity( p_codec, p_quad, p_gen, (Boolean) (p_codec->encoder.rc.number != 0), is_B_slice, nbits );
        if ( p_codec->encoder.rc.number != 0 ) {
            if( p_gen->NoGranularFieldRC == 0 || p_gen->FieldControl == 0 ) {
                _updatePparams( p_quad, p_gen, complexity );
            }
            else {
                p_gen->NoGranularFieldRC = 0;
            }
        }
        break;
    case RC_MODE_3:
        complexity = _updateComplexity( p_codec, p_quad, p_gen, (Boolean) (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P), is_B_slice, nbits );
        if (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_I && (p_codec->encoder.rc.number != 0)) {
            p_gen->NISlice--;
        }

        if ( SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P ) {
            if( p_gen->NoGranularFieldRC == 0 || p_gen->FieldControl == 0 ) {
                _updatePparams( p_quad, p_gen, complexity );
                p_gen->NPSlice--;
            }
            else {
                p_gen->NoGranularFieldRC = 0;
            }
        }
        else if ( SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B ) {
            _updateBparams( p_quad, p_gen, complexity );
            p_gen->hierNb[ __HierarchicalCoding ? (__p_curr_frm_struct_layer - 1) : 0 ]--;
        }
        break;
    }
}

static int32_t _updateComplexity( hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, Boolean is_updated, Boolean is_B_slice, int nbits )
{
    double Avem_Qc;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;

    /* frame layer rate control */
    if (p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
        return ((int) HL_MATH_FLOOR(nbits * p_quad->m_Qc + 0.5));
    }
    /* basic unit layer rate control */
    else {
        if ( is_updated ) {
            if ( p_gen->NoGranularFieldRC == 0 || p_gen->FieldControl == 0 ) {
                Avem_Qc = (double)p_quad->TotalFrameQP / (double)p_quad->TotalNumberofBasicUnit;
                return ((int)HL_MATH_FLOOR(nbits * Avem_Qc + 0.5));
            }
        }
        else if ( /* p_codec->encoder.rc.type == B_SLICE*/ is_B_slice ) {
            return ((int) HL_MATH_FLOOR(nbits * p_quad->m_Qc + 0.5));
        }
    }
    return 0;
}

static double _ComputeFrameMAD(hl_codec_264_t* p_codec)
{
    int64_t TotalMAD = 0;
    const hl_codec_264_nal_sps_t* pc_sps;
    unsigned int i;

    pc_sps = p_codec->sps.pc_active;

    for (i = 0; i < pc_sps->uPicSizeInMapUnits; i++) {
        TotalMAD += p_codec->encoder.rc.p_rc_gen->MADofMB[i];
    }
    return (double)TotalMAD / (256.0 * (double)pc_sps->uPicSizeInMapUnits);
}

static void _updatePparams( RCQuadratic *p_quad, RCGeneric *p_gen, int complexity )
{
    p_quad->Xp = complexity;
    p_quad->Np--;
    p_quad->Wp = p_quad->Xp;
    p_quad->Pm_Hp = p_gen->NumberofHeaderBits;
    p_quad->NumberofCodedPFrame++;
    p_quad->NumberofPPicture++;
}

static void _updateBparams( RCQuadratic *p_quad, RCGeneric *p_gen, int complexity )
{
    p_quad->Xb = complexity;
    p_quad->Nb--;
    p_quad->Wb = p_quad->Xb / THETA;
    p_quad->NumberofBFrames++;
    p_gen->NumberofCodedBFrame++;
}

static void _RCModelEstimator (hl_codec_264_t* p_codec, RCQuadratic *p_quad, int n_windowSize, Boolean *m_rgRejected)
{
    int n_realSize = n_windowSize;
    int i;
    double oneSampleQ = 0;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double MatrixValue;
    Boolean estimateX2 = FALSE;
    HL_CODEC_264_SLICE_TYPE_T SliceTypeModulo5 = (p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA) ? HL_CODEC_264_SLICE_TYPE_I : HL_CODEC_264_SLICE_TYPE_P;

    for (i = 0; i < n_windowSize; i++) {
        // find the number of samples which are not rejected
        if (m_rgRejected[i]) {
            n_realSize--;
        }
    }

    // default RD model estimation results
    p_quad->m_X1 = p_quad->m_X2 = 0.0;

    for (i = 0; i < n_windowSize; i++) {
        if (!m_rgRejected[i]) {
            oneSampleQ = p_quad->m_rgQp[i];
        }
    }
    for (i = 0; i < n_windowSize; i++) {
        // if all non-rejected Q are the same, take 1st order model
        if ((p_quad->m_rgQp[i] != oneSampleQ) && !m_rgRejected[i]) {
            estimateX2 = TRUE;
        }
        if (!m_rgRejected[i]) {
            p_quad->m_X1 += (p_quad->m_rgQp[i] * p_quad->m_rgRp[i]) / n_realSize;
        }
    }

    // take 2nd order model to estimate X1 and X2
    if ((n_realSize >= 1) && estimateX2) {
        for (i = 0; i < n_windowSize; i++) {
            if (!m_rgRejected[i]) {
                a00  = a00 + 1.0;
                a01 += 1.0 / p_quad->m_rgQp[i];
                a10  = a01;
                a11 += 1.0 / (p_quad->m_rgQp[i] * p_quad->m_rgQp[i]);
                b0  += p_quad->m_rgQp[i] * p_quad->m_rgRp[i];
                b1  += p_quad->m_rgRp[i];
            }
        }
        // solve the equation of AX = B
        MatrixValue=a00*a11-a01*a10;
        if(fabs(MatrixValue) > 0.000001) {
            p_quad->m_X1 = (b0 * a11 - b1 * a01) / MatrixValue;
            p_quad->m_X2 = (b1 * a00 - b0 * a10) / MatrixValue;
        }
        else {
            p_quad->m_X1 = b0 / a00;
            p_quad->m_X2 = 0.0;
        }
    }
    if( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) {
        p_quad->Pm_X1 = p_quad->m_X1;
        p_quad->Pm_X2 = p_quad->m_X2;
    }
}

static void _MADModelEstimator (hl_codec_264_t* p_codec, RCQuadratic *p_quad, int n_windowSize, Boolean *PictureRejected)
{
    int    n_realSize = n_windowSize;
    int    i;
    double oneSampleQ = 0.0;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double MatrixValue;
    Boolean estimateX2 = FALSE;
    HL_CODEC_264_SLICE_TYPE_T SliceTypeModulo5 = (p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA) ? HL_CODEC_264_SLICE_TYPE_I : HL_CODEC_264_SLICE_TYPE_P;

    for (i = 0; i < n_windowSize; i++) {
        // find the number of samples which are not rejected
        if (PictureRejected[i]) {
            n_realSize--;
        }
    }

    // default MAD model estimation results
    p_quad->MADPictureC1 = p_quad->MADPictureC2 = 0.0;

    for (i = 0; i < n_windowSize; i++) {
        if (!PictureRejected[i]) {
            oneSampleQ = p_quad->PictureMAD[i];
        }
    }

    for (i = 0; i < n_windowSize; i++) {
        // if all non-rejected MAD are the same, take 1st order model
        if ((p_quad->PictureMAD[i] != oneSampleQ) && !PictureRejected[i]) {
            estimateX2 = TRUE;
        }
        if (!PictureRejected[i]) {
            p_quad->MADPictureC1 += p_quad->PictureMAD[i] / (p_quad->ReferenceMAD[i]*n_realSize);
        }
    }

    // take 2nd order model to estimate X1 and X2
    if ((n_realSize >= 1) && estimateX2) {
        for (i = 0; i < n_windowSize; i++) {
            if (!PictureRejected[i]) {
                a00  = a00 + 1.0;
                a01 += p_quad->ReferenceMAD[i];
                a10  = a01;
                a11 += p_quad->ReferenceMAD[i] * p_quad->ReferenceMAD[i];
                b0  += p_quad->PictureMAD[i];
                b1  += p_quad->PictureMAD[i]   * p_quad->ReferenceMAD[i];
            }
        }
        // solve the equation of AX = B
        MatrixValue = a00 * a11 - a01 * a10;
        if(fabs(MatrixValue) > 0.000001) {
            p_quad->MADPictureC2 = (b0 * a11 - b1 * a01) / MatrixValue;
            p_quad->MADPictureC1 = (b1 * a00 - b0 * a10) / MatrixValue;
        }
        else {
            p_quad->MADPictureC1 = b0/a01;
            p_quad->MADPictureC2 = 0.0;
        }
    }
    if( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) {
        p_quad->PMADPictureC1 = p_quad->MADPictureC1;
        p_quad->PMADPictureC2 = p_quad->MADPictureC2;
    }
}

static void _updateMADModel (hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen)
{
    int    n_windowSize;
    int    i;
    double std = 0.0, threshold;
    int    m_Nc = p_quad->NumberofCodedPFrame;
    Boolean PictureRejected[RC_MODEL_HISTORY];
    double  error          [RC_MODEL_HISTORY];
    const hl_codec_264_nal_sps_t* pc_sps;
    HL_CODEC_264_SLICE_TYPE_T SliceTypeModulo5 = (p_codec->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA) ? HL_CODEC_264_SLICE_TYPE_I : HL_CODEC_264_SLICE_TYPE_P;

    pc_sps = p_codec->sps.pc_active;

    if(p_quad->NumberofCodedPFrame>0) {
        //assert (p_codec->encoder.rc.type!=P_SLICE);
        /*frame layer rate control*/
        if(p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
            m_Nc = p_quad->NumberofCodedPFrame;
        }
        else { // basic unit layer rate control
            m_Nc=p_quad->NumberofCodedPFrame*p_quad->TotalNumberofBasicUnit+p_quad->CodedBasicUnit;
        }

        for (i = (RC_MODEL_HISTORY-2); i > 0; i--) {
            // update the history
            p_quad->PPictureMAD[i]  = p_quad->PPictureMAD[i - 1];
            p_quad->PictureMAD[i]   = p_quad->PPictureMAD[i];
            p_quad->ReferenceMAD[i] = p_quad->ReferenceMAD[i-1];
        }
        p_quad->PPictureMAD[0] = p_quad->CurrentFrameMAD;
        p_quad->PictureMAD[0]  = p_quad->PPictureMAD[0];

        if(p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
            p_quad->ReferenceMAD[0]=p_quad->PictureMAD[1];
        }
        else {
            if (((__PicInterlace==ADAPTIVE_CODING)||(__MbInterlace)) &&(p_gen->FieldControl==1)) {
                p_quad->ReferenceMAD[0]=p_quad->FCBUPFMAD[p_quad->TotalNumberofBasicUnit-1-p_quad->NumberofBasicUnit];
            }
            else {
                p_quad->ReferenceMAD[0]=p_quad->BUPFMAD[p_quad->TotalNumberofBasicUnit-1-p_quad->NumberofBasicUnit];
            }
        }
        p_quad->MADPictureC1 = p_quad->PMADPictureC1;
        p_quad->MADPictureC2 = p_quad->PMADPictureC2;

        /*compute the size of window*/
        n_windowSize = (p_quad->CurrentFrameMAD > p_quad->PreviousFrameMAD)
                       ? (int) ((float)(RC_MODEL_HISTORY-1) * p_quad->PreviousFrameMAD / p_quad->CurrentFrameMAD)
                       : (int) ((float)(RC_MODEL_HISTORY-1) * p_quad->CurrentFrameMAD / p_quad->PreviousFrameMAD);
        n_windowSize = HL_MATH_CLIP3(1, (m_Nc-1), n_windowSize);
        n_windowSize=HL_MATH_MIN(n_windowSize, HL_MATH_MIN(20, p_quad->MADm_windowSize + 1));

        /*update the previous window size*/
        p_quad->MADm_windowSize=n_windowSize;

        for (i = 0; i < (RC_MODEL_HISTORY-1); i++) {
            PictureRejected[i] = FALSE;
        }

        //update the MAD for the previous frame
        if( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) {
            p_quad->PreviousFrameMAD=p_quad->CurrentFrameMAD;
        }

        // initial MAD model estimator
        _MADModelEstimator (p_codec, p_quad, n_windowSize, PictureRejected);

        // remove outlier
        for (i = 0; i < n_windowSize; i++) {
            error[i] = p_quad->MADPictureC1 * p_quad->ReferenceMAD[i] + p_quad->MADPictureC2 - p_quad->PictureMAD[i];
            std += (error[i] * error[i]);
        }

        threshold = (n_windowSize == 2) ? 0 : sqrt (std / n_windowSize);
        for (i = 0; i < n_windowSize; i++) {
            if (fabs(error[i]) > threshold) {
                PictureRejected[i] = TRUE;
            }
        }
        // always include the last data point
        PictureRejected[0] = FALSE;

        // second MAD model estimator
        _MADModelEstimator (p_codec, p_quad, n_windowSize, PictureRejected);
    }
}

static void _updateRCModel (hl_codec_264_t* p_codec, RCQuadratic *p_quad, RCGeneric *p_gen, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5)
{
    int n_windowSize;
    int i;
    double std = 0.0, threshold;
    int m_Nc = p_quad->NumberofCodedPFrame;
    Boolean MADModelFlag = FALSE;
    Boolean m_rgRejected[RC_MODEL_HISTORY];
    double  error       [RC_MODEL_HISTORY];
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_sps = p_codec->sps.pc_active;

    if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) {
        /*frame layer rate control*/
        if (p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
            p_quad->CurrentFrameMAD = _ComputeFrameMAD(p_codec);
            m_Nc=p_quad->NumberofCodedPFrame;
        }
        /*basic unit layer rate control*/
        else {
            /*compute the MAD of the current basic unit*/
            p_quad->CurrentFrameMAD = (double) ((p_gen->TotalMADBasicUnit >> 8)/p_codec->encoder.rc.BasicUnit);
            p_gen->TotalMADBasicUnit=0;

            /* compute the average number of header bits*/
            p_quad->CodedBasicUnit=p_quad->TotalNumberofBasicUnit-p_quad->NumberofBasicUnit;
            if (p_quad->CodedBasicUnit > 0) {
                p_quad->PAveHeaderBits1=(int)((double)(p_quad->PAveHeaderBits1*(p_quad->CodedBasicUnit-1)+
                                                       p_gen->NumberofBasicUnitHeaderBits)/p_quad->CodedBasicUnit+0.5);
                if (p_quad->PAveHeaderBits3 == 0) {
                    p_quad->PAveHeaderBits2 = p_quad->PAveHeaderBits1;
                }
                else {
                    p_quad->PAveHeaderBits2 = (int)((double)(p_quad->PAveHeaderBits1 * p_quad->CodedBasicUnit+
                                                    p_quad->PAveHeaderBits3 * p_quad->NumberofBasicUnit)/p_quad->TotalNumberofBasicUnit+0.5);
                }
            }

            if ((p_quad->NumberofBasicUnit >= p_quad->TotalNumberofBasicUnit) || (p_quad->NumberofBasicUnit<0)) {
                static const int __framepoc = 0;
                HL_DEBUG_ERROR("Write into invalid memory in updateRCModel at frame %d, p_quad->NumberofBasicUnit %d\n",
                               __framepoc, p_quad->NumberofBasicUnit);
            }

            /*update the record of MADs for reference*/
            if (((__PicInterlace == ADAPTIVE_CODING) || (__MbInterlace)) && (p_gen->FieldControl == 1)) {
                p_quad->FCBUCFMAD[p_quad->TotalNumberofBasicUnit-1-p_quad->NumberofBasicUnit]=p_quad->CurrentFrameMAD;
            }
            else {
                p_quad->BUCFMAD[p_quad->TotalNumberofBasicUnit-1-p_quad->NumberofBasicUnit]=p_quad->CurrentFrameMAD;
            }

            if (p_quad->NumberofBasicUnit != 0) {
                m_Nc = p_quad->NumberofCodedPFrame * p_quad->TotalNumberofBasicUnit + p_quad->CodedBasicUnit;
            }
            else {
                m_Nc = (p_quad->NumberofCodedPFrame-1) * p_quad->TotalNumberofBasicUnit + p_quad->CodedBasicUnit;
            }
        }

        if (m_Nc > 1) {
            MADModelFlag=TRUE;
        }

        p_quad->PPreHeader = p_gen->NumberofHeaderBits;
        for (i = (RC_MODEL_HISTORY-2); i > 0; i--) {
            // update the history
            p_quad->Pm_rgQp[i] = p_quad->Pm_rgQp[i - 1];
            p_quad->m_rgQp[i]  = p_quad->Pm_rgQp[i];
            p_quad->Pm_rgRp[i] = p_quad->Pm_rgRp[i - 1];
            p_quad->m_rgRp[i]  = p_quad->Pm_rgRp[i];
        }
        p_quad->Pm_rgQp[0] = _QP2Qstep(p_quad->m_Qc); //*1.0/p_quad->CurrentFrameMAD;
        /*frame layer rate control*/
        if (p_codec->encoder.rc.BasicUnit == pc_sps->uPicSizeInMapUnits) {
            p_quad->Pm_rgRp[0] = p_gen->NumberofTextureBits*1.0/p_quad->CurrentFrameMAD;
        }
        /*basic unit layer rate control*/
        else {
            p_quad->Pm_rgRp[0] = p_gen->NumberofBasicUnitTextureBits*1.0/p_quad->CurrentFrameMAD;
        }

        p_quad->m_rgQp[0] = p_quad->Pm_rgQp[0];
        p_quad->m_rgRp[0] = p_quad->Pm_rgRp[0];
        p_quad->m_X1 = p_quad->Pm_X1;
        p_quad->m_X2 = p_quad->Pm_X2;

        /*compute the size of window*/
        n_windowSize = (p_quad->CurrentFrameMAD>p_quad->PreviousFrameMAD)
                       ? (int)(p_quad->PreviousFrameMAD/p_quad->CurrentFrameMAD * (RC_MODEL_HISTORY-1) )
                       : (int)(p_quad->CurrentFrameMAD/p_quad->PreviousFrameMAD *(RC_MODEL_HISTORY-1));
        n_windowSize=HL_MATH_CLIP3(1, m_Nc, n_windowSize);
        n_windowSize=HL_MATH_MIN(n_windowSize,p_quad->m_windowSize+1);
        n_windowSize=HL_MATH_MIN(n_windowSize,(RC_MODEL_HISTORY-1));

        /*update the previous window size*/
        p_quad->m_windowSize=n_windowSize;

        for (i = 0; i < (RC_MODEL_HISTORY-1); i++) {
            m_rgRejected[i] = FALSE;
        }

        // initial RD model estimator
        _RCModelEstimator (p_codec, p_quad, n_windowSize, m_rgRejected);

        n_windowSize = p_quad->m_windowSize;
        // remove outlier

        for (i = 0; i < (int) n_windowSize; i++) {
            error[i] = p_quad->m_X1 / p_quad->m_rgQp[i] + p_quad->m_X2 / (p_quad->m_rgQp[i] * p_quad->m_rgQp[i]) - p_quad->m_rgRp[i];
            std += error[i] * error[i];
        }
        threshold = (n_windowSize == 2) ? 0 : sqrt (std / n_windowSize);
        for (i = 0; i < (int) n_windowSize; i++) {
            if (fabs(error[i]) > threshold) {
                m_rgRejected[i] = TRUE;
            }
        }
        // always include the last data point
        m_rgRejected[0] = FALSE;

        // second RD model estimator
        _RCModelEstimator (p_codec, p_quad, n_windowSize, m_rgRejected);

        if ( MADModelFlag ) {
            _updateMADModel(p_codec, p_quad, p_gen);
        }
        else if ( (SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P) || (__RCUpdateMode == RC_MODE_1 && (p_codec->encoder.rc.number != 0)) ) {
            p_quad->PPictureMAD[0] = p_quad->CurrentFrameMAD;
        }
    }
}



/*** OBJECT DEFINITION FOR "hl_codec_264_rc_gop_t" ***/
static hl_object_t* hl_codec_264_rc_gop_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_rc_gop_t *p_rc_gop = (hl_codec_264_rc_gop_t*)self;
    if (p_rc_gop) {

    }
    return self;
}
static hl_object_t* hl_codec_264_rc_gop_dtor(hl_object_t * self)
{
    hl_codec_264_rc_gop_t *p_rc_gop = (hl_codec_264_rc_gop_t*)self;
    if (p_rc_gop) {

    }
    return self;
}
static int hl_codec_264_rc_gop_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_rc_gop_def_s = {
    sizeof(hl_codec_264_rc_gop_t),
    hl_codec_264_rc_gop_ctor,
    hl_codec_264_rc_gop_dtor,
    hl_codec_264_rc_gop_cmp,
};
const hl_object_def_t *hl_codec_264_rc_gop_def_t = &hl_codec_264_rc_gop_def_s;






#else /* ===Doubango Telecom implementation based on JVT-O079=== */


#if !defined(HL_CODEC_264_RC_L1_LOW)
#	define HL_CODEC_264_RC_L1_LOW			0.15 // l1 when Npix (number of pixel in the picture) <= CIF
#endif
#if !defined(HL_CODEC_264_RC_L2_LOW)
#	define HL_CODEC_264_RC_L2_LOW			0.45 // l2 when Npix <= CIF
#endif
#if !defined(HL_CODEC_264_RC_L3_LOW)
#	define HL_CODEC_264_RC_L3_LOW			0.9 // l2 when Npix <= CIF
#endif
#if !defined(HL_CODEC_264_RC_L1_HIGH)
#	define HL_CODEC_264_RC_L1_HIGH			0.6 // l1 when Npix > CIF
#endif
#if !defined(HL_CODEC_264_RC_L2_HIGH)
#	define HL_CODEC_264_RC_L2_HIGH			1.4 // l2 when Npix > CIF
#endif
#if !defined(HL_CODEC_264_RC_L3_HIGH)
#	define HL_CODEC_264_RC_L3_HIGH			2.4 // l2 when Npix > CIF
#endif
#if !defined(HL_CODEC_264_RC_GAMMA_NO_NONSTORED_PICTS)
#	define HL_CODEC_264_RC_GAMMA_NO_NONSTORED_PICTS			0.5
#endif
#if !defined(HL_CODEC_264_RC_BETA_NO_NONSTORED_PICTS)
#	define HL_CODEC_264_RC_BETA_NO_NONSTORED_PICTS			0.5
#endif

extern const hl_object_def_t *hl_codec_264_rc_gop_def_t;

typedef struct hl_codec_264_rc_pict_xs {
    hl_size_t u_idx;
    int64_t i_r; // Ri(j): the instant available bit rate
    int64_t i_v; // Vi(j): the occupancy of the virtual buffer
    int64_t i_B; // Bi(j): the total bits for the rest pictures
    int64_t i_m; // mhi(j): the total number of header bits and motion vector bits,
    int64_t i_s; // Si(j): the target buffer level
    int64_t i_b; // bi(j): the actual generated bits
    int64_t i_t; // Ti(j): the target bits
    int64_t i_qpp; // QPp,i(j): the QP value for the current picture
    int64_t i_wp; // Wp,i(j)
    int64_t i_mad; // MADi(j)
}
hl_codec_264_rc_pict_xt;

typedef struct hl_codec_264_rc_gop_s {
    HL_DECLARE_OBJECT;

    hl_size_t u_idx;
    int64_t i_s_next; // Si(j + 1): the target buffer level
    int64_t i_np; // Np(i): the number of P-Frames in the GOP
    double d_a1;
    double d_a2; // linear coeff.

    struct hl_codec_264_rc_pict_xs* pc_pict_active;
    struct hl_codec_264_rc_pict_xs** pp_list_picts;
    hl_size_t u_list_picts_count;
    hl_size_t u_list_picts_idx;
}
hl_codec_264_rc_gop_t;

static HL_ERROR_T _hl_codec_264_rc_gop_create(hl_codec_264_t* p_codec, hl_codec_264_rc_gop_t** pp_rc_gop)
{
    if (!pp_rc_gop || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_rc_gop = hl_object_create(hl_codec_264_rc_gop_def_t);
    if (!*pp_rc_gop) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    if (!((*pp_rc_gop)->pp_list_picts = hl_memory_calloc(p_codec->pc_base->gop_size, sizeof(hl_codec_264_rc_pict_xt*)))) {
        HL_OBJECT_SAFE_FREE((*pp_rc_gop));
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_rc_gop)->u_list_picts_count = p_codec->pc_base->gop_size;

    return HL_ERROR_SUCCESS;
}

// Call this function every time we start coding an IDR frame (start of the GOP)
HL_ERROR_T hl_codec_264_rc_start_gop(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_nal_sps_t* pc_sps;
    hl_codec_264_rc_gop_t* p_gop;
    size_t u_nex_gop_idx;

    pc_sps = p_codec->sps.pc_active;

    // Is previous GOP full?
    p_gop = p_codec->encoder.rc.p_list_gops[p_codec->encoder.rc.u_p_list_gops_idx];
    if (!p_gop || (int32_t)(p_gop->u_list_picts_idx + 1) < p_codec->pc_base->gop_size) {
        u_nex_gop_idx = p_codec->encoder.rc.u_p_list_gops_idx;
    }
    else {
        u_nex_gop_idx = (p_codec->encoder.rc.u_p_list_gops_idx + 1) % HL_CODEC_264_RC_GOP_WINDOW_SIZE_MAX;
    }

    // Create the next GOP
    if (!(p_gop = p_codec->encoder.rc.p_list_gops[u_nex_gop_idx])) {
        err = _hl_codec_264_rc_gop_create(p_codec, &p_gop);
        if (err) {
            return err;
        }
        // Add the GOP to the list
        HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
            p_codec->encoder.rc.p_list_gops,
            p_codec->encoder.rc.u_p_list_gops_count,
            HL_CODEC_264_RC_GOP_WINDOW_SIZE_MAX,
            p_gop,
            u_nex_gop_idx, err);

        if (err) {
            HL_OBJECT_SAFE_FREE(p_gop);
            return err;
        }
        p_gop = p_codec->encoder.rc.p_list_gops[u_nex_gop_idx];
        p_gop->u_idx = u_nex_gop_idx;
    }
    p_gop->u_list_picts_idx = 0;
    p_gop->i_np = 0;
    p_gop->d_a1 = 1;
    p_gop->d_a2 = 0;
    p_codec->encoder.rc.u_p_list_gops_idx = u_nex_gop_idx;
    p_codec->encoder.rc.pc_gop_active = p_gop;

    if ((!p_codec->encoder.rc.i_np || !p_codec->encoder.rc.i_qp_p_avg_sum)) {
        double d_gop_br, d_gop_sec, d_bpp, d_f, d_npix, d_l1, d_l2, d_l3;

        d_npix = p_codec->encoder.pc_frame->data_size[0] /*+ p_codec->encoder.pc_frame->data_size[1] + p_codec->encoder.pc_frame->data_size[2]*/;
        d_f = p_codec->pc_base->fps.den / p_codec->pc_base->fps.num;
        d_gop_sec = 1.0/*(p_codec->encoder.gop_left / d_f)*/; // number of seconds left in the gop
        d_gop_br = (p_codec->encoder.rc.i_bitrate * d_gop_sec); // total bits (R) in the gop
        d_bpp = d_gop_br / ( d_f * d_npix);

        if (d_npix <= HL_CODEC_264_RC_CIF_NPIX) {
            // Picture <= CIF
            d_l1 = HL_CODEC_264_RC_L1_LOW;
            d_l2 = HL_CODEC_264_RC_L2_LOW;
            d_l3 = HL_CODEC_264_RC_L3_LOW;
        }
        else {
            // Picture > CIF
            d_l1 = HL_CODEC_264_RC_L1_HIGH;
            d_l2 = HL_CODEC_264_RC_L2_HIGH;
            d_l3 = HL_CODEC_264_RC_L3_HIGH;
        }

        // Guess initial qp
        p_codec->encoder.i_qp = p_codec->pc_base->qp;
        if (d_bpp <= d_l1) {
            p_codec->encoder.i_qp = 40;
        }
        else if (d_bpp > d_l1 && d_bpp <= d_l2) {
            p_codec->encoder.i_qp = 30;
        }
        else if (d_bpp > d_l2 && d_bpp <= d_l3) {
            p_codec->encoder.i_qp = 20;
        }
        else if (d_bpp > d_l3) {
            p_codec->encoder.i_qp = 10;
        }
    }
    else {
        int32_t i_qp_prim, i_tmp;

        i_tmp = (p_codec->encoder.rc.i_np / 15);
        i_qp_prim = (p_codec->encoder.rc.i_qp_p_avg_sum /  p_codec->encoder.rc.i_np) - HL_MATH_MIN(2, i_tmp);
        i_tmp = p_codec->encoder.i_qp + 2;
        i_tmp = HL_MATH_MIN(i_tmp, i_qp_prim);
        p_codec->encoder.i_qp = HL_MATH_MAX((p_codec->encoder.i_qp - 2), i_tmp);
    }

    // Clip QP
    if (p_codec->pc_base->rc_qp_min >= 0 && p_codec->pc_base->rc_qp_max >= 0 && (p_codec->pc_base->rc_qp_max >= p_codec->pc_base->rc_qp_min)) {
        p_codec->encoder.i_qp = HL_MATH_CLIP3(p_codec->pc_base->rc_qp_min, p_codec->pc_base->rc_qp_max, p_codec->encoder.i_qp);
    }

    // Reset RC stats
    p_codec->encoder.rc.i_np = 0;
    p_codec->encoder.rc.i_qp_p_avg_sum = 0;
    p_codec->encoder.rc.i_qp_p_avg_sum_acc = 0;

    HL_DEBUG_INFO("[RC] qp for current GOP=%d", p_codec->encoder.i_qp);

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_pict_start(hl_codec_264_t* p_codec)
{
    hl_codec_264_rc_gop_t* pc_gop = p_codec->encoder.rc.pc_gop_active;
    hl_size_t u_list_picts_idx = pc_gop->u_list_picts_idx;
    hl_codec_264_rc_pict_xt* p_pict;
    p_codec->encoder.rc.i_pict_bits_hdr = 0;
    p_codec->encoder.rc.i_pict_bits_data = 0;
    p_codec->encoder.rc.i_pict_mad = 0;
    if (p_codec->encoder.rc.pc_gop_active->pp_list_picts[u_list_picts_idx]) {
        u_list_picts_idx = (u_list_picts_idx + 1) % p_codec->encoder.rc.pc_gop_active->u_list_picts_count;
    }

    if (!(p_pict = p_codec->encoder.rc.pc_gop_active->pp_list_picts[u_list_picts_idx])) {
        p_pict = hl_memory_calloc(sizeof(hl_codec_264_rc_pict_xt), 1);
        if (!p_pict) {
            return HL_ERROR_OUTOFMEMMORY;
        }
        p_codec->encoder.rc.pc_gop_active->pp_list_picts[u_list_picts_idx] = p_pict;
    }

    p_pict->u_idx = u_list_picts_idx;
    p_pict->i_qpp = p_codec->encoder.i_qp;
    p_pict->i_s = pc_gop->i_s_next; // Si(j + 1) - (2-72)
    p_codec->encoder.rc.pc_gop_active->pc_pict_active = p_pict;
    p_codec->encoder.rc.pc_gop_active->u_list_picts_idx = u_list_picts_idx;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rc_pict_end(hl_codec_264_t* p_codec)
{
    hl_codec_264_rc_gop_t* pc_gop = p_codec->encoder.rc.pc_gop_active;
    hl_codec_264_rc_pict_xt* pc_pict = pc_gop->pc_pict_active;
    size_t Ni;

    HL_DEBUG_INFO("[RC] GOP(%u), PICT(%u), Bits(HDR)=%lli, Bits(DATA)=%lli", pc_gop->u_idx, pc_pict->u_idx, p_codec->encoder.rc.i_pict_bits_hdr, p_codec->encoder.rc.i_pict_bits_data);

    Ni = (pc_pict->u_idx + 1); // Total nuùber of pictures in the GOP

    pc_pict->i_r = p_codec->encoder.rc.i_bitrate;
    pc_pict->i_m = p_codec->encoder.rc.i_pict_bits_hdr;
    pc_pict->i_mad = p_codec->encoder.rc.i_pict_mad;
    pc_pict->i_b = (p_codec->encoder.rc.i_pict_bits_hdr + p_codec->encoder.rc.i_pict_bits_data);

    // Update Np(i) - Number of P-Frames in the GOP at ith position.
    if (!pc_pict->u_idx > 0) {
        ++pc_gop->i_np;
    }

    // Compute Wp,i(j) - (2-71)
    pc_pict->i_wp = (pc_pict->i_b * pc_pict->i_qpp);

    // Compute Vi(j) - (2-62)
    if (pc_pict->u_idx == 0) {
        // Vi(1)
        if (pc_gop->u_idx == 0) {
            pc_pict->i_v = 0;
        }
        else {
            hl_codec_264_rc_gop_t* pc_gop_prev = p_codec->encoder.rc.p_list_gops[pc_gop->u_idx - 1];
            hl_codec_264_rc_pict_xt* pc_pict_last;
            if (!pc_gop_prev) {
                HL_DEBUG_ERROR("Previous GOP at %u is null", (pc_gop->u_idx - 1));
                return HL_ERROR_INVALID_STATE;
            }
            // last picture in the previous GOP
            pc_pict_last = pc_gop_prev->pp_list_picts[pc_gop_prev->u_list_picts_count - 1];
            if (!pc_pict_last) {
                HL_DEBUG_ERROR("Previous Pict at %u in GOP %u is null", pc_gop_prev->u_list_picts_count - 1, pc_gop_prev->u_idx);
                return HL_ERROR_INVALID_STATE;
            }
            pc_pict->i_v = pc_pict_last->i_v;
        }
    }
    else {
        hl_codec_264_rc_pict_xt* pc_pict_prev = pc_gop->pp_list_picts[pc_pict->u_idx - 1/* (j - 1) */];
        if (!pc_pict_prev) {
            HL_DEBUG_ERROR("Previous PICT at %u is null", (pc_pict->u_idx - 1));
            return HL_ERROR_INVALID_STATE;
        }
        // Vi(j)
        pc_pict->i_v = pc_pict_prev->i_v + pc_pict_prev->i_b - ((pc_pict_prev->i_r) / p_codec->encoder.rc.i_fps);
    }

    // Compute Bi(j) - (2 - 60)
    if (pc_pict->u_idx == 0) {
        pc_pict->i_B = (pc_pict->i_r / p_codec->encoder.rc.i_fps) * Ni - pc_pict->i_v;
    }
    else {
        hl_codec_264_rc_pict_xt* pc_pict_prev = pc_gop->pp_list_picts[pc_pict->u_idx - 1/* (j-1) */]; // pict a (j - 1)
        if (!pc_pict_prev) {
            HL_DEBUG_ERROR("Previous PICT at %u is null", (pc_pict->u_idx-1));
            return HL_ERROR_INVALID_STATE;
        }
        pc_pict->i_B = pc_pict_prev->i_B + ( (pc_pict->i_r - pc_pict_prev->i_r)/p_codec->encoder.rc.i_fps ) * (Ni - pc_pict->u_idx + 1) - pc_pict_prev->i_b;
    }

    // Compute Si(2)
    if (pc_pict->u_idx <= 1) {
        // (2-69)
        pc_pict->i_s = pc_pict->i_v;
    }
    else {
        hl_codec_264_rc_pict_xt* pc_pict_1 = pc_gop->pp_list_picts[1/* (2) */];
        if (!pc_pict_1) {
            HL_DEBUG_ERROR("Previous PICT at %u is null", (1));
            return HL_ERROR_INVALID_STATE;
        }
        // (2-72)
        pc_gop->i_s_next = pc_pict->i_s - (pc_pict_1->i_s / (pc_gop->i_np - 1));
    }

    {
        // Compute Ti(j)
        // FIXME: is "remaining" means what it means?
        int32_t Npr; // the number of the remaining stored pictures (remaining P-Frames to encode)
        double tmp0, tmp1;
        hl_codec_264_rc_pict_xt* pc_pict_prev;

        if (pc_pict->u_idx > 0) {
            pc_pict_prev = pc_gop->pp_list_picts[pc_pict->u_idx - 1/* (j - 1) */];
            if (!pc_pict_prev) {
                HL_DEBUG_ERROR("Previous PICT at %u is null", (pc_pict->u_idx - 1));
                return HL_ERROR_INVALID_STATE;
            }
        }
        else {
            pc_pict_prev = pc_pict;
        }

        Npr = (int32_t)(pc_gop->u_list_picts_count - (pc_pict->u_idx + 1));
        tmp0 = ((double)pc_pict->i_r / p_codec->encoder.rc.i_fps) +
               HL_CODEC_264_RC_GAMMA_NO_NONSTORED_PICTS * (pc_pict->i_s - pc_pict->i_v); // (2-73)
        tmp1 = ((double)pc_pict_prev->i_wp * pc_pict->i_B) /
               (pc_pict_prev->i_wp * Npr);
        pc_pict->i_t = (int64_t)((pc_pict->i_B * tmp1) + ((1 - HL_CODEC_264_RC_BETA_NO_NONSTORED_PICTS) * tmp0));
        pc_pict->i_mad = (int64_t)((pc_gop->d_a1 * pc_pict_prev->i_mad) + pc_gop->d_a2); // (2-79)

        // Quadratic function (2-80)
        {

        }
    }

    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_rc_gop_t" ***/
static hl_object_t* hl_codec_264_rc_gop_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_rc_gop_t *p_rc_gop = (hl_codec_264_rc_gop_t*)self;
    if (p_rc_gop) {

    }
    return self;
}
static hl_object_t* hl_codec_264_rc_gop_dtor(hl_object_t * self)
{
    hl_codec_264_rc_gop_t *p_rc_gop = (hl_codec_264_rc_gop_t*)self;
    if (p_rc_gop) {
        if (p_rc_gop->pp_list_picts) {
            hl_size_t u;
            // Must not use "HL_LIST_STATIC_SAFE_FREE_OBJECTS()" because the list doesn't contain objects.
            for (u = 0; u < p_rc_gop->u_list_picts_count; ++u) {
                if (!p_rc_gop->pp_list_picts[u]) {
                    break; // last
                }
                HL_SAFE_FREE(p_rc_gop->pp_list_picts[u]);
            }
            hl_memory_free(p_rc_gop->pp_list_picts);
        }
    }
    return self;
}
static int hl_codec_264_rc_gop_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_rc_gop_def_s = {
    sizeof(hl_codec_264_rc_gop_t),
    hl_codec_264_rc_gop_ctor,
    hl_codec_264_rc_gop_dtor,
    hl_codec_264_rc_gop_cmp,
};
const hl_object_def_t *hl_codec_264_rc_gop_def_t = &hl_codec_264_rc_gop_def_s;
#endif