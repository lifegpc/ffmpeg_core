#ifndef _MUSICPLAYER2_CH_LAYOUT_H
#define _MUSICPLAYER2_CH_LAYOUT_H
#if __cplusplus
extern "C" {
#endif
#include "core.h"
#if NEW_CHANNEL_LAYOUT
#define GET_AV_CODEC_CHANNELS(context) (context->ch_layout.nb_channels)
#else
#define GET_AV_CODEC_CHANNELS(context) (context->channels)
#endif
#if __cplusplus
}
#endif
#endif
