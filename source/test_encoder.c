#include <hartallo/hl_api.h>
#include <hartallo/hl_memory.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_object.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_time.h>
#include <hartallo/hl_frame.h>
#include <hartallo/hl_codec.h>

#include <assert.h>

// FIXME: YUV sequences: http://trace.eas.asu.edu/yuv/

#define HL_TEST_ENCODER_SVC_ENABLED				0 // Whether to enable SVC encoding

#define HL_TEST_ENCODER_OUTPUT_FILE_PATH	"./encoder.264"
#define HL_TEST_ENCODER_OUTPUT_FPS				15

#if 0 // CIF
#	define HL_TEST_ENCODER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\BA1_FT_C\\BA1_FT_C.yuv"
#	define HL_TEST_ENCODER_INPUT_IMAGE_WIDTH		352
#	define HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT		288
#else // QCIF
#	define HL_TEST_ENCODER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\AUD_MW_E\\AUD_MW_E.yuv"
#	define HL_TEST_ENCODER_INPUT_IMAGE_WIDTH		176
#	define HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT		144
#endif

#define HL_TEST_ENCODER_BUFFER_EXTRA_SIZE	40960// should be 0 but because of PCM which could have many slice headers....
uint8_t HL_TEST_ENCODER_BUFFER_YUV[ (HL_TEST_ENCODER_INPUT_IMAGE_WIDTH * HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT * 3) >> 1 ];
uint8_t HL_TEST_ENCODER_BUFFER_H264[ ((HL_TEST_ENCODER_INPUT_IMAGE_WIDTH * HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT * 3) >>1) + HL_TEST_ENCODER_BUFFER_EXTRA_SIZE];

#if HL_TEST_ENCODER_SVC_ENABLED
#	define HL_TEST_ENCODER_SVC_INPUT0_FILE_PATH		"C:\\Projects\\toulde\\videos\\SVCBS-1\\SVCBS-1\\SVCBS-1-L0.yuv"
#	define HL_TEST_ENCODER_SVC_INPUT0_IMAGE_WIDTH	176
#	define HL_TEST_ENCODER_SVC_INPUT0_IMAGE_HEIGHT	144
uint8_t HL_TEST_ENCODER_SVC_BUFFER0_YUV[ (HL_TEST_ENCODER_SVC_INPUT0_IMAGE_WIDTH * HL_TEST_ENCODER_SVC_INPUT0_IMAGE_HEIGHT * 3) >> 1 ];

#	define HL_TEST_ENCODER_SVC_INPUT1_FILE_PATH		"C:\\Projects\\toulde\\videos\\SVCBS-1\\SVCBS-1\\SVCBS-1-L1.yuv"
#	define HL_TEST_ENCODER_SVC_INPUT1_IMAGE_WIDTH	352
#	define HL_TEST_ENCODER_SVC_INPUT1_IMAGE_HEIGHT	288
uint8_t HL_TEST_ENCODER_SVC_BUFFER1_YUV[ (HL_TEST_ENCODER_SVC_INPUT1_IMAGE_WIDTH * HL_TEST_ENCODER_SVC_INPUT1_IMAGE_HEIGHT * 3) >> 1 ];
#endif /* HL_TEST_ENCODER_SVC_ENABLED */

static int8_t HL_H264_START_CODE_PREFIX[3] = { 0x00, 0x00, 0x01 };

HL_ERROR_T hl_test_encoder()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const struct hl_codec_plugin_def_s* pc_plugin_codec = HL_NULL;
    struct hl_codec_s* p_inst_codec = HL_NULL;
    struct hl_codec_result_s* p_result = HL_NULL;
    uint64_t cpu_start, cpu_end, time_start, time_end;
    FILE *p_file_in0 = HL_NULL, *p_file_in1 = HL_NULL, *p_file_out = HL_NULL;
    hl_frame_video_t *p_frame_in = HL_NULL;
    int64_t i64_best_bitrate;
    static hl_rational_t __fps = { 1, HL_TEST_ENCODER_OUTPUT_FPS };
#if HL_TEST_ENCODER_SVC_ENABLED
    static const hl_size_t __sizeof_svc_in0_buff = sizeof(HL_TEST_ENCODER_SVC_BUFFER0_YUV);
    static const hl_size_t __sizeof_svc_in1_buff = sizeof(HL_TEST_ENCODER_SVC_BUFFER1_YUV);
#else
    static const hl_size_t __sizeof_in_buff = sizeof(HL_TEST_ENCODER_BUFFER_YUV);
#endif
    int32_t i_pict_idx = 0;

    // Guess best bitrate
