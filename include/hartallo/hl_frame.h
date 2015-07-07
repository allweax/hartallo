#ifndef _HARTALLO_FRAME_H_
#define _HARTALLO_FRAME_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

/**@file hl_frame.h */

HARTALLO_API HL_ERROR_T hl_frame_video_create(struct hl_frame_video_s** pp_frame);
HARTALLO_API HL_ERROR_T hl_frame_video_alloc(struct hl_frame_video_s* p_frame, HL_VIDEO_CHROMA_T chroma, hl_size_t width, hl_size_t height);
HARTALLO_API HL_ERROR_T hl_frame_video_fill(struct hl_frame_video_s* p_frame, HL_VIDEO_CHROMA_T chroma, hl_size_t width, hl_size_t height, const uint8_t* pc_data, hl_size_t u_data_size);

typedef struct hl_frame_s {
    HL_DECLARE_OBJECT;

    HL_MEDIA_TYPE_T type;

    void* _priv_data_ptr; //!\ Private data. You must not use this filed.
    hl_size_t _priv_data_size;//!\ Private data. You must not use this filed.
}
hl_frame_t;
#define HL_DECLARE_FRAME hl_frame_t __base_frame__
#define HL_FRAME(self) ((hl_frame_t*)(self))

typedef struct hl_frame_video_s {
    HL_DECLARE_FRAME;

    const uint8_t* data_ptr[4]; // Y,U,V or R,G,G,A or ....
    hl_size_t data_size[4];
    hl_size_t data_width[4];
    hl_size_t data_height[4];

    /** The video chroma type. Should not be changed. Use @ref hl_frame_video_alloc() or @ref hl_frame_video_fill() to set this value. */
    HL_VIDEO_CHROMA_T chroma;
    /** The encoding type for the current frame being encoded. Could be changed at any time. */
    HL_VIDEO_ENCODING_TYPE_T encoding;
}
hl_frame_video_t;

HL_END_DECLS

#endif /* _HARTALLO_FRAME_H_ */
