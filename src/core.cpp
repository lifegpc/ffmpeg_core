#include "core.h"

#include <malloc.h>
#include <string.h>
#include "cpp2c.h"
#include "wchar_util.h"
#include "open.h"
#include "output.h"
#include "loop.h"
#include "decode.h"
#include "filter.h"
#include "speed.h"
#include "cda.h"
#include "fileop.h"
#include "file.h"
#include "equalizer_settings.h"
#include "linked_list.h"
#include "cstr_util.h"
#include "ffmpeg_core_version.h"
#include "ch_layout.h"
#include "time_util.h"

#if HAVE_WASAPI
#include "wasapi.h"
#include "position_data.h"
#endif

#define CODEPAGE_SIZE 3
#define DEVICE_NAME_LIST struct LinkedList<char*>*

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

template <typename T>
void tfree(T* data) {
    free((void*)data);
}

void free_music_handle(MusicHandle* handle) {
    if (!handle) return;
    if (handle->device_id) SDL_CloseAudioDevice(handle->device_id);
#if HAVE_WASAPI
    if (handle->wasapi) close_WASAPI_device(handle);
#endif
    if (handle->thread) {
#if _WIN32
        DWORD status;
        while (GetExitCodeThread(handle->thread, &status)) {
            if (status == STILL_ACTIVE) {
                status = 0;
                handle->stoping = 1;
                Sleep(10);
            } else {
                break;
            }
        }
#else
        int status = pthread_tryjoin_np(handle->thread, nullptr);
        if (status == EBUSY) {
            handle->stoping = 1;
            pthread_join(handle->thread, nullptr);
        }
#endif
    }
    if (handle->filter_thread) {
#if _WIN32
        DWORD status;
        while (GetExitCodeThread(handle->filter_thread, &status)) {
            if (status == STILL_ACTIVE) {
                status = 0;
                handle->stoping = 1;
                Sleep(10);
            } else {
                break;
            }
        }
#else
        int status = pthread_tryjoin_np(handle->filter_thread, nullptr);
        if (status == EBUSY) {
            handle->stoping = 1;
            pthread_join(handle->filter_thread, nullptr);
        }
#endif
    }
    if (handle->graph) {
        avfilter_graph_free(&handle->graph);
    }
    c_linked_list_clear(&handle->filters, nullptr);
    if (handle->buffer) av_audio_fifo_free(handle->buffer);
    if (handle->filters_buffer) av_audio_fifo_free(handle->filters_buffer);
    if (handle->swrac) swr_free(&handle->swrac);
    if (handle->decoder) avcodec_free_context(&handle->decoder);
    if (handle->fmt) avformat_close_input(&handle->fmt);
    if (handle->sdl_initialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
#if NEW_CHANNEL_LAYOUT
    if (handle->output_channel_layout.nb_channels != 0) {
        av_channel_layout_uninit(&handle->output_channel_layout);
    }
#endif
#if HAVE_WASAPI
    if (handle->wasapi_initialized) {
        uninit_WASAPI();
    }
    position_data_list_clear(&handle->position_data);
#endif
    if (handle->s && handle->settings_is_alloc) {
        free_ffmpeg_core_settings(handle->s);
    }
    if (handle->cda) free(handle->cda);
    if (handle->url) free(handle->url);
    if (handle->parsed_url) free_url_parse_result(handle->parsed_url);
#if _WIN32
    if (handle->mutex) CloseHandle(handle->mutex);
    if (handle->mutex2) CloseHandle(handle->mutex2);
#else
    pthread_mutex_destroy(&handle->mutex);
    pthread_mutex_destroy(&handle->mutex2);
#endif
#if HAVE_WASAPI
    if (handle->mutex3) CloseHandle(handle->mutex3);
#endif
    free(handle);
}

void free_music_info_handle(MusicInfoHandle* handle) {
    if (!handle) return;
    if (handle->fmt) avformat_close_input(&handle->fmt);
    if (handle->cda) free(handle->cda);
    free(handle);
}

void free_ffmpeg_core_settings(FfmpegCoreSettings* s) {
    if (!s) return;
    free_equalizer_channels(&s->equalizer_channels);
    free(s);
}

void free_device_name_list(DeviceNameList** list) {
    if (list) {
        auto l = (DEVICE_NAME_LIST)(*list);
        linked_list_clear(l, tfree);
        *list = (DeviceNameList*)l;
    }
}

void ffmpeg_core_free(void* data) {
    free(data);
}

void* ffmpeg_core_malloc(size_t size) {
    return malloc(size);
}

void* ffmpeg_core_realloc(void* data, size_t size) {
    return realloc(data, size);
}

int ffmpeg_core_log_format_line(void* ptr, int level, const char* fmt, va_list vl, char* line, int line_size, int* print_prefix) {
    return av_log_format_line2(ptr, level, fmt, vl, line, line_size, print_prefix);
}

void ffmpeg_core_log_set_callback(void(*callback)(void*, int, const char*, va_list)) {
    av_log_set_callback(callback);
}

void ffmpeg_core_log_set_flags(int arg) {
    av_log_set_flags(arg);
}

const char* ffmpeg_core_version_str() {
    return FFMPEG_CORE_VERSION;
}

int32_t ffmpeg_core_version() {
    return FFMPEG_CORE_VERSION_INT;
}

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#if _MSC_VER
#define PRINTF(f, ...) use_av_log ? av_log(NULL, av_log_level, f, __VA_ARGS__) : printf(f, __VA_ARGS__)
#else
#define PRINTF(f, ...) if (use_av_log) { av_log(NULL, av_log_level, f, ##__VA_ARGS__); } else { printf(f, ##__VA_ARGS__); }
#endif


void ffmpeg_core_dump_library_version(int use_av_log, int av_log_level) {
    unsigned int v = avutil_version();
    PRINTF("FFMPEG libraries: \n");
    PRINTF("libavutil     %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avcodec_version();
    PRINTF("libavcodec    %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avformat_version();
    PRINTF("libavformat   %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avdevice_version();
    PRINTF("libavdevice   %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avfilter_version();
    PRINTF("libavfilter   %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = swresample_version();
    PRINTF("libswresample %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    PRINTF("Other thirdparty libraries: \n");
    SDL_version sv;
    SDL_GetVersion(&sv);
    PRINTF("SDL           %3u.%3u.%3u\n", sv.major, sv.minor, sv.patch);
}

#define QUICK_PRINT_CONF(f, name) if (basic != f()) { \
PRINTF(name " have different configuration: %s\n", f()); }

void ffmpeg_core_dump_ffmpeg_configuration(int use_av_log, int av_log_level) {
    std::string basic = avutil_configuration();
    PRINTF("configuration: %s\n", basic.c_str());
    QUICK_PRINT_CONF(avcodec_configuration, "libavcodec")
    QUICK_PRINT_CONF(avformat_configuration, "libavformat")
    QUICK_PRINT_CONF(avdevice_configuration, "libavdevice")
    QUICK_PRINT_CONF(avfilter_configuration, "libavfilter")
    QUICK_PRINT_CONF(swresample_configuration, "libswresample")
}

#undef QUICK_PRINT_CONF
#undef PRINTF

int ffmpeg_core_open(const CORE_CHAR* url, MusicHandle** handle) {
    return ffmpeg_core_open3(url, handle, nullptr, nullptr);
}

int ffmpeg_core_open2(const CORE_CHAR* url, MusicHandle** h, FfmpegCoreSettings* s) {
    return ffmpeg_core_open3(url, h, s, nullptr);
}

int ffmpeg_core_open3(const CORE_CHAR* url, MusicHandle** h, FfmpegCoreSettings* s, const CORE_CHAR* device) {
    if (!url || !h) return FFMPEG_CORE_ERR_NULLPTR;
    std::string u;
#if _WIN32
    // 将文件名转为UTF-8，ffmpeg API处理的都是UTF-8文件名
    if (!wchar_util::wstr_to_str(u, url, CP_UTF8)) {
        return FFMPEG_CORE_ERR_INVAILD_NAME;
    }
#else
    u = url;
#endif
    std::string d;
#if _WIN32
    if (device && !wchar_util::wstr_to_str(d, device, CP_UTF8)) {
        return FFMPEG_CORE_ERR_INVAILD_NAME;
    }
#else
    if (device) d = device;
#endif
#if NDEBUG
    // 设置ffmpeg日志级别为Error
    av_log_set_level(AV_LOG_ERROR);
#else
    av_log_set_level(AV_LOG_VERBOSE);
#endif
    MusicHandle* handle = (MusicHandle*)malloc(sizeof(MusicHandle));
    int re = FFMPEG_CORE_ERR_OK;
    if (!handle) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate MusicHandle.\n");
        return FFMPEG_CORE_ERR_OOM;
    }
    memset(handle, 0, sizeof(MusicHandle));
#if !_WIN32
    pthread_mutex_init(&handle->mutex, nullptr);
    pthread_mutex_init(&handle->mutex2, nullptr);
#endif
    if (s) {
        handle->s = s;
    } else {
        handle->settings_is_alloc = 1;
        handle->s = ffmpeg_core_init_settings();
        if (!handle->s) {
            av_log(NULL, AV_LOG_FATAL, "Failed to allocate settings struct.\n");
            re = FFMPEG_CORE_ERR_OOM;
            goto end;
        }
    }
    handle->first_pts = INT64_MIN;
    handle->part_end_pts = INT64_MAX;
#if HAVE_WASAPI
    if (handle->s->use_wasapi) {
        handle->is_use_wasapi = 1;
    }
    if (handle->s->enable_exclusive) {
        handle->is_exclusive = 1;
    }
#endif
    handle->is_cda = is_cda_file(u.c_str());
    if (handle->is_cda) {
        if ((re = read_cda_file(handle, u.c_str()))) {
            goto end;
        }
        u = fileop::dirname(u);
        if ((re = open_cd_device(handle, u.c_str()))) {
            goto end;
        }
    } else {
        if ((re = open_input(handle, u.c_str()))) {
            goto end;
        }
    }
#ifndef NDEBUG
    av_dump_format(handle->fmt, 0, u.c_str(), 0);
#endif
    if (!cpp2c::string2char(u, handle->url)) {
        re = FFMPEG_CORE_ERR_OOM;
        goto end;
    }
    handle->parsed_url = urlparse(handle->url, nullptr, 1);
    if (!handle->parsed_url) {
        re = FFMEPG_CORE_ERR_FAILED_PARSE_URL;
        goto end;
    }
    handle->is_file = is_file(handle->parsed_url);
    if ((re = find_audio_stream(handle))) {
        av_log(NULL, AV_LOG_FATAL, "Failed to find suitable audio stream.\n");
        goto end;
    }
    if ((re = open_decoder(handle))) {
        goto end;
    }
#if HAVE_WASAPI
    if (handle->is_use_wasapi) {
        if ((re = init_wasapi_output(handle, d.empty() ? nullptr : d.c_str()))) {
            goto end;
        }
    } else {
        if ((re = init_output(handle, d.empty() ? nullptr : d.c_str()))) {
            goto end;
        }
    }
#else
    if ((re = init_output(handle, d.empty() ? nullptr : d.c_str()))) {
        goto end;
    }
#endif
    if ((re = init_filters(handle))) {
        goto end;
    }
    handle->filters_buffer = av_audio_fifo_alloc(handle->target_format, handle->sdl_spec.channels, 1);
    if (!handle->filters_buffer) {
        re = FFMPEG_CORE_ERR_OOM;
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate buffer for filters.\n");
        goto end;
    }
#if _WIN32
    handle->mutex = CreateMutexW(nullptr, FALSE, nullptr);
    if (!handle->mutex) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_MUTEX;
        goto end;
    }
    handle->mutex2 = CreateMutexW(nullptr, FALSE, nullptr);
    if (!handle->mutex2) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_MUTEX;
        goto end;
    }
    handle->thread = CreateThread(nullptr, 0, event_loop, handle, 0, &handle->thread_id);
    if (!handle->thread) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_THREAD;
        goto end;
    }
    handle->filter_thread = CreateThread(nullptr, 0, filter_loop, handle, 0, &handle->filter_thread_id);
    if (!handle->filter_thread) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_THREAD;
        goto end;
    }
#else
    if (pthread_create(&handle->thread, nullptr, event_loop, handle)) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_THREAD;
        goto end;
    }
    if (pthread_create(&handle->filter_thread, nullptr, filter_loop, handle)) {
        re = FFMPEG_CORE_ERR_FAILED_CREATE_THREAD;
        goto end;
    }
#endif
    *h = handle;
    return re;
end:
    free_music_handle(handle);
    return re;
}

