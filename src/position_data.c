#include "position_data.h"

#include "wasapi.h"

int64_t cal_true_buffer_time(PositionDataList* list, int64_t buffered) {
    if (!list) return 0;
    PositionDataList* t = position_data_list_tail(list);
    if (!t) return 0;
    int64_t result = 0;
    if (t->have_data) {
        result += min(buffered, t->length);
    }
    buffered -= t->length;
    if (buffered <= 0) return result;
    while (t->prev) {
        t = t->prev;
        if (t->have_data) {
            result += min(buffered, t->length);
        }
        buffered -= t->length;
        if (buffered <= 0) return result;
    }
    return result;
}

int add_data_to_position_data(MusicHandle* handle, int size, char have_data) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    if (!handle->is_use_wasapi || size == 0) return FFMPEG_CORE_ERR_OK;
    AVRational base = { 1, handle->sdl_spec.freq };
    int64_t len = av_rescale_q_rnd(size / handle->target_format_pbytes / handle->sdl_spec.channels, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    if (!position_data_list_append(&handle->position_data, len, have_data)) {
        return FFMPEG_CORE_ERR_OOM;
    }
    int64_t clen = 0;
    PositionDataList* cur = position_data_list_tail(handle->position_data);
    if (cur) {
        clen += cur->length;
    }
    if (clen > handle->wasapi->frame_pts) {
        position_data_remove_before(cur);
        handle->position_data = cur;
        return FFMPEG_CORE_ERR_OK;
    }
    while (cur && cur->prev) {
        cur = cur->prev;
        clen += cur->length;
        if (clen > handle->wasapi->frame_pts) {
            position_data_remove_before(cur);
            handle->position_data = cur;
            return FFMPEG_CORE_ERR_OK;
        }
    }
    return FFMPEG_CORE_ERR_OK;
}

int64_t cal_true_pts(MusicHandle* handle) {
    if (!handle) return -1;
    if (!handle->is_use_wasapi) return handle->pts;
    DWORD re = WaitForSingleObject(handle->mutex3, 10);
    if (re == WAIT_OBJECT_0) {
        uint32_t frame_count;
        int64_t re = 0;
        if (SUCCEEDED(handle->wasapi->client->lpVtbl->GetCurrentPadding(handle->wasapi->client, &frame_count))) {
            AVRational base = { 1, handle->sdl_spec.freq };
            re = cal_true_buffer_time(handle->position_data, av_rescale_q_rnd(handle->wasapi->frame_count - frame_count, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        }
        ReleaseMutex(handle->mutex3);
        return handle->pts - re;
    }
    return handle->pts;
}
