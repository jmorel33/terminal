#ifndef KT_SERIALIZE_H
#define KT_SERIALIZE_H

#include "kterm.h"

#ifdef __cplusplus
extern "C" {
#endif

// Serialize the session state (grid, cursor, scrollback) to a newly allocated buffer.
// Returns true on success, false on failure.
// The caller is responsible for freeing *out_buf using KTerm_Free.
bool KTerm_SerializeSession(KTermSession* session, void** out_buf, size_t* out_len);

// Restore session state from a buffer.
// Returns true on success, false on failure.
bool KTerm_DeserializeSession(KTermSession* session, const void* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // KT_SERIALIZE_H

#ifdef KTERM_SERIALIZE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define KTERM_SERIALIZE_MAGIC "KTERM_SES_V1"
#define KTERM_SERIALIZE_MAGIC_LEN 12
#define KTERM_SERIALIZE_VERSION 1

typedef struct {
    char magic[KTERM_SERIALIZE_MAGIC_LEN];
    uint32_t version;
    uint32_t header_size;
    int cols;
    int rows;
    int buffer_height;
    int screen_head;
    int view_offset;
    int saved_view_offset;
    int history_rows_populated;
    int cursor_x;
    int cursor_y;
    int scroll_top;
    int scroll_bottom;
    int left_margin;
    int right_margin;
    uint32_t dec_modes;
    ANSIModes ansi_modes;
    ExtendedKTermColor current_fg;
    ExtendedKTermColor current_bg;
    ExtendedKTermColor current_ul_color;
    ExtendedKTermColor current_st_color;
    uint32_t current_attributes;
    int tab_capacity;
    int tab_count;
    int tab_default_width;
    bool title_changed;
    bool icon_changed;
    char window_title[MAX_TITLE_LENGTH];
    char icon_title[MAX_TITLE_LENGTH];
    char terminal_name[64];
} KTermSessionHeader;

static bool KTerm_SerializeMulOverflow(size_t a, size_t b, size_t* out) {
    if (!out) return true;
    if (a != 0 && b > SIZE_MAX / a) return true;
    *out = a * b;
    return false;
}

static bool KTerm_SerializeAddOverflow(size_t a, size_t b, size_t* out) {
    if (!out) return true;
    if (b > SIZE_MAX - a) return true;
    *out = a + b;
    return false;
}

bool KTerm_SerializeSession(KTermSession* session, void** out_buf, size_t* out_len) {
    if (!session || !out_buf || !out_len) return false;
    *out_buf = NULL;
    *out_len = 0;
    if (!session->screen_buffer || !session->alt_buffer) return false;
    if (session->cols <= 0 || session->rows <= 0 || session->buffer_height < session->rows) return false;

    // Calculate size
    size_t header_size = sizeof(KTermSessionHeader);
    size_t screen_cells = 0;
    size_t alt_cells = 0;
    size_t screen_size = 0;
    size_t alt_size = 0;
    size_t tab_size = 0;
    size_t total_size = 0;

    if (KTerm_SerializeMulOverflow((size_t)session->buffer_height, (size_t)session->cols, &screen_cells)) return false;
    if (KTerm_SerializeMulOverflow(screen_cells, sizeof(EnhancedTermChar), &screen_size)) return false;
    if (KTerm_SerializeMulOverflow((size_t)session->rows, (size_t)session->cols, &alt_cells)) return false;
    if (KTerm_SerializeMulOverflow(alt_cells, sizeof(EnhancedTermChar), &alt_size)) return false;
    if (session->tab_stops.stops && session->tab_stops.capacity > 0) {
        if (KTerm_SerializeMulOverflow((size_t)session->tab_stops.capacity, sizeof(bool), &tab_size)) return false;
    }
    if (KTerm_SerializeAddOverflow(header_size, screen_size, &total_size)) return false;
    if (KTerm_SerializeAddOverflow(total_size, alt_size, &total_size)) return false;
    if (KTerm_SerializeAddOverflow(total_size, tab_size, &total_size)) return false;

    *out_buf = KTerm_Malloc(total_size);
    if (!*out_buf) return false;

    *out_len = total_size;
    unsigned char* ptr = (unsigned char*)*out_buf;

    // Header
    KTermSessionHeader header;
    memset(&header, 0, sizeof(header));
    strncpy(header.magic, KTERM_SERIALIZE_MAGIC, KTERM_SERIALIZE_MAGIC_LEN);
    header.version = KTERM_SERIALIZE_VERSION;
    header.header_size = (uint32_t)sizeof(KTermSessionHeader);
    header.cols = session->cols;
    header.rows = session->rows;
    header.buffer_height = session->buffer_height;
    header.screen_head = session->screen_head;
    header.view_offset = session->view_offset;
    header.saved_view_offset = session->saved_view_offset;
    header.history_rows_populated = session->history_rows_populated;
    header.cursor_x = session->cursor.x;
    header.cursor_y = session->cursor.y;
    header.scroll_top = session->scroll_top;
    header.scroll_bottom = session->scroll_bottom;
    header.left_margin = session->left_margin;
    header.right_margin = session->right_margin;
    header.dec_modes = session->dec_modes;
    header.ansi_modes = session->ansi_modes;
    header.current_fg = session->current_fg;
    header.current_bg = session->current_bg;
    header.current_ul_color = session->current_ul_color;
    header.current_st_color = session->current_st_color;
    header.current_attributes = session->current_attributes;
    header.tab_capacity = session->tab_stops.capacity;
    header.tab_count = session->tab_stops.count;
    header.tab_default_width = session->tab_stops.default_width;
    header.title_changed = session->title.title_changed;
    header.icon_changed = session->title.icon_changed;
    memcpy(header.window_title, session->title.window_title, sizeof(header.window_title));
    memcpy(header.icon_title, session->title.icon_title, sizeof(header.icon_title));
    memcpy(header.terminal_name, session->title.terminal_name, sizeof(header.terminal_name));

    memcpy(ptr, &header, header_size);
    ptr += header_size;

    // Screen Buffer
    memcpy(ptr, session->screen_buffer, screen_size);
    ptr += screen_size;

    // Alt Buffer
    memcpy(ptr, session->alt_buffer, alt_size);
    ptr += alt_size;

    if (tab_size > 0) {
        memcpy(ptr, session->tab_stops.stops, tab_size);
        ptr += tab_size;
    }

    return true;
}

bool KTerm_DeserializeSession(KTermSession* session, const void* buf, size_t len) {
    if (!session || !buf) return false;

    const unsigned char* ptr = (const unsigned char*)buf;
    size_t offset = 0;

    // Header Check
    if (len < sizeof(KTermSessionHeader)) return false;
    KTermSessionHeader header;
    memcpy(&header, ptr, sizeof(KTermSessionHeader));

    if (strncmp(header.magic, KTERM_SERIALIZE_MAGIC, KTERM_SERIALIZE_MAGIC_LEN) != 0) {
        return false; // Invalid magic
    }
    if (header.version != KTERM_SERIALIZE_VERSION || header.header_size != sizeof(KTermSessionHeader)) {
        return false;
    }
    if (header.cols <= 0 || header.rows <= 0 || header.buffer_height < header.rows) return false;
    if (header.screen_head < 0 || header.screen_head >= header.buffer_height) return false;
    if (header.view_offset < 0 || header.saved_view_offset < 0) return false;
    if (header.history_rows_populated < 0 || header.history_rows_populated > MAX_SCROLLBACK_LINES) return false;
    if (header.cursor_x < 0 || header.cursor_x >= header.cols) return false;
    if (header.cursor_y < 0 || header.cursor_y >= header.rows) return false;
    if (header.scroll_top < 0 || header.scroll_bottom < header.scroll_top || header.scroll_bottom >= header.rows) return false;
    if (header.left_margin < 0 || header.right_margin < header.left_margin || header.right_margin >= header.cols) return false;
    if (header.tab_capacity < 0 || header.tab_count < 0 || header.tab_count > header.tab_capacity) return false;
    if (header.tab_capacity > 0 && header.tab_default_width <= 0) return false;

    offset += sizeof(KTermSessionHeader);

    // Validate dimensions match session? Or resize session?
    // For now, assume session is already initialized or resized to match.
    // Ideally, we should resize the session to match the serialized state.
    // Let's call QueueResize? No, we are restoring state directly.
    // If dimensions differ, we can't easily map the buffer directly.
    // The simplest approach for Phase 1 is to require dimensions match or fail,
    // OR resize the session if possible.
    // Since KTermSession allocation depends on cols/rows, we should probably check.

    if (session->cols != header.cols || session->rows != header.rows) {
        // We could call KTerm_QueueResize but that's async.
        // We probably want synchronous restoration.
        // Let's just fail for now if dimensions mismatch, or implement resize logic.
        // Given we are deserializing "into" a session, the user should probably ensure it's sized correctly
        // or we force it.
        // Let's try to update metadata.
        // But buffer sizes are critical.
        // If we just overwrite pointers, we leak old buffers if we don't free them.
        // Let's assume for this "hook" implementation that we just overwrite content
        // IF sizes match. If not, fail.
        if (session->cols != header.cols || session->rows != header.rows) {
             // For robustness, let's just return false and let caller handle resize.
             // Or, we can try to realloc.
             return false;
        }
    }

    size_t screen_cells = 0;
    size_t alt_cells = 0;
    size_t screen_size = 0;
    size_t alt_size = 0;
    size_t tab_size = 0;

    if (KTerm_SerializeMulOverflow((size_t)header.buffer_height, (size_t)header.cols, &screen_cells)) return false;
    if (KTerm_SerializeMulOverflow(screen_cells, sizeof(EnhancedTermChar), &screen_size)) return false;
    if (KTerm_SerializeMulOverflow((size_t)header.rows, (size_t)header.cols, &alt_cells)) return false;
    if (KTerm_SerializeMulOverflow(alt_cells, sizeof(EnhancedTermChar), &alt_size)) return false;
    if (header.tab_capacity > 0) {
        if (KTerm_SerializeMulOverflow((size_t)header.tab_capacity, sizeof(bool), &tab_size)) return false;
    }

    size_t expected_len = 0;
    if (KTerm_SerializeAddOverflow(offset, screen_size, &expected_len)) return false;
    if (KTerm_SerializeAddOverflow(expected_len, alt_size, &expected_len)) return false;
    if (KTerm_SerializeAddOverflow(expected_len, tab_size, &expected_len)) return false;
    if (expected_len > len) return false; // Buffer too small

    // Restore Metadata
    session->buffer_height = header.buffer_height; // Should match if cols/rows match?
    // Wait, buffer_height includes scrollback. If serialized session had different scrollback depth?
    // We should be careful.
    // If buffer_height differs, we can't direct copy if we treat it as ring buffer.
    // Let's check buffer_height.
    if (session->buffer_height != header.buffer_height) {
        return false;
    }

    session->screen_head = header.screen_head;
    session->view_offset = header.view_offset;
    session->saved_view_offset = header.saved_view_offset;
    session->history_rows_populated = header.history_rows_populated;
    session->cursor.x = header.cursor_x;
    session->cursor.y = header.cursor_y;
    session->scroll_top = header.scroll_top;
    session->scroll_bottom = header.scroll_bottom;
    session->left_margin = header.left_margin;
    session->right_margin = header.right_margin;
    session->dec_modes = (DECModes)header.dec_modes;
    session->ansi_modes = header.ansi_modes;
    session->current_fg = header.current_fg;
    session->current_bg = header.current_bg;
    session->current_ul_color = header.current_ul_color;
    session->current_st_color = header.current_st_color;
    session->current_attributes = header.current_attributes;
    session->title.title_changed = header.title_changed;
    session->title.icon_changed = header.icon_changed;
    memcpy(session->title.window_title, header.window_title, sizeof(session->title.window_title));
    session->title.window_title[MAX_TITLE_LENGTH - 1] = '\0';
    memcpy(session->title.icon_title, header.icon_title, sizeof(session->title.icon_title));
    session->title.icon_title[MAX_TITLE_LENGTH - 1] = '\0';
    memcpy(session->title.terminal_name, header.terminal_name, sizeof(session->title.terminal_name));
    session->title.terminal_name[sizeof(session->title.terminal_name) - 1] = '\0';

    // Restore Screen Buffer
    memcpy(session->screen_buffer, ptr + offset, screen_size);
    offset += screen_size;

    // Restore Alt Buffer
    memcpy(session->alt_buffer, ptr + offset, alt_size);
    offset += alt_size;

    if (tab_size > 0) {
        if (header.tab_capacity != session->tab_stops.capacity) {
            bool* new_stops = (bool*)KTerm_Realloc(session->tab_stops.stops, tab_size);
            if (!new_stops) return false;
            session->tab_stops.stops = new_stops;
            session->tab_stops.capacity = header.tab_capacity;
        }
        memcpy(session->tab_stops.stops, ptr + offset, tab_size);
        session->tab_stops.count = header.tab_count;
        session->tab_stops.default_width = header.tab_default_width;
        offset += tab_size;
    }

    // Mark as dirty
    if (session->row_dirty) {
        for(int i=0; i<session->rows; i++) session->row_dirty[i] = KTERM_DIRTY_FRAMES;
    }
    session->dirty_rect = (KTermRect){0, 0, session->cols, session->rows};

    return true;
}

#endif // KTERM_SERIALIZE_IMPLEMENTATION
