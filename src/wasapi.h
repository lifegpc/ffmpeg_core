#ifndef _MUSICPLAYER2_WASAPI_H
#define _MUSICPLAYER2_WASAPI_H
#if __cplusplus
#include "core.h"
extern "C" {
#endif
#include <audioclient.h>
#include <mmdeviceapi.h>
typedef struct WASAPIDevice {
IMMDevice* device;
wchar_t* name;
} WASAPIDevice;
typedef struct WASAPIHandle {
IAudioClient* client;
IAudioRenderClient* render;
uint32_t frame_count;
HANDLE thread;
HRESULT err;
HANDLE eve;
unsigned char is_playing : 1;
unsigned char have_err : 1;
unsigned char stoping : 1;
} WASAPIHandle;
void free_WASAPIHandle(WASAPIHandle* handle);
int init_WASAPI();
void uninit_WASAPI();
int create_WASAPIDevice(IMMDevice* device, WASAPIDevice** output);
void free_WASAPIDevice(WASAPIDevice* device);
/**
 * @brief 创建一个新的IMMDevice实例
 * @param src 源实例
 * @param out 新实例
 * @return 
*/
int copy_IMMDevice(IMMDevice* src, IMMDevice** out);
/**
 * @brief 获取设备
 * @param name 设备名称，如果为NULL返回默认设备
 * @param result 返回结果
 * @return 
*/
int get_Device(const wchar_t* name, IMMDevice** result);
/**
 * @brief 打开WASAPI设备
 * @param handle MusicHandle
 * @param name 设备名称
 * @return 
*/
int open_WASAPI_device(MusicHandle* handle, const wchar_t* name);
void close_WASAPI_device(MusicHandle* handle);
#if NEW_CHANNEL_LAYOUT
int32_t get_WASAPI_channel_mask(AVChannelLayout* channel_layout, int channels);
#else
int32_t get_WASAPI_channel_mask(int64_t channel_layout, int channels);
#endif
int format_is_pcm(enum AVSampleFormat fmt);
int get_format_info(AVCodecContext* context, WAVEFORMATEXTENSIBLE* format);
#define WASAPI_FORMAT_CHANGE_BITS 1
#define WASAPI_FORMAT_CHANGE_CHANNELS 2
#define WASAPI_FORMAT_CHANGE_SAMPLE_RATES 4
#define WASAPI_FORMAT_CHANGE_FORMAT 8
/**
 * @brief 检查client是否支持格式
 * @param client Client
 * @param exclusive 是否为独占模式
 * @param base 基础格式
 * @param result 支持的格式
 * @param change 哪些参数将被手动修改至默认值
 * @return 
*/
int check_format_supported(IAudioClient* client, int exclusive, WAVEFORMATEXTENSIBLE* base, WAVEFORMATEX** result, int change);
int init_wasapi_output(MusicHandle* handle, const char* device);
DWORD WINAPI wasapi_loop2(LPVOID handle);
DWORD WINAPI wasapi_loop(LPVOID handle);
void play_WASAPI_device(MusicHandle* handle, int play);
#if __cplusplus
}
#endif
#endif
