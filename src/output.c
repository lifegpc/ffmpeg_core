#include "output.h"

#include "speed.h"
#include "ch_layout.h"
#if HAVE_WASAPI
#include "position_data.h"
#define ADD_POSITION_DATA(len, have_data) if (handle->is_use_wasapi && wasapi_get_object) add_data_to_position_data(handle, len, have_data);
#else
#define ADD_POSITION_DATA(len, have_data)
#endif
#include "mixing.h"

#include "time_util.h"

int init_output(MusicHandle* handle, const char* device) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    if (!handle->sdl_initialized) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            return FFMPEG_CORE_ERR_SDL;
        }
        handle->sdl_initialized = 1;
    }
    SDL_AudioSpec sdl_spec;
    sdl_spec.freq = handle->decoder->sample_rate;
    sdl_spec.format = convert_to_sdl_format(handle->decoder->sample_fmt);
    if (!sdl_spec.format) {
        const char* tmp = av_get_sample_fmt_name(handle->decoder->sample_fmt);
        av_log(NULL, AV_LOG_FATAL, "Unknown sample format: %s (%i)\n", tmp ? tmp : "", handle->decoder->sample_fmt);
        return FFMPEG_CORE_ERR_UNKNOWN_SAMPLE_FMT;
    }
    sdl_spec.channels = GET_AV_CODEC_CHANNELS(handle->decoder);
    sdl_spec.samples = handle->decoder->sample_rate / 100;
    sdl_spec.callback = SDL_callback;
    sdl_spec.userdata = handle;
    memcpy(&handle->sdl_spec, &sdl_spec, sizeof(SDL_AudioSpec));
    handle->device_id = SDL_OpenAudioDevice(device, 0, &sdl_spec, &handle->sdl_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!handle->device_id) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open audio device \"%s\": %s\n", "default", SDL_GetError());
        return FFMPEG_CORE_ERR_SDL;
    }
    enum AVSampleFormat target_format = convert_to_sdl_supported_format(handle->decoder->sample_fmt);
    int re = 0;
#if NEW_CHANNEL_LAYOUT
    if (re = get_sdl_channel_layout(handle->decoder->ch_layout.nb_channels, &handle->output_channel_layout)) {
        return re;
    }
    if (re = swr_alloc_set_opts2(&handle->swrac, &handle->output_channel_layout, target_format, handle->sdl_spec.freq, &handle->decoder->ch_layout, handle->decoder->sample_fmt, handle->decoder->sample_rate, 0, NULL)) {
        return re;
    }
#else
    handle->output_channel_layout = get_sdl_channel_layout(handle->decoder->channels);
    handle->swrac = swr_alloc_set_opts(NULL, handle->output_channel_layout, target_format, handle->sdl_spec.freq, handle->decoder->channel_layout, handle->decoder->sample_fmt, handle->decoder->sample_rate, 0, NULL);
#endif
    if (!handle->swrac) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate resample context.\n");
        return FFMPEG_CORE_ERR_OOM;
    }
    set_mixing_opts(handle);
    if ((re = swr_init(handle->swrac)) < 0) {
        return re;
    }
    if (!(handle->buffer = av_audio_fifo_alloc(target_format, GET_AV_CODEC_CHANNELS(handle->decoder), 1))) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate buffer.\n");
        return FFMPEG_CORE_ERR_OOM;
    }
    handle->target_format = target_format;
    handle->target_format_pbytes = av_get_bytes_per_sample(target_format);
    return FFMPEG_CORE_ERR_OK;
}

enum AVSampleFormat convert_to_sdl_supported_format(enum AVSampleFormat fmt) {
    switch (fmt) {
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_FLTP:
        case AV_SAMPLE_FMT_DBLP:
            return AV_SAMPLE_FMT_FLT;
        case AV_SAMPLE_FMT_U8P:
            return AV_SAMPLE_FMT_U8;
        case AV_SAMPLE_FMT_S16P:
            return AV_SAMPLE_FMT_S16;
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_S64:
        case AV_SAMPLE_FMT_S64P:
            return AV_SAMPLE_FMT_S32;
        default:
            return fmt;
    }
}