int ffmpeg_core_info_open(const CORE_CHAR* url, MusicInfoHandle** handle) {
    if (!url || !handle) return FFMPEG_CORE_ERR_NULLPTR;
    std::string u;
#if _WIN32
    // 将文件名转为UTF-8，ffmpeg API处理的都是UTF-8文件名
    if (!wchar_util::wstr_to_str(u, url, CP_UTF8)) {
        return FFMPEG_CORE_ERR_INVAILD_NAME;
    }
#else
    u = url;
#endif
#if NDEBUG
    // 设置ffmpeg日志级别为Error
    av_log_set_level(AV_LOG_ERROR);
#else
    av_log_set_level(AV_LOG_VERBOSE);
#endif
    MusicInfoHandle* h = (MusicInfoHandle*)malloc(sizeof(MusicInfoHandle));
    int re = FFMPEG_CORE_ERR_OK;
    int is_cda = 0;
    if (!h) {
        return FFMPEG_CORE_ERR_OOM;
    }
    memset(h, 0, sizeof(MusicInfoHandle));
    is_cda = is_cda_file(u.c_str());
    if (is_cda) {
        if ((re = read_cda_file2(h, u.c_str()))) {
            goto end;
        }
        u = fileop::dirname(u);
        if ((re = open_cd_device2(h, u.c_str()))) {
            goto end;
        }
    } else {
        if ((re = open_input2(h, u.c_str()))) {
            goto end;
        }
    }
    if ((re = find_audio_stream2(h))) {
        goto end;
    }
    *handle = h;
    return FFMPEG_CORE_ERR_OK;
end:
    free_music_info_handle(h);
    return re;
}