#if HL_TEST_ENCODER_SVC_ENABLED
    err = hl_codec_guess_best_bitrate(HL_VIDEO_MOTION_RANK_MEDIUM, HL_TEST_ENCODER_SVC_INPUT1_IMAGE_WIDTH, HL_TEST_ENCODER_SVC_INPUT1_IMAGE_HEIGHT, &__fps, &i64_best_bitrate);
#else
    err = hl_codec_guess_best_bitrate(HL_VIDEO_MOTION_RANK_MEDIUM, HL_TEST_ENCODER_INPUT_IMAGE_WIDTH, HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT, &__fps, &i64_best_bitrate);
#endif
    if (err) {
        assert(0);
        goto bail;
    }

    // Codec plugin
    err = hl_codec_plugin_find(HL_CODEC_TYPE_H264_SVC, &pc_plugin_codec);
    if (err) {
        assert(0);
        goto bail;
    }

    // Codec instance
    err = hl_codec_create(pc_plugin_codec, &p_inst_codec);
    if (err) {
        assert(0);
        goto bail;
    }

    // Create result object
    err = hl_codec_result_create(&p_result);
    if (err) {
        assert(0);
        goto bail;
    }

    // Open Input file
#if HL_TEST_ENCODER_SVC_ENABLED
    p_file_in0 = fopen(HL_TEST_ENCODER_SVC_INPUT0_FILE_PATH, "rb");
    if (!p_file_in0) {
        HL_DEBUG_ERROR("Failed to open file at %s", HL_TEST_ENCODER_SVC_INPUT0_FILE_PATH);
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }
    p_file_in1 = fopen(HL_TEST_ENCODER_SVC_INPUT1_FILE_PATH, "rb");
    if (!p_file_in1) {
        HL_DEBUG_ERROR("Failed to open file at %s", HL_TEST_ENCODER_SVC_INPUT1_FILE_PATH);
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }
#else
    p_file_in0 = fopen(HL_TEST_ENCODER_INPUT_FILE_PATH, "rb");
    if (!p_file_in0) {
        HL_DEBUG_ERROR("Failed to open file at %s", HL_TEST_ENCODER_INPUT_FILE_PATH);
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }
#endif /* HL_TEST_ENCODER_SVC_ENABLED */

    // Open Output file
    p_file_out = fopen(HL_TEST_ENCODER_OUTPUT_FILE_PATH, "wb+");
    if (!p_file_out) {
        HL_DEBUG_ERROR("Failed to open file at %s", HL_TEST_ENCODER_OUTPUT_FILE_PATH);
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }

    // Create input frame
    err = hl_frame_video_create(&p_frame_in);
    if (err) {
        goto bail;
    }

    p_inst_codec->gop_size = 400; // FIXME: doesn't work -> green
    p_inst_codec->me_range = 8; // FIXME
    p_inst_codec->qp = 31; // FIXME: Try with QP=5 and Foreman QCIF
    p_inst_codec->fps.num = 1, p_inst_codec->fps.den = HL_TEST_ENCODER_OUTPUT_FPS;
    p_inst_codec->rc_bitrate = -1/*45020*//*i64_best_bitrate*/; // FIXME
    p_inst_codec->deblock_flag = 0; // FIXME: not working with multithreaded
    p_inst_codec->threads_count = 2-1; // FIXME
    p_inst_codec->distortion_mesure_type = HL_VIDEO_DISTORTION_MESURE_TYPE_SAD; // FIXME
    p_inst_codec->me_type = (HL_VIDEO_ME_TYPE_INTEGER | HL_VIDEO_ME_TYPE_HALF | HL_VIDEO_ME_TYPE_QUATER); // FIXME
    p_inst_codec->me_part_types = HL_VIDEO_ME_PART_TYPE_ALL;
    p_inst_codec->me_subpart_types = HL_VIDEO_ME_SUBPART_TYPE_ALL;
    p_inst_codec->me_early_term_flag = 0;

    cpu_start = hl_cpu_get_cycles_count_global();
    time_start = hl_time_now();

#if HL_TEST_ENCODER_SVC_ENABLED
    /* Turn codec from H.264 "AVC" to H.264 "SVC".
    * Layers must be in increasing order
    */
    err = hl_codec_add_layer(p_inst_codec, HL_TEST_ENCODER_SVC_INPUT0_IMAGE_WIDTH, HL_TEST_ENCODER_SVC_INPUT0_IMAGE_HEIGHT, 0, 0);
    if (err) {
        goto bail;
    }
    err = hl_codec_add_layer(p_inst_codec, HL_TEST_ENCODER_SVC_INPUT1_IMAGE_WIDTH, HL_TEST_ENCODER_SVC_INPUT1_IMAGE_HEIGHT, 0, 0);
    if (err) {
        goto bail;
    }
