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
#if NEW_CHANNEL_LAYOUT
/**
 * @brief 从Channel Mask获取channel数
 * @param channel_layout Channel Mask
 * @return Channel数
*/
int get_channel_layout_channels(uint64_t channel_layout);
#else
#define get_channel_layout_channels av_get_channel_layout_nb_channels
#endif
#if __cplusplus
}
#endif
#endif
