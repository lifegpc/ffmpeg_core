#include "position_data_linked_list.h"

#include "linked_list.h"

#define INTYPE struct LinkedList<PositionData>*
#define OUTTYPE PositionDataList*

int position_data_list_append(PositionDataList** list, int64_t length, char have_data) {
    if (!list) return 0;
    auto l = (INTYPE)(*list);
    PositionData p = { length, have_data };
    auto re = linked_list_append(l, &p);
    *list = (OUTTYPE)l;
    return re ? 1 : 0;
}

void position_data_list_clear(PositionDataList** list) {
    if (!list) return;
    auto l = (INTYPE)(*list);
    linked_list_clear(l);
    *list = (OUTTYPE)l;
}

void position_data_remove_before(PositionDataList* node) {
    linked_list_remove_before((INTYPE)node);
}

PositionDataList* position_data_list_tail(PositionDataList* list) {
    auto l = (INTYPE)list;
    return (OUTTYPE)linked_list_tail(l);
}
