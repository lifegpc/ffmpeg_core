#ifndef _MUSICPALYER2_POSITION_DATA_LINKED_LIST_H
#define _MUSICPALYER2_POSITION_DATA_LINKED_LIST_H
#if __cplusplus
#include "../core.h"
extern "C" {
#endif
#if !__cplusplus
#include "../core.h"
#endif
/// 存放WASAPI缓冲区内一段数据的类型
typedef struct PositionData {
/// 数据长度
int64_t length;
/// 是否有实际数据
char have_data;
} PositionData;
#if __cplusplus
typedef struct PositionDataList: public PositionData {
#else
typedef struct PositionDataList {
struct PositionData;
#endif
struct PositionDataList* prev;
struct PositionDataList* next;
} PositionDataList;
int position_data_list_append(PositionDataList** list, int64_t length, char have_data);
void position_data_list_clear(PositionDataList** list);
void position_data_remove_before(PositionDataList* node);
PositionDataList* position_data_list_tail(PositionDataList* list);
#if __cplusplus
}
#endif
#endif
