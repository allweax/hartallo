#include <hartallo/hl_api.h>
#include <hartallo/hl_memory.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_object.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_time.h>

#include <assert.h>

#define HL_TEST_PARSER_NAL_CHUNK_SIZE		(2048*1024*1*8)

typedef struct test_file_s {
    const char* path_in_h264;
    const char* path_out_yuv;
    const char* stream_type;
    const char* purpose;
}
test_file_t;

static test_file_t test_files[] = {
    /*=== Base Profile ===*/
    /*{
    	"C:\\Projects\\toulde\\videos\\AUD_MW_E\\AUD_MW_E.264",
    	"AUD_MW_E.yuv",
    	"AVCAUD-1",
    	"Check that the decoder can properly decode I slices with Access unit delimiter NAL units"
    },
    {
    	"C:\\Projects\\toulde\\videos\\BA1_Sony_D\\BA1_Sony_D.264",
    	"BA1_Sony_D.yuv",
    	"AVCBA-1",
    	"Check that the decoder can properly decode I slices with the deblocking filter process enabled"
    },
    {
    	"C:\\Projects\\toulde\\videos\\SVA_BA1_B\\SVA_BA1_B.264",
    	"SVA_BA1_B.yuv",
    	"AVCBA-2",
    	"Check that the decoder can properly decode I slices with the deblocking filter process enabled",
    },
    { // FIXME: blur
    	"C:\\Projects\\toulde\\videos\\BA1_FT_C\\BA1_FT_C.264",
    	"BA1_FT_C.yuv",
    	"AVCBA-7",
    	"Check that the decoder can properly decode P slices with the deblocking filter process enabled",
    },
    // pic_order_cnt_type=1
    // Spatial direct prediction
    // I only
    {	// FIXME: chroma
    	"C:\\Projects\\toulde\\videos\\BAMQ1_JVC_C\\BAMQ1_JVC_C.264",
    	"BAMQ1_JVC_C.yuv",
    	"AVCMQ-3",
    	"Check that the decoder can properly decode I slices with mb_qp_delta not equal to 0"
    },
    // pic_order_cnt_type=1
    // Spatial direct prediction
    // I and P
    {
    	"C:\\Projects\\toulde\\videos\\BAMQ2_JVC_C\\BAMQ2_JVC_C.264",
    	"BAMQ2_JVC_C.yuv",
    	"AVCMQ-4",
    	"Check that the decoder can properly decode P slices with mb_qp_delta not equal to 0",
    },
    // pic_order_cnt_type =2
    // Multiple slice per picture
    {	// FIXME: blur
    	"C:\\Projects\\toulde\\videos\\SVA_Base_B\\SVA_Base_B.264",
    	"SVA_Base_B.yuv",
    	"AVCSL-1",
    	"Check that the decoder can properly decode pictures with multiple slices"
    },
    // pic_order_cnt_type =0
    // Multiple slice per picture
    {	// FIXME: blur
    	"C:\\Projects\\toulde\\videos\\SVA_FM1_E\\SVA_FM1_E.264",
    	"SVA_FM1_E.yuv",
    	"AVCSL-2",
    	"Check that the decoder can properly decode pictures with multiple slices"
    },
    // slice_qp_delta is equal to a non-zero (from slice to slice)
    // pic_order_cnt_type is equal to 0.
    // Spatial direct prediction is used for direct prediction
    // Each picture contains 20 slices.
    {
    	"C:\\Projects\\toulde\\videos\\BASQP1_Sony_C\\BASQP1_Sony_C.264",
    	"BASQP1_Sony_C.yuv",
    	"AVCSQ-1",
    	"Check that the decoder can properly decode I slices with non-zero values of slice_qp_delta"
    },*/
    // All slices are coded as I or P slices.
    //pic_order_cnt_type=0
    //The number of slices and slice groups is greater than 1 in each picture
    //Check that the decoder handles multiple slice groups and parameter sets
    {
        // FIXME: blur
        "C:\\Projects\\toulde\\videos\\FM1_BT_B\\FM1_BT_B.264",
        "FM1_BT_B.yuv",
        "AVCFM-1",
        "Check that the decoder handles multiple slice groups and parameter sets"
    },/*
	// All slices are coded as I or P slices.
	// pic_order_cnt_type=1
	// The number of slices and slice groups is greater than 1 in each picture
	{	// FIXME: fails, AVC_H264 also fails. Test with Thialgou
		"C:\\Projects\\toulde\\videos\\FM2_SVA_C\\FM2_SVA_C.264",
		"FM2_SVA_C.yuv",
		"AVCFM-2",
		"Check that the decoder handles multiple slice groups and parameter sets"
	},
	// pic_order_cnt_type=2
	// Recovery point SEI
	// Multiple slice groups and parameter sets
	{	// FIXME: /!\CRASH + BLUR
		"C:\\Projects\\toulde\\videos\\FM1_FT_E\\FM1_FT_E.264",
		"FM1_FT_E.yuv",
		"AVCFM-3",
		"Check that the decoder handles multiple slice groups and parameter sets"
	},
	// constrained_intra_pred_flag=1
	{
		"C:\\Projects\\toulde\\videos\\CI_MW_D\\CI_MW_D.264",
		"CI_MW_D.yuv",
		"AVCCI-1",
		"Check that the decoder handles constrained intra prediction"
	},
	{
		"C:\\Projects\\toulde\\videos\\CVFC1_Sony_C\\CVFC1_Sony_C.264",
		"CVFC1_Sony_C.yuv",
		"AVCFC-1",
		"Decoding of I and P slices with frame cropping"
	},
	{
		"C:\\Projects\\toulde\\videos\\MIDR_MW_D\\MIDR_MW_D.264",
		"MIDR_MW_D.yuv",
		"AVCMIDR-1",
		"Check that the decoder can properly decode I slices with more than IDR in bitstream"
	},
	{	// FIXME: Completly WRONG
		"C:\\Projects\\toulde\\videos\\NRF_MW_E\\NRF_MW_E.264",
		"NRF_MW_E.yuv",
		"AVCNRF-1",
		"Check that the decoder can properly decode I and P slices with non-reference pictures"
	},
	{
		"C:\\Projects\\toulde\\videos\\MPS_MW_A\\MPS_MW_A.264",
		"MPS_MW_A.yuv",
		"AVCMPS-1",
		"Decoding of I and P slices with multiple parameter set"
	},
	{
		"C:\\Projects\\toulde\\videos\\CVPCMNL1_SVA_C\\CVPCMNL1_SVA_C.264",
		"CVPCMNL1_SVA_C.yuv",
		"AVCPCM-1",
		"Check that the decoder can properly decode macroblocks with mb_type equal to I_PCM"
	},
	//	pic_order_cnt_type=1
	//	Reference picture list reordering and memory management control operations
	{	// FIXME: /!\ Errors in the console
		"C:\\Projects\\toulde\\videos\\MR1_BT_A\\MR1_BT_A.264",
		"MR1_BT_A.yuv",
		"AVCMR-1",
		"Check that the decoder handles reference picture list reordering and memory management control operations"
	},
	// pic_order_cnt_type=2
	// Reference picture list reordering and memory management control operat
	{	// FIXME: /!\ Errors in the console
		"C:\\Projects\\toulde\\videos\\MR2_TANDBERG_E\\MR2_TANDBERG_E.264",
		"MR2_TANDBERG_E.yuv",
		"AVCMR-2",
		"Check that the decoder handles reference picture list reordering and memory management control operations"
	}*/
};