#endif /* HL_TEST_ENCODER_SVC_ENABLED */

    // Read from input file and encode
#if HL_TEST_ENCODER_SVC_ENABLED
    while (fread(HL_TEST_ENCODER_SVC_BUFFER0_YUV, 1, __sizeof_svc_in0_buff, p_file_in0) == __sizeof_svc_in0_buff && fread(HL_TEST_ENCODER_SVC_BUFFER1_YUV, 1, __sizeof_svc_in1_buff, p_file_in1) == __sizeof_svc_in1_buff) {
#else
    while (fread(HL_TEST_ENCODER_BUFFER_YUV, 1, __sizeof_in_buff, p_file_in0) == __sizeof_in_buff) {
#endif /* HL_TEST_ENCODER_SVC_ENABLED */
#if HL_TEST_ENCODER_SVC_ENABLED
        p_frame_in->encoding = HL_VIDEO_ENCODING_TYPE_AUTO; // FIXME

        // Encode first layer
        err = hl_frame_video_fill(p_frame_in,
                                  HL_VIDEO_CHROMA_YUV420,
                                  HL_TEST_ENCODER_SVC_INPUT0_IMAGE_WIDTH,
                                  HL_TEST_ENCODER_SVC_INPUT0_IMAGE_HEIGHT,
                                  HL_TEST_ENCODER_SVC_BUFFER0_YUV,
                                  __sizeof_svc_in0_buff);
        if (err) {
            goto bail;
        }
        err = hl_codec_encode(p_inst_codec, (hl_frame_t*)p_frame_in, p_result);
        if (err) {
            goto bail;
        }

        // Encode second layer
        err = hl_frame_video_fill(p_frame_in,
                                  HL_VIDEO_CHROMA_YUV420,
                                  HL_TEST_ENCODER_SVC_INPUT1_IMAGE_WIDTH,
                                  HL_TEST_ENCODER_SVC_INPUT1_IMAGE_HEIGHT,
                                  HL_TEST_ENCODER_SVC_BUFFER1_YUV,
                                  __sizeof_svc_in1_buff);
        if (err) {
            goto bail;
        }
        err = hl_codec_encode(p_inst_codec, (hl_frame_t*)p_frame_in, p_result);
        if (err) {
            goto bail;
        }
#else
        // fill YUV data into the frame
        err = hl_frame_video_fill(p_frame_in,
                                  HL_VIDEO_CHROMA_YUV420,
                                  HL_TEST_ENCODER_INPUT_IMAGE_WIDTH,
                                  HL_TEST_ENCODER_INPUT_IMAGE_HEIGHT,
                                  HL_TEST_ENCODER_BUFFER_YUV,
                                  __sizeof_in_buff);
        if (err) {
            goto bail;
        }
        // encode frame
        p_frame_in->encoding = HL_VIDEO_ENCODING_TYPE_AUTO; // FIXME
        err = hl_codec_encode(p_inst_codec, (hl_frame_t*)p_frame_in, p_result);
        if (err) {
            goto bail;
        }
#endif /* HL_TEST_ENCODER_SVC_ENABLED */

        // Copy headers (SPS, PPS...) to the output file
        if ((p_result->type & HL_CODEC_RESULT_TYPE_HDR)) {
            fwrite(p_inst_codec->hdr_bytes, 1, p_inst_codec->hdr_bytes_count, p_file_out); // headers alread contains SCP
        }
        // Copy output data to the file
        if (p_result->type & HL_CODEC_RESULT_TYPE_DATA) {
            fwrite(HL_H264_START_CODE_PREFIX, 1, sizeof(HL_H264_START_CODE_PREFIX), p_file_out); // SCP
            fwrite(p_result->data_ptr, 1, p_result->data_size, p_file_out);
            ++i_pict_idx;
            if (i_pict_idx == 10/*300*/) {
                break;
            }
        }
    }

    time_end = hl_time_now();
    cpu_end = hl_cpu_get_cycles_count_global();

    HL_DEBUG_INFO("CPU Cycles count = %llu\n", (cpu_end - cpu_start));
    HL_DEBUG_INFO("Encoded %d frames in %llu millis --> Speed=%.2f frames/sec.",
                  i_pict_idx,
                  (time_end - time_start),
                  ((float)i_pict_idx / (((float)time_end - (float)time_start) / (float)1000)));

bail:
    if (p_file_in0) {
        fclose(p_file_in0);
    }
    if (p_file_in1) {
        fclose(p_file_in1);
    }
    if (p_file_out) {
        fclose(p_file_out);
    }
    HL_OBJECT_SAFE_FREE(p_inst_codec);
    HL_OBJECT_SAFE_FREE(p_result);
    HL_OBJECT_SAFE_FREE(p_frame_in);
    return err;
}