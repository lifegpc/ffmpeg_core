#ifndef _MUSICPLAYER2_LOOP_H
#define _MUSICPLAYER2_LOOP_H
#if __cplusplus
extern "C" {
#endif
#include "core.h"
int seek_to_pos(MusicHandle* handle);
int reopen_file(MusicHandle* handle);
/// 基础事件处理，如果处理过返回1反之0
int basic_event_handle(MusicHandle* handle);
#if _WIN32
DWORD WINAPI event_loop(LPVOID handle);
DWORD WINAPI filter_loop(LPVOID handle);
#else
void* event_loop(void* handle);
void* filter_loop(void* handle);
#endif
#if __cplusplus
}
#endif
#endif
