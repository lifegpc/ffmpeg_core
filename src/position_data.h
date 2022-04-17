#ifndef _MUSICPALYER2_POSITION_DATA_H
#define _MUSICPALYER2_POSITION_DATA_H
#if __cplusplus
#include "linked_list/position_data_linked_list.h"
extern "C" {
#endif
#if !__cplusplus
#include "linked_list/position_data_linked_list.h"
#endif
/**
 * @brief 计算真实的WASAPI缓冲区内的缓冲时间
 * @param list 数据来源
 * @param buffered 缓冲时间
 * @return 真实缓冲时间
*/
int64_t cal_true_buffer_time(PositionDataList* list, int64_t buffered);
/**
 * @brief 将新数据加到列表
 * @param handle Handle
 * @param size 新数据大小
 * @param have_data 是否包含真实数据
 * @return 错误代码
*/
int add_data_to_position_data(MusicHandle* handle, int size, char have_data);
/**
 * @brief 计算准确的当前播放时间
 * @param handle Handle
 * @return 当前播放时间（未使用WASAPI时不进行校准）
*/
int64_t cal_true_pts(MusicHandle* handle);
#if __cplusplus
}
#endif
#endif
