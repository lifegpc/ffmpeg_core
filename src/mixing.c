#include "mixing.h"
#include "ch_layout.h"

void set_mixing_opts(MusicHandle* handle) {
    if (!handle->decoder || !handle->swrac) return;
    int nb_channels = GET_AV_CODEC_CHANNELS(handle->decoder);
    if (handle->s->enable_mixing && (!handle->s->do_not_mix_stereo || nb_channels != 2)) {
        av_opt_set_int(handle->swrac, "clip_protection", !handle->s->normalize_matrix && handle->s->clip_protection, 0);
        av_opt_set_int(handle->swrac, "internal_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
        av_opt_set_double(handle->swrac, "center_mix_level", handle->s->center_mix_level, 0);
        av_opt_set_double(handle->swrac, "surround_mix_level", handle->s->surround_mix_level, 0);
        av_opt_set_double(handle->swrac, "lfe_mix_level", handle->s->lfe_mix_level, 0);
        av_opt_set_double(handle->swrac, "rematrix_maxval", handle->s->normalize_matrix ? 1.0 : 0.0, 0);
        av_opt_set_int(handle->swrac, "matrix_encoding", handle->s->matrix_encoding, 0);
    }
}