HL_ERROR_T hl_test_conformance()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint8_t* p_buffer = (uint8_t*)hl_memory_calloc(HL_TEST_PARSER_NAL_CHUNK_SIZE, sizeof(uint8_t)), *_p;
    FILE *p_file = HL_NULL;
    hl_size_t nal_start, nal_end, nread = 0, index = 0, count, i;
    const struct hl_parser_plugin_def_s* pc_plugin_parser = HL_NULL;
    struct hl_parser_s* p_inst_parser = HL_NULL;
    const struct hl_codec_plugin_def_s* pc_plugin_codec = HL_NULL;
    struct hl_codec_s* p_inst_codec = HL_NULL;
    struct hl_codec_result_s* p_result = HL_NULL;
    uint64_t cpu_start, cpu_end, time_start, time_end;

    // Parser plugin
    err = hl_parser_plugin_find(HL_CODEC_TYPE_H264_SVC, &pc_plugin_parser);
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }
    // Parser instance
    err = hl_parser_create(pc_plugin_parser, &p_inst_parser);
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }

    // Create result object
    err = hl_codec_result_create(&p_result);
    if (err) {
        assert(0);
        goto bail;
    }

    // Codec plugin
    err = hl_codec_plugin_find(HL_CODEC_TYPE_H264_SVC, &pc_plugin_codec);
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }

    for (i = 0; i < sizeof(test_files)/sizeof(test_files[0]); ++i) {
        // openfile
        p_file = fopen(test_files[i].path_in_h264, "rb");
        if (!p_file) {
            HL_DEBUG_ERROR("Failed to open file at %s", test_files[i].path_in_h264);
            err = HL_ERROR_ACCESS_DENIED;
            goto bail;
        }

        HL_DEBUG_INFO("\n\n[%s]=== Purpose: %s ===\n", test_files[i].path_out_yuv, test_files[i].purpose);

        // Codec instance
        err = hl_codec_create(pc_plugin_codec, &p_inst_codec);
        if (err != HL_ERROR_SUCCESS) {
            assert(0);
            goto bail;
        }

        if (!p_buffer) {
            err = HL_ERROR_OUTOFMEMMORY;
            assert(0);
            goto bail;
        }
        if (!p_file) {
            err = HL_ERROR_NOT_FOUND;
            assert(0);
            goto bail;
        }

        cpu_start = hl_cpu_get_cycles_count_global();
        time_start = hl_time_now();

        while ((count = (hl_size_t)fread(p_buffer + nread, 1, HL_TEST_PARSER_NAL_CHUNK_SIZE - index, p_file))) {
            nread += count;
            _p = p_buffer;
            while (count && (err = hl_parser_find_bounds(p_inst_parser, _p, count, &nal_start, &nal_end)) == HL_ERROR_SUCCESS) {
                // HL_DEBUG_INFO("Parser, bounds: [%u, %u], size=%u", nal_start, nal_end, (nal_end - nal_start + 1));
                err = hl_codec_decode(p_inst_codec, &_p[nal_start], (nal_end - nal_start + 1), p_result);
                if (err != HL_ERROR_SUCCESS) {
                    //goto bail;
                }
                _p += nal_end;
                count -= nal_end;
            }
            if (err != HL_ERROR_NOT_FOUND && err != HL_ERROR_SUCCESS) {
                goto bail;
            }
        }
        time_end = hl_time_now();
        cpu_end = hl_cpu_get_cycles_count_global();

        HL_DEBUG_INFO("CPU Cycles count = %llu\n", (cpu_end - cpu_start));
        HL_DEBUG_INFO("Time = %llu", (time_end - time_start));

        // free local resources
        HL_OBJECT_SAFE_FREE(p_inst_codec);
        if (p_file) {
            fclose(p_file);
        }
    }

bail:
    if (p_file) {
        fclose(p_file);
    }
    HL_SAFE_FREE(p_buffer);
    HL_OBJECT_SAFE_FREE(p_inst_parser);
    HL_OBJECT_SAFE_FREE(p_inst_codec);
    HL_OBJECT_SAFE_FREE(p_result);
    return err;
}