int ffmpeg_core_play(MusicHandle* handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
#if HAVE_WASAPI
    if (handle->is_use_wasapi) {
        play_WASAPI_device(handle, 1);
    } else {
        SDL_PauseAudioDevice(handle->device_id, 0);
    }
#else
    SDL_PauseAudioDevice(handle->device_id, 0);
#endif
    handle->is_playing = 1;
    return FFMPEG_CORE_ERR_OK;
}

int ffmpeg_core_pause(MusicHandle* handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
#if HAVE_WASAPI
    if (handle->is_use_wasapi) {
        play_WASAPI_device(handle, 0);
    } else {
        SDL_PauseAudioDevice(handle->device_id, 1);
    }
#else
    SDL_PauseAudioDevice(handle->device_id, 1);
#endif
    handle->is_playing = 0;
    return FFMPEG_CORE_ERR_OK;
}

int64_t ffmpeg_core_get_cur_position(MusicHandle* handle) {
    if (!handle) return -1;
#if HAVE_WASAPI
    int64_t pts = cal_true_pts(handle);
#else
    int64_t pts = handle->pts;
#endif
    if (handle->only_part) return pts - handle->part_start_pts;
    return pts;
}

int ffmpeg_core_song_is_over(MusicHandle* handle) {
    if (!handle || !handle->buffer) return 0;
    if (handle->is_eof && av_audio_fifo_size(handle->buffer) == 0) return 1;
    else return 0;
}

