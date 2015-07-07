#include "hartallo/hl_frame.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

static void hl_frame_deinit(struct hl_frame_s* p_frame);

/** Creates a new video frame.
* @param pp_frame A pointer to the newly created video frame. You must release the returned object using @ref HL_OBJECT_SAFE_FREE().
* @sa @ref hl_frame_video_alloc()
* @sa @ref hl_frame_video_fill()
* @retval Error or success code.
*/
HL_ERROR_T hl_frame_video_create(struct hl_frame_video_s** pp_frame)
{
    extern const hl_object_def_t *hl_frame_video_def_t;
    if (!pp_frame) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_frame = hl_object_create(hl_frame_video_def_t);
    if (!*pp_frame) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    HL_FRAME(*pp_frame)->type = HL_MEDIA_TYPE_VIDEO;
    (*pp_frame)->chroma = HL_VIDEO_CHROMA_YUV420;
    (*pp_frame)->encoding = HL_VIDEO_ENCODING_TYPE_AUTO;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_frame_video_alloc(struct hl_frame_video_s* p_frame, HL_VIDEO_CHROMA_T chroma, hl_size_t width, hl_size_t height)
{
    if (!p_frame || !width || !height) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    switch(chroma) {
    case HL_VIDEO_CHROMA_YUV420: {
        hl_size_t _data_size = (width * height * 3) >> 1;
        if(!(HL_FRAME(p_frame)->_priv_data_ptr = hl_memory_realloc(HL_FRAME(p_frame)->_priv_data_ptr, _data_size))) {
            HL_FRAME(p_frame)->_priv_data_size = 0;
            HL_DEBUG_ERROR("Failed to allocate %u", _data_size);
            return HL_ERROR_OUTOFMEMMORY;
        }
        HL_FRAME(p_frame)->_priv_data_size = _data_size;
        break;
    }
    default: {
        HL_DEBUG_ERROR("%d not supported as valid chroma", chroma);
        return HL_ERROR_INVALID_FORMAT;
    }
    }

    return hl_frame_video_fill(p_frame, chroma, width, height, HL_FRAME(p_frame)->_priv_data_ptr, HL_FRAME(p_frame)->_priv_data_size);
}

HL_ERROR_T hl_frame_video_fill(struct hl_frame_video_s* p_frame, HL_VIDEO_CHROMA_T chroma, hl_size_t width, hl_size_t height, const uint8_t* pc_data, hl_size_t u_data_size)
{
    if (!p_frame || !width || !height || !pc_data || !u_data_size) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    switch(chroma) {
    case HL_VIDEO_CHROMA_YUV420: {
        hl_size_t u_x_data_size = (width * height * 3) >> 1;
        if (u_x_data_size > u_data_size) {
            HL_DEBUG_ERROR("%u is too short to contains valid YUV420 data (%ux%u)", u_data_size, width, height);
            return HL_ERROR_TOOSHORT;
        }
        p_frame->data_size[0] = (width * height);
        p_frame->data_width[0] = width;
        p_frame->data_height[0] = height;
        p_frame->data_size[1] = p_frame->data_size[2] = p_frame->data_size[0] >> 2;
        p_frame->data_width[1] = p_frame->data_width[2] = p_frame->data_width[0] >> 1;
        p_frame->data_height[1] = p_frame->data_height[2] = p_frame->data_height[0] >> 1;
        p_frame->data_ptr[0] = &pc_data[0];
        p_frame->data_ptr[1] = &pc_data[p_frame->data_size[0]];
        p_frame->data_ptr[2] = &pc_data[p_frame->data_size[0] + p_frame->data_size[1]];
        break;
    }

    default: {
        HL_DEBUG_ERROR("%d not supported as valid chroma", chroma);
        return HL_ERROR_INVALID_FORMAT;
    }
    }

    p_frame->chroma = chroma;

    return HL_ERROR_SUCCESS;
}

static void hl_frame_deinit(struct hl_frame_s* p_frame)
{
    if (p_frame) {
        HL_SAFE_FREE(p_frame->_priv_data_ptr);
        p_frame->_priv_data_size = 0;
    }
}

static hl_object_t* hl_frame_video_ctor(hl_object_t * self, va_list * app)
{
    hl_frame_video_t *p_frame = (hl_frame_video_t*)self;
    if (p_frame) {

    }
    return self;
}
static hl_object_t* hl_frame_video_dtor(hl_object_t * self)
{
    hl_frame_video_t *p_frame = (hl_frame_video_t*)self;
    if (p_frame) {
        hl_frame_deinit(HL_FRAME(p_frame));
    }
    return self;
}
static int hl_frame_video_cmp(const hl_object_t *_f1, const hl_object_t *_f2)
{
    return (int)((int*)_f1 - (int*)_f2);
}
static const hl_object_def_t hl_frame_video_def_s = {
    sizeof(hl_frame_video_t),
    hl_frame_video_ctor,
    hl_frame_video_dtor,
    hl_frame_video_cmp,
};
const hl_object_def_t *hl_frame_video_def_t = &hl_frame_video_def_s;
