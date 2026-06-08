#ifndef KT_CLIENT_STRING_H
#define KT_CLIENT_STRING_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static inline void KTermClient_CopyString(char* dst, size_t dst_size, const char* src) {
    if (!dst || dst_size == 0) return;
    if (!src) src = "";
    snprintf(dst, dst_size, "%s", src);
}

static inline void KTermClient_CopySpan(char* dst, size_t dst_size, const char* src, size_t src_len) {
    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    if (src_len >= dst_size) src_len = dst_size - 1;
    memcpy(dst, src, src_len);
    dst[src_len] = '\0';
}

#endif // KT_CLIENT_STRING_H