int64_t ffmpeg_core_get_song_length(MusicHandle* handle) {
    if (!handle || !handle->fmt) return -1;
    if (handle->only_part) {
        return min(handle->part_end_pts - handle->part_start_pts, handle->fmt->duration - handle->part_start_pts);
    }
    return handle->fmt->duration;
}

int64_t ffmpeg_core_info_get_song_length(MusicInfoHandle* handle) {
    if (!handle || !handle->fmt) return -1;
    if (handle->cda) return get_cda_duration(handle->cda);
    return handle->fmt->duration;
}

int ffmpeg_core_get_channels(MusicHandle* handle) {
    if (!handle || !handle->decoder) return -1;
    return GET_AV_CODEC_CHANNELS(handle->decoder);
}

int ffmpeg_core_info_get_channels(MusicInfoHandle* handle) {
    if (!handle || !handle->is) return -1;
    return GET_AV_CODEC_CHANNELS(handle->is->codecpar);
}

int ffmpeg_core_get_freq(MusicHandle* handle) {
    if (!handle || !handle->decoder) return -1;
    return handle->decoder->sample_rate;
}

int ffmpeg_core_info_get_freq(MusicInfoHandle* handle) {
    if (!handle || !handle->is) return -1;
    return handle->is->codecpar->sample_rate;
}

int ffmpeg_core_seek(MusicHandle* handle, int64_t time) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    size_t st, et;
    st = time_util::time_ns();
#if _WIN32
    DWORD re = WaitForSingleObject(handle->mutex, handle->s->max_wait_time);
#else
    size_t tmp = st + handle->s->max_wait_time * 1000000;
    struct timespec ts;
    ts.tv_sec = tmp / 1000000000;
    ts.tv_nsec = tmp % 1000000000;
    int re = pthread_mutex_timedlock(&handle->mutex, &ts);
