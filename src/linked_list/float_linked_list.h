#ifndef _MUSICPALYER2_FLOAT_LINKED_LIST_H
#define _MUSICPALYER2_FLOAT_LINKED_LIST_H
#if __cplusplus
extern "C" {
#endif
typedef struct float_linked_list {
float d;
struct float_linked_list* prev;
struct float_linked_list* next;
} float_linked_list;
int float_linked_list_append(float_linked_list** list, float data);
void float_linked_list_clear(float_linked_list** list);
char* float_linked_list_join(float_linked_list* list, char join_char);
#if __cplusplus
}
#endif
#endif
