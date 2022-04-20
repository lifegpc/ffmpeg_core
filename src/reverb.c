#include "reverb.h"

#include <math.h>
#include "linked_list/float_linked_list.h"

#define APPEND_DATA(list, data) if (!float_linked_list_append(&list, data)) { \
re = FFMPEG_CORE_ERR_OOM; \
goto end; \
}

#define SET_FLOAT_OPT(key, data) snprintf(buf, sizeof(buf), "%f", data); \
if ((re = av_dict_set(&opts, key, buf, 0)) < 0) { \
goto end; \
} \
re = 0;

#define SET_FLOAT_LIST(key, list) tmp = float_linked_list_join(list, '|'); \
if (tmp) { \
    if ((re = av_dict_set(&opts, key, tmp, 0)) < 0) { \
        free(tmp); \
        goto end; \
    } \
    free(tmp); \
    re = 0; \
} else { \
    re = FFMPEG_CORE_ERR_OOM; \
    goto end; \
}

int create_reverb_filter(AVFilterGraph* graph, AVFilterContext* src, c_linked_list** list, float delay, float mix, int type) {
    if (!graph || !src || !list) return FFMPEG_CORE_ERR_NULLPTR;
    const AVFilter* aecho = avfilter_get_by_name("aecho");
    float in_gain = 1.0f;
    float out_gain = 1.0f;
    float_linked_list* delays = NULL, * decays = NULL;
    AVFilterContext* context = NULL;
    AVDictionary* opts = NULL;
    char buf[32];
    char* tmp = NULL;
    int re = FFMPEG_CORE_ERR_OK;
    switch (type) {
        case REVERB_TYPE_ROOM:
            {
                float d = sqrt(2) / 2;
                APPEND_DATA(delays, delay * d)
                APPEND_DATA(delays, d)
                APPEND_DATA(decays, mix)
                APPEND_DATA(decays, mix * d)
                out_gain = 0.99f / (1.0f + mix + mix * d);
            }
            break;
        default:
            av_log(NULL, AV_LOG_FATAL, "Unknown reverb type: %i.\n", type);
            re = AVERROR(EINVAL);
            goto end;
    }
    if (!delays || !decays) {
        re = AVERROR(EINVAL);
        goto end;
    }
    if (!(context = avfilter_graph_alloc_filter(graph, aecho, "aecho"))) {
        av_log(NULL, AV_LOG_ERROR, "Failed to alloc new filter aecho.\n");
        re = FFMPEG_CORE_ERR_OOM;
        goto end;
    }
    SET_FLOAT_OPT("in_gain", in_gain)
    SET_FLOAT_OPT("out_gain", out_gain)
    SET_FLOAT_LIST("delays", delays)
    SET_FLOAT_LIST("decays", decays)
    if ((re = avfilter_init_dict(context, &opts)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to create aecho filter: %s (%i)\n", av_err2str(re), re);
        goto end;
    }
    if (!c_linked_list_append(list, (void*)context)) {
        av_log(NULL, AV_LOG_FATAL, "Failed to append filter \"aecho\" to list.\n");
        re = FFMPEG_CORE_ERR_OOM;
        goto end;
    }
    if (c_linked_list_count(*list) > 1) {
        AVFilterContext* last = c_linked_list_tail(*list)->prev->d;
        if ((re = avfilter_link(last, 0, context, 0)) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to link %s:%i -> %s:%i: %s (%i)\n", last->name, 0, context->name, 0, av_err2str(re), re);
            goto end;
        }
    } else {
        if ((re = avfilter_link(src, 0, context, 0)) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to link %s:%i -> %s:%i: %s (%i)\n", src->name, 0, context->name, 0, av_err2str(re), re);
            goto end;
        }
    }
end:
    if (opts) av_dict_free(&opts);
    float_linked_list_clear(&delays);
    float_linked_list_clear(&decays);
    return re;
}