#endif
    if (re == WAIT_OBJECT_0) {
        handle->is_seek = 1;
        handle->seek_pos = time;
        if (handle->only_part) {
            handle->seek_pos += handle->part_start_pts;
        }
    } else if (re == WAIT_TIMEOUT) {
        return FFMPEG_CORE_ERR_TIMEOUT;
    } else {
        return FFMPEG_CORE_ERR_WAIT_MUTEX_FAILED;
    }
    if (handle->is_reopen) {
        ReleaseMutex(handle->mutex);
        return FFMPEG_CORE_ERR_OK;
    }
    handle->have_err = 0;
    ReleaseMutex(handle->mutex);
    while (1) {
        if (!handle->is_seek) break;
        et = time_util::time_ns();
        if ((et - st) >= ((size_t)handle->s->max_wait_time * 1000000)) return FFMPEG_CORE_ERR_WAIT_TIMEOUT;
        time_util::mssleep(10);
    }
    return handle->have_err ? handle->err : FFMPEG_CORE_ERR_OK;
}

int ffmpeg_core_is_playing(MusicHandle* handle) {
    if (!handle) return -1;
    return handle->is_playing;
}

int ffmpeg_core_get_bits(MusicHandle* handle) {
    if (!handle || !handle->decoder) return -1;
    return handle->decoder->bits_per_raw_sample;
}

int ffmpeg_core_info_get_bits(MusicInfoHandle* handle) {
    if (!handle || !handle->is) return -1;
    return handle->is->codecpar->bits_per_raw_sample;
}

int64_t ffmpeg_core_get_bitrate(MusicHandle* handle) {
    if (!handle || !handle->decoder) return -1;
    if (handle->decoder->bit_rate) return handle->decoder->bit_rate;
    if (handle->fmt && handle->fmt->bit_rate > 0) return handle->fmt->bit_rate;
    return 0;
}

int64_t ffmpeg_core_info_get_bitrate(MusicInfoHandle* handle) {
    if (!handle || !handle->is) return -1;
    if (handle->is->codecpar->bit_rate > 0) return handle->is->codecpar->bit_rate;
    if (handle->fmt && handle->fmt->bit_rate > 0) return handle->fmt->bit_rate;
    return 0;
}

#if _WIN32
std::wstring get_metadata_str(AVDictionary* dict, const char* key, int flags) {
    auto re = av_dict_get(dict, key, nullptr, flags);
    if (!re || !re->value) return L"";
    std::string value(re->value);
    std::wstring result;
    unsigned int cps[CODEPAGE_SIZE] = { CP_UTF8, CP_ACP, CP_OEMCP };
    for (size_t i = 0; i < CODEPAGE_SIZE; i++) {
        if (wchar_util::str_to_wstr(result, value, cps[i])) {
            return result;
        }
    }
    return L"";
}
#else
std::string get_metadata_str(AVDictionary* dict, const char* key, int flags) {
    auto re = av_dict_get(dict, key, nullptr, flags);
    if (!re || !re->value) return "";
    return std::string(re->value);
}
#endif

CORE_CHAR* ffmpeg_core_get_metadata(MusicHandle* handle, const char* key) {
    if (!handle || !key) return nullptr;
    if (handle->fmt && handle->fmt->metadata) {
        auto re = get_metadata_str(handle->fmt->metadata, key, 0);
        if (!re.empty()) {
            CORE_CHAR* r = nullptr;
            if (cpp2c::string2char(re, r)) {
                return r;
            }
        }
    }
    if (handle->is && handle->is->metadata) {
        auto re = get_metadata_str(handle->is->metadata, key, 0);
        if (!re.empty()) {
            CORE_CHAR* r = nullptr;
            if (cpp2c::string2char(re, r)) {
                return r;
            }
        }
    }
    return nullptr;
}

CORE_CHAR* ffmpeg_core_info_get_metadata(MusicInfoHandle* handle, const char* key) {
    if (!handle || !key) return nullptr;
    if (handle->fmt && handle->fmt->metadata) {
        auto re = get_metadata_str(handle->fmt->metadata, key, 0);
        if (!re.empty()) {
            CORE_CHAR* r = nullptr;
            if (cpp2c::string2char(re, r)) {
                return r;
            }
        }
    }
    if (handle->is && handle->is->metadata) {
        auto re = get_metadata_str(handle->is->metadata, key, 0);
        if (!re.empty()) {
            CORE_CHAR* r = nullptr;
            if (cpp2c::string2char(re, r)) {
                return r;
            }
        }
    }
    return nullptr;
}

