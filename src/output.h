#ifndef _MUSICPLAYER2_OUTPUT_H
#define _MUSICPLAYER2_OUTPUT_H
#if __cplusplus
extern "C" {
#endif
#include "core.h"
int init_output(MusicHandle* handle, const char* device);
enum AVSampleFormat convert_to_sdl_supported_format(enum AVSampleFormat fmt);
void SDL_callback(void* userdata, uint8_t* stream, int len);
SDL_AudioFormat convert_to_sdl_format(enum AVSampleFormat fmt);
#if NEW_CHANNEL_LAYOUT
/**
 * @brief 获取与SDL输出匹配的输出布局
 * @param channels 声道数
 * @param channel_layout 布局
 * @return 错误代码
*/
int get_sdl_channel_layout(int channels, AVChannelLayout* channel_layout);
#else
/**
 * @brief 获取与SDL输出匹配的输出布局
 * @param channels 声道数
 * @return 布局
*/
uint64_t get_sdl_channel_layout(int channels);
#endif
#if __cplusplus
}
#endif
#endif
