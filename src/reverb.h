#ifndef _MUSICPLAYER2_REVERB_H
#define _MUSICPLAYER2_REVERB_H
#include "core.h"
#if __cplusplus
extern "C" {
#endif
/**
 * @brief 设置混响
 * @param graph Graph
 * @param src 输入
 * @param list Filters列表
 * @param delay 延迟时间
 * @param mix 混响强度
 * @param type 混响类型
 * @return 
*/
int create_reverb_filter(AVFilterGraph* graph, AVFilterContext* src, c_linked_list** list, float delay, float mix, int type);
#if __cplusplus
}
#endif
#endif