int send_reinit_filters(MusicHandle* handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    size_t st, et;
    st = time_util::time_ns();
#if _WIN32
    DWORD re = WaitForSingleObject(handle->mutex, handle->s->max_wait_time);
#else
    size_t tmp = st + handle->s->max_wait_time * 1000000;
    struct timespec ts;
    ts.tv_sec = tmp / 1000000000;
    ts.tv_nsec = tmp % 1000000000;
    int re = pthread_mutex_timedlock(&handle->mutex, &ts);
#endif
    if (re == WAIT_OBJECT_0) {
        handle->need_reinit_filters = 1;
    } else if (re == WAIT_TIMEOUT) {
        return FFMPEG_CORE_ERR_TIMEOUT;
    } else {
        return FFMPEG_CORE_ERR_WAIT_MUTEX_FAILED;
    }
    if (handle->is_reopen) {
        ReleaseMutex(handle->mutex);
        return FFMPEG_CORE_ERR_OK;
    }
    handle->have_err = 0;
    ReleaseMutex(handle->mutex);
    while (handle->need_reinit_filters) {
        et = time_util::time_ns();
        if ((et - st) >= ((size_t)handle->s->max_wait_time * 1000000)) return FFMPEG_CORE_ERR_WAIT_TIMEOUT;
        time_util::mssleep(10);
    }
    return handle->have_err ? handle->err : FFMPEG_CORE_ERR_OK;
}

FfmpegCoreSettings* ffmpeg_core_init_settings() {
    FfmpegCoreSettings* s = (FfmpegCoreSettings*)malloc(sizeof(FfmpegCoreSettings));
    if (!s) return nullptr;
    memset(s, 0, sizeof(FfmpegCoreSettings));
    s->speed = 1.0;
    s->volume = 100;
    s->cache_length = 15;
    s->max_retry_count = 3;
    s->url_retry_interval = 5;
    s->max_wait_time = 3000;
#if HAVE_WASAPI
    s->wasapi_min_buffer_time = 20;
#endif
    s->enable_mixing = 1;
    s->center_mix_level = 0.71;
    s->surround_mix_level = 0.71;
    s->do_not_mix_stereo = 1;
    s->clip_protection = 1;
    s->max_wait_buffer_time = 5;
    return s;
}

int ffmpeg_core_settings_set_volume(FfmpegCoreSettings* s, int volume) {
    if (!s) return 0;
    if (volume >= 0 && volume <= 100) {
        s->volume = volume;
        return 1;
    }
    return 0;
}

int ffmpeg_core_set_volume(MusicHandle* handle, int volume) {
    if (!handle || !handle->s) return FFMPEG_CORE_ERR_NULLPTR;
    int r = ffmpeg_core_settings_set_volume(handle->s, volume);
    if (!r) return FFMPEG_CORE_ERR_FAILED_SET_VOLUME;
    return send_reinit_filters(handle);
}

int ffmpeg_core_settings_set_speed(FfmpegCoreSettings* s, float speed) {
    if (!s) return 0;
    int sp = get_speed(speed);
    if (sp >= 63 && sp <= 16000) {
        s->speed = sp / 1000.0;
        return 1;
    }
    return 0;
}

int ffmpeg_core_set_speed(MusicHandle* handle, float speed) {
    if (!handle || !handle->s) return FFMPEG_CORE_ERR_NULLPTR;
    int r = ffmpeg_core_settings_set_speed(handle->s, speed);
    if (!r) return FFMPEG_CORE_ERR_FAILED_SET_SPEED;
    return send_reinit_filters(handle);
}

int ffmpeg_core_settings_set_cache_length(FfmpegCoreSettings* s, int length) {
    if (!s) return 0;
    if (length >= 1 && length <= 60) s->cache_length = length;
    return 1;
}

int ffmpeg_core_get_error(MusicHandle* handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    return handle->have_err ? handle->err : 0;
}

