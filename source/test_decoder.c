#include <hartallo/hl_api.h>
#include <hartallo/hl_memory.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_object.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_time.h>
#include <hartallo/hl_codec.h>

#include <assert.h>

#define HL_TEST_PARSER_NAL_CHUNK_SIZE		(2048*1024*1*8)
#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\toulde\\videos\\AUD_MW_E\\AUD_MW_E.264"//"/Users/diopmamadou/Documents/tmp/hl78965/samples/AUD_MW_E/AUD_MW_E.264"
//#define HL_TEST_PARSER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\freh1_b\\Freh1_B.264"
//#define HL_TEST_PARSER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\SVCBS-1\\SVCBS-1\\SVCBS-1.264"
//#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\hartallo\\tests\\TestH264\\bourne.264" // 1920x816
//#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\hl78965\\clean\\hartallo\\encoder.264"
//#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\toulde\\videos\\FM1_BT_B\\FM1_BT_B.264" // Multiple slice + deblocking, MD(cpp)<>MD5(asm) beacuse of interpol clipping
//#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\tmp\\x264_104\\output.264" // x264
//#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\hl78965\\clean\\hartallo\\encoderSVC.264"
//#define HL_TEST_PARSER_INPUT_FILE_PATH			"C:\\Projects\\hl78965\\tests\\TestH264\\output.264" // Hartallo old version

//#define HL_TEST_PARSER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\BA1_FT_C\\BA1_FT_C.264" // FIXME: crash
//#define HL_TEST_PARSER_INPUT_FILE_PATH			"C:\\Projects\\toulde\\videos\\CVPCMNL1_SVA_C\\CVPCMNL1_SVA_C.264" // I_PCM

#define HL_TEST_PARSER_OUTPUT_FILE_PATH				"./decoded.yuv"

// Intel guide "14.4 INSTRUCTION LATENCY" -> for latency

HL_ERROR_T hl_test_decoder()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint8_t* p_buffer = (uint8_t*)hl_memory_calloc(HL_TEST_PARSER_NAL_CHUNK_SIZE, sizeof(uint8_t)), *_p;
    FILE *p_file = fopen(HL_TEST_PARSER_INPUT_FILE_PATH, "rb");
    FILE *p_file_out = fopen(HL_TEST_PARSER_OUTPUT_FILE_PATH, "wb+");
    hl_size_t nal_start, nal_end, nread = 0, index = 0, count;
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
    if (!p_file || !p_file_out) {
        err = HL_ERROR_NOT_FOUND;
        assert(0);
        goto bail;
    }

    p_inst_codec->threads_count = 1; // FIXME: "2" crash

    // Say which SVC layers to decode (use negative values for both "dqid_min" and "dqid_max" to decode and output all layers)
    p_inst_codec->dqid_min = HL_SVC_H264_MAKE_DQID(1, 0); // From QCIF layer
    p_inst_codec->dqid_max = HL_SVC_H264_MAKE_DQID(1, 0); // To CIF layer

    cpu_start = hl_cpu_get_cycles_count_global();
    time_start = hl_time_now();

    while ((count = (hl_size_t)fread(p_buffer + nread, 1, HL_TEST_PARSER_NAL_CHUNK_SIZE - index, p_file))) {
        nread += count;
        _p = p_buffer;
        while (count && (err = hl_parser_find_bounds(p_inst_parser, _p, count, &nal_start, &nal_end)) == HL_ERROR_SUCCESS) {
            // HL_DEBUG_INFO("Parser, bounds: [%u, %u], size=%u", nal_start, nal_end, (nal_end - nal_start + 1));
            err = hl_codec_decode(p_inst_codec, &_p[nal_start], (nal_end - nal_start + 1), p_result);
            if (p_file_out && (p_result->type & HL_CODEC_RESULT_TYPE_DATA)) {
                fwrite(p_result->data_ptr, 1, p_result->data_size, p_file_out);
            }
            if (err != HL_ERROR_SUCCESS) {
                //goto bail;
            }
            _p += nal_end;
            count -= nal_end;
        }
        //if (err != HL_ERROR_NOT_FOUND && err != HL_ERROR_SUCCESS) {
        //    break;
        // }
    }

    time_end = hl_time_now();
    cpu_end = hl_cpu_get_cycles_count_global();

    HL_DEBUG_INFO("CPU Cycles count = %llu\n", (cpu_end - cpu_start));
    HL_DEBUG_INFO("Time = %llu", (time_end - time_start));

bail:
    if (p_file) {
        fclose(p_file);
    }
    if (p_file_out) {
        fclose(p_file_out);
    }
    HL_SAFE_FREE(p_buffer);
    HL_OBJECT_SAFE_FREE(p_inst_parser);
    HL_OBJECT_SAFE_FREE(p_inst_codec);
    HL_OBJECT_SAFE_FREE(p_result);
    return err;
}

