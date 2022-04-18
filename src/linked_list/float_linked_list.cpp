#include "float_linked_list.h"

#include <stdio.h>
#include "cpp2c.h"
#include "linked_list.h"

#define INTYPE struct LinkedList<float>*
#define OUTTYPE float_linked_list*

int float_linked_list_append(float_linked_list** list, float data) {
    if (!list) return 0;
    auto l = (INTYPE)(*list);
    auto re = linked_list_append(l, &data);
    *list = (OUTTYPE)l;
    return re ? 1 : 0;
}

void float_linked_list_clear(float_linked_list** list) {
    if (!list) return;
    auto l = (INTYPE)(*list);
    linked_list_clear(l);
    *list = (OUTTYPE)l;
}

char* float_linked_list_join(float_linked_list* list, char join_char) {
    if (!list) return nullptr;
    std::string s;
    char buf[32];
    snprintf(buf, sizeof(buf), "%f", list->d);
    s = buf;
    float_linked_list* cur = list;
    while (cur->next) {
        cur = cur->next;
        snprintf(buf, sizeof(buf), "%f", cur->d);
        s += join_char;
        s += buf;
    }
    char* re = nullptr;
    if (cpp2c::string2char(s, re)) return re;
    return nullptr;
}