const CORE_CHAR* ffmpeg_core_get_err_msg2(int err) {
    if (err < 0) return CCORE_CHAR("An error occured in ffmpeg.");
    switch (err) {
        case FFMPEG_CORE_ERR_OK:
            return CCORE_CHAR("No error occured.");
        case FFMPEG_CORE_ERR_NULLPTR:
            return CCORE_CHAR("Got an unexpected null pointer.");
        case FFMPEG_CORE_ERR_INVAILD_NAME:
            return CCORE_CHAR("URI contains invalid chars.");
        case FFMPEG_CORE_ERR_OOM:
            return CCORE_CHAR("Out of memory.");
        case FFMPEG_CORE_ERR_NO_AUDIO_OR_DECODER:
            return CCORE_CHAR("No audio tracks in file or decoder is not available.");
        case FFMPEG_CORE_ERR_UNKNOWN_SAMPLE_FMT:
            return CCORE_CHAR("The format of audio sample is not available.");
        case FFMPEG_CORE_ERR_SDL:
            return CCORE_CHAR("An error occured in SDL.");
        case FFMPEG_CORE_ERR_FAILED_CREATE_THREAD:
            return CCORE_CHAR("Failed to create new thread.");
        case FFMPEG_CORE_ERR_FAILED_CREATE_MUTEX:
            return CCORE_CHAR("Failed to creare new mutex.");
        case FFMPEG_CORE_ERR_WAIT_MUTEX_FAILED:
            return CCORE_CHAR("Failed to wait mutex.");
        case FFMPEG_CORE_ERR_NO_AUDIO:
            return CCORE_CHAR("No audio tracks in file.");
        case FFMPEG_CORE_ERR_FAILED_SET_VOLUME:
            return CCORE_CHAR("Failed to set volume.");
        case FFMPEG_CORE_ERR_FAILED_SET_SPEED:
            return CCORE_CHAR("Failed to set speed.");
        case FFMPEG_CORE_ERR_TOO_BIG_FFT_DATA_LEN:
            return CCORE_CHAR("FFT data's length is too big.");
        case FFMPEG_CORE_ERR_FAILED_OPEN_FILE:
            return CCORE_CHAR("Failed to open file.");
        case FFMPEG_CORE_ERR_FAILED_READ_FILE:
            return CCORE_CHAR("Failed to read file.");
        case FFMPEG_CORE_ERR_INVALID_CDA_FILE:
            return CCORE_CHAR("Invalid CDA file.");
        case FFMPEG_CORE_ERR_NO_LIBCDIO:
            return CCORE_CHAR("libcdio not found.");
        case FFMEPG_CORE_ERR_FAILED_PARSE_URL:
            return CCORE_CHAR("Failed to parse url.");
        case FFMPEG_CORE_ERR_FAILED_SET_EQUALIZER_CHANNEL:
            return CCORE_CHAR("Failed to set equalizer.");
        case FFMPEG_CORE_ERR_FAILED_INIT_WASAPI:
            return CCORE_CHAR("Failed to initialize WASAPI.");
        case FFMEPG_CORE_ERR_FAILED_GET_WASAPI_DEVICE_PROP:
            return CCORE_CHAR("Failed to get WASAPI audio device's property.");
        case FFMPEG_CORE_ERR_WASAPI_NOT_INITED:
            return CCORE_CHAR("WASAPI is not initialized.");
        case FFMPEG_CORE_ERR_WASAPI_DEVICE_NOT_FOUND:
            return CCORE_CHAR("Can not found specified device.");
        case FFMPEG_CORE_ERR_WASAPI_FAILED_OPEN_DEVICE:
            return CCORE_CHAR("Failed to open audio device.");
        case FFMPEG_CORE_ERR_INVALID_DEVICE_NAME:
            return CCORE_CHAR("Device name contains invalid chars.");
        case FFMPEG_CORE_ERR_WASAPI_NO_SUITABLE_FORMAT:
            return CCORE_CHAR("Can not find suitable format for device.");
        case FFMPEG_CORE_ERR_FAILED_CREATE_EVENT:
            return CCORE_CHAR("Failed to create new event.");
        case FFMPEG_CORE_ERR_TIMEOUT:
            return CCORE_CHAR("Operation timeout.");
        case FFMPEG_CORE_ERR_WAIT_TIMEOUT:
            return CCORE_CHAR("The request is already send, but operation not completed.");
        default:
            return CCORE_CHAR("Unknown error.");
    }
}

CORE_CHAR* ffmpeg_core_get_err_msg(int err) {
    if (err < 0) {
        char msg[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, err);
#if _WIN32
        std::wstring wmsg;
        if (wchar_util::str_to_wstr(wmsg, msg, CP_UTF8)) {
            wchar_t* tmp = nullptr;
            if (cpp2c::string2char(wmsg, tmp)) {
                return tmp;
            }
        }
#else
        char* tmp = nullptr;
        if (cpp2c::string2char(msg, tmp)) {
            return tmp;
        }
#endif
    } else if (err == FFMPEG_CORE_ERR_SDL) {
        char msg[128];
        SDL_GetErrorMsg(msg, 128);
#if _WIN32
        std::wstring wmsg;
        if (wchar_util::str_to_wstr(wmsg, msg, CP_UTF8)) {
            wchar_t* tmp = nullptr;
            if (cpp2c::string2char(wmsg, tmp)) {
                return tmp;
            }
        }
#else
        char* tmp = nullptr;
        if (cpp2c::string2char(msg, tmp)) {
            return tmp;
        }
#endif
    } else {
        auto msg = ffmpeg_core_get_err_msg2(err);
        CORE_CHAR* tmp = nullptr;
        if (cpp2c::string2char(msg, tmp)) {
            return tmp;
        }
    }
    return nullptr;
}

