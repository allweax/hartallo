#include <hartallo/hl_api.h>
#include <hartallo/hl_memory.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_object.h>

#include <assert.h>

#define HL_TEST_PARSER_NAL_CHUNK_SIZE		(2048*1024*1*8)
#define HL_TEST_PARSER_INPUT_FILE_PATH		"C:\\Projects\\toulde\\videos\\AUD_MW_E\\AUD_MW_E.264"

HL_ERROR_T hl_test_parser()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint8_t* p_buffer = (uint8_t*)hl_memory_calloc(HL_TEST_PARSER_NAL_CHUNK_SIZE, sizeof(uint8_t)), *_p;
    FILE *p_file = fopen(HL_TEST_PARSER_INPUT_FILE_PATH, "rb");
    hl_size_t nal_start, nal_end, nread = 0, index = 0, count;
    const struct hl_parser_plugin_def_s* pc_plugin_parser = HL_NULL;
    struct hl_parser_s* p_inst_parser = HL_NULL;

    err = hl_parser_plugin_find(HL_CODEC_TYPE_H264_SVC, &pc_plugin_parser);
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }
    err = hl_parser_create(pc_plugin_parser, &p_inst_parser);
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

    while ((count = (hl_size_t)fread(p_buffer + nread, 1, HL_TEST_PARSER_NAL_CHUNK_SIZE - index, p_file))) {
        nread += count;
        _p = p_buffer;
        while (count && (err = hl_parser_find_bounds(p_inst_parser, _p, count, &nal_start, &nal_end)) == HL_ERROR_SUCCESS) {
            HL_DEBUG_INFO("Parser, bounds: [%u, %u], size: %u", nal_start, nal_end, (nal_end - nal_start + 1));
            _p += nal_end;
            count -= nal_end;
        }
        if (err != HL_ERROR_NOT_FOUND && err != HL_ERROR_SUCCESS) {
            break;
        }
    }

bail:
    if (p_file) {
        fclose(p_file);
    }
    HL_SAFE_FREE(p_buffer);
    HL_OBJECT_SAFE_FREE(p_inst_parser);
    return err;
}

