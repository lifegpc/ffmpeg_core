#include "ch_layout.h"

#include <string.h>

#if NEW_CHANNEL_LAYOUT
int get_channel_layout_channels(uint64_t channel_layout) {
    AVChannelLayout tmp;
    memset(&tmp, 0, sizeof(AVChannelLayout));
    if (av_channel_layout_from_mask(&tmp, channel_layout)) {
        return 0;
    }
    int re = tmp.nb_channels;
    av_channel_layout_uninit(&tmp);
    return re;
}
#endif