int ffmpeg_core_settings_set_max_retry_count(FfmpegCoreSettings* s, int max_retry_count) {
    if (!s) return 0;
    if (max_retry_count >= -1) {
        s->max_retry_count = max_retry_count;
        return 1;
    }
    return 0;
}

int ffmpeg_core_settings_set_url_retry_interval(FfmpegCoreSettings* s, int url_retry_interval) {
    if (!s) return 0;
    if (url_retry_interval >= 1 && url_retry_interval <= 120) {
        s->url_retry_interval = url_retry_interval;
        return 1;
    }
    return 0;
}

int ffmpeg_core_settings_set_equalizer_channel(FfmpegCoreSettings* s, int channel, int gain) {
    if (channel < 0 || channel > 999999 || gain < -900 || gain > 900) return 0;
    return set_equalizer_channel(&s->equalizer_channels, channel, gain) ? 0 : 1;
}

int ffmpeg_core_set_equalizer_channel(MusicHandle* handle, int channel, int gain) {
    if (!handle || !handle->s) return FFMPEG_CORE_ERR_NULLPTR;
    int r = ffmpeg_core_settings_set_equalizer_channel(handle->s, channel, gain);
    if (!r) return FFMPEG_CORE_ERR_FAILED_SET_EQUALIZER_CHANNEL;
    return send_reinit_filters(handle);
}

DeviceNameList* ffmpeg_core_get_audio_devices() {
    DEVICE_NAME_LIST list = nullptr;
    int r = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (r) return nullptr;
    int count = SDL_GetNumAudioDevices(0);
    if (count <= 0) {
        goto end;
    }
    for (int i = 0; i < count; i++) {
        const char* n = SDL_GetAudioDeviceName(i, 0);
        char* p = nullptr;
        if (!n) {
            linked_list_clear(list, tfree);
            goto end;
        }
        if (cstr_util_copy_str(&p, n)) {
            linked_list_clear(list, tfree);
            goto end;
        }
        if (!linked_list_append(list, &p)) {
            free(p);
            linked_list_clear(list, tfree);
            goto end;
        }
    }
end:
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return (DeviceNameList*)list;
}

int ffmpeg_core_is_wasapi_supported() {
#if HAVE_WASAPI
    return 1;
#else
    return 0;
#endif
}

int ffmpeg_core_settings_set_use_WASAPI(FfmpegCoreSettings* s, int enable) {
    if (!s) return 0;
#if HAVE_WASAPI
    s->use_wasapi = enable ? 1 : 0;
    return 1;
#else
    return 0;
#endif
}

int ffmpeg_core_settings_set_enable_exclusive(FfmpegCoreSettings* s, int enable) {
    if (!s) return 0;
#if HAVE_WASAPI
    s->enable_exclusive = enable ? 1 : 0;
    return 1;
#else
    return enable ? 1 : 0;
#endif
}

int ffmpeg_core_settings_set_max_wait_time(FfmpegCoreSettings* s, int timeout) {
    if (!s) return 0;
    if (timeout < 100 || timeout > 30000) return 0;
    s->max_wait_time = timeout;
    return 1;
}

int ffmpeg_core_settings_set_wasapi_min_buffer_time(FfmpegCoreSettings* s, int time) {
#if HAVE_WASAPI
    if (!s) return 0;
    if (time < 20 || time > 30000) return 0;
    s->wasapi_min_buffer_time = time;
#endif
    return 0;
}

int ffmpeg_core_settings_set_reverb(FfmpegCoreSettings* s, int type, float mix, float time) {
    if (!s || type < REVERB_TYPE_OFF || type > REVERB_TYPE_MAX) return 0;
    if (type == REVERB_TYPE_OFF) {
        s->reverb_type = REVERB_TYPE_OFF;
        return 1;
    }
    if (mix <= 0 || time <= 0) return 0;
    s->reverb_type = type;
    s->reverb_delay = time;
    s->reverb_mix = mix;
    return 1;
}

int ffmpeg_core_set_reverb(MusicHandle* handle, int type, float mix, float time) {
    if (!handle || !handle->s) return FFMPEG_CORE_ERR_NULLPTR;
    int re = ffmpeg_core_settings_set_reverb(handle->s, type, mix, time);
    if (!re) return AVERROR(EINVAL);
    return send_reinit_filters(handle);
}

int ffmpeg_core_settings_set_max_wait_buffer_time(FfmpegCoreSettings* s, int time) {
    if (!s) return 0;
    if (time < 1 || time > 5) return 0;
    s->max_wait_buffer_time = time;
    return 1;
}