SDL_AudioFormat convert_to_sdl_format(enum AVSampleFormat fmt) {
    fmt = convert_to_sdl_supported_format(fmt);
    switch (fmt) {
        case AV_SAMPLE_FMT_U8:
            return AUDIO_U8;
        case AV_SAMPLE_FMT_S16:
            return AUDIO_S16SYS;
        case AV_SAMPLE_FMT_S32:
            return AUDIO_S32SYS;
        case AV_SAMPLE_FMT_FLT:
            return AUDIO_F32SYS;
        default:
            return 0;
    }
}

void SDL_callback(void* userdata, uint8_t* stream, int len) {
    MusicHandle* handle = (MusicHandle*)userdata;
#if HAVE_WASAPI
    /// 是否获取到了Mutex3所有权
    char wasapi_get_object = 0;
    if (handle->is_use_wasapi) {
        DWORD tmp = WaitForSingleObject(handle->mutex3, 1);
        if (tmp == WAIT_OBJECT_0) {
            wasapi_get_object = 1;
        }
    }
#endif
#if _WIN32
    DWORD re = WaitForSingleObject(handle->mutex, handle->s->max_wait_buffer_time);
#else
    struct timespec ts;
    size_t now = time_time_ns();
    now += handle->s->max_wait_buffer_time * 1000000;
    ts.tv_sec = now / 1000000000;
    ts.tv_nsec = now % 1000000000;
    int re = pthread_mutex_timedlock(&handle->mutex, &ts);
#endif
    if (re != WAIT_OBJECT_0) {
        // 无法获取Mutex所有权，填充空白数据
        memset(stream, 0, len);
        ADD_POSITION_DATA(len, 0)
#if HAVE_WASAPI
        if (wasapi_get_object) ReleaseMutex(handle->mutex3);
#endif
        return;
    }
    int samples_need = len / handle->target_format_pbytes / handle->sdl_spec.channels;
    int buffer_size = handle->sdl_spec.freq / 5;
    if (av_audio_fifo_size(handle->buffer) == 0) {
        // 缓冲区没有数据，填充空白数据
        memset(stream, 0, len);
        ADD_POSITION_DATA(len, 0)
    } else if (!handle->graph) {
        int writed = av_audio_fifo_read(handle->buffer, (void**)&stream, samples_need);
        if (writed > 0) {
            // 增大缓冲区开始时间
            AVRational base = { 1, handle->sdl_spec.freq };
            handle->pts += av_rescale_q_rnd(writed, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        }
        if (writed < 0) {
            // 读取发生错误，填充空白数据
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
        } else if (writed < samples_need) {
            size_t len = ((size_t)samples_need - writed) * handle->target_format_pbytes, alen = (size_t)writed * handle->target_format_pbytes;
            // 不足的区域用空白数据填充
            memset(stream + alen, 0, len);
            ADD_POSITION_DATA(alen, 1)
            ADD_POSITION_DATA(len, 0)
        } else {
            ADD_POSITION_DATA(len, 1)
        }
    } else if (handle->is_easy_filters) {
        AVFrame* in = av_frame_alloc(), * out = av_frame_alloc();
        int writed = 0;
        int samples_need_in = 0;
        if (!in || !out) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
        samples_need_in = samples_need * get_speed(handle->s->speed) / 1000;
#if NEW_CHANNEL_LAYOUT
        if (av_channel_layout_copy(&in->ch_layout, &handle->output_channel_layout) < 0) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
#else
        in->channels = handle->sdl_spec.channels;
        in->channel_layout = handle->output_channel_layout;
#endif
        in->format = handle->target_format;
        in->sample_rate = handle->sdl_spec.freq;
        in->nb_samples = samples_need_in;
        if (av_frame_get_buffer(in, 0) < 0) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
        // 从缓冲区读取数据
        writed = av_audio_fifo_read(handle->buffer, (void**)in->data, samples_need_in);
        if (writed > 0) {
            // 增大缓冲区开始时间
            AVRational base = { 1, handle->sdl_spec.freq };
            handle->pts += av_rescale_q_rnd(writed, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        }
        if (writed < 0) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
        in->nb_samples = writed;
        // 喂给 filters 数据
        if (av_buffersrc_add_frame(handle->filter_inp, in) < 0) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
        // 从 filters 拿回数据
        if (av_buffersink_get_frame(handle->filter_out, out) < 0) {
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
            goto end;
        }
        if (out->nb_samples >= samples_need) {
            memcpy(stream, out->data[0], len);
            ADD_POSITION_DATA(len, 1)
        } else {
            size_t le = (size_t)out->nb_samples * handle->target_format_pbytes * handle->sdl_spec.channels;
            memcpy(stream, out->data[0], le);
            ADD_POSITION_DATA(le, 1)
            memset(stream, 0, len - le);
            ADD_POSITION_DATA(len - le, 0)
        }
end:
        if (in) av_frame_free(&in);
        if (out) av_frame_free(&out);
    } else if (!handle->is_wait_filters || av_audio_fifo_size(handle->filters_buffer) > buffer_size) {
        handle->is_wait_filters = 0;
        int writed = av_audio_fifo_read(handle->filters_buffer, (void**)&stream, samples_need);
        if (writed > 0) {
            // 增大缓冲区开始时间
            AVRational base = { 1, handle->sdl_spec.freq }, base2 = { get_speed(handle->s->speed), 1000 }, tar = { 1, 1 };
            int samples_in = av_rescale_q_rnd(writed, base2, tar, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            handle->filters_buffer_offset -= samples_in;
            handle->pts += av_rescale_q_rnd(samples_in, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            av_audio_fifo_drain(handle->buffer, samples_in);
        }
        if (writed < 0) {
            // 读取发生错误，填充空白数据
            memset(stream, 0, len);
            ADD_POSITION_DATA(len, 0)
        } else if (writed < samples_need) {
            size_t alen = (size_t)writed * handle->target_format_pbytes, len = ((size_t)samples_need - writed) * handle->target_format_pbytes;
            // 不足的区域用空白数据填充
            memset(stream + alen, 0, len);
            ADD_POSITION_DATA(alen, 1)
            ADD_POSITION_DATA(len, 0)
        } else {
            ADD_POSITION_DATA(len, 1)
        }
    } else {
        memset(stream, 0, len);
        ADD_POSITION_DATA(len, 0)
    }
    ReleaseMutex(handle->mutex);
#if HAVE_WASAPI
    if (wasapi_get_object) {
        ReleaseMutex(handle->mutex3);
    }
#endif
}

#if NEW_CHANNEL_LAYOUT
int get_sdl_channel_layout(int channels, AVChannelLayout* channel_layout) {
    if (!channel_layout) return FFMPEG_CORE_ERR_NULLPTR;
    switch (channels) {
        case 2:
            return av_channel_layout_from_string(channel_layout, "FL+FR");
        case 3:
            return av_channel_layout_from_string(channel_layout, "FL+FR+LFE");
        case 4:
            return av_channel_layout_from_string(channel_layout, "FL+FR+BL+BR");
        case 5:
            return av_channel_layout_from_string(channel_layout, "FL+FR+FC+BL+BR");
        case 6:
            return av_channel_layout_from_string(channel_layout, "FL+FR+FC+LFE+SL+SR");
        case 7:
            return av_channel_layout_from_string(channel_layout, "FL+FR+FC+LFE+BC+SL+SR");
        case 8:
            return av_channel_layout_from_string(channel_layout, "FL+FR+FC+LFE+BL+BR+SL+SR");
        default:
            av_channel_layout_default(channel_layout, channels);
            return FFMPEG_CORE_ERR_OK;
    }
}
#endif

#if OLD_CHANNEL_LAYOUT
uint64_t get_sdl_channel_layout(int channels) {
    switch (channels) {
        case 2:
            return av_get_channel_layout("FL+FR");
        case 3:
            return av_get_channel_layout("FL+FR+LFE");
        case 4:
            return av_get_channel_layout("FL+FR+BL+BR");
        case 5:
            return av_get_channel_layout("FL+FR+FC+BL+BR");
        case 6:
            return av_get_channel_layout("FL+FR+FC+LFE+SL+SR");
        case 7:
            return av_get_channel_layout("FL+FR+FC+LFE+BC+SL+SR");
        case 8:
            return av_get_channel_layout("FL+FR+FC+LFE+BL+BR+SL+SR");
        default:
            return av_get_default_channel_layout(channels);
    }
}
#endif
