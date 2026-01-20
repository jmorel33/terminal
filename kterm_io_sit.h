#ifndef KTERM_IO_SIT_H
#define KTERM_IO_SIT_H

#include "kterm.h"

#ifdef KTERM_TESTING
#include "mock_situation.h"
#else
#include "situation.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// INPUT ADAPTER (Situation -> KTerm)
// =============================================================================
// This adapter handles keyboard and mouse input using the Situation library
// and translates it into VT sequences for KTerm.

void KTermSit_ProcessInput(KTerm* term);

#ifdef KTERM_IO_SIT_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Helper macro to access active session
#ifndef GET_SESSION
#define GET_SESSION(term) (&(term)->sessions[(term)->active_session])
#endif

// Use Core Key Event Structure
typedef KTermEvent KTermSitKeyEvent;

// Forward declarations of internal helpers
static void KTermSit_UpdateKeyboard(KTerm* term);
static void KTermSit_UpdateMouse(KTerm* term);
static void KTermSit_GenerateVTSequence(KTerm* term, KTermSitKeyEvent* event);
static void KTermSit_HandleControlKey(KTerm* term, KTermSitKeyEvent* event);
static void KTermSit_HandleAltKey(KTerm* term, KTermSitKeyEvent* event);
static int KTermSit_EncodeUTF8(int codepoint, char* buffer);

void KTermSit_ProcessInput(KTerm* term) {
    KTermSit_UpdateKeyboard(term);
    KTermSit_UpdateMouse(term);
}

static void KTermSit_HandleControlKey(KTerm* term, KTermSitKeyEvent* event) {
    // Handle Ctrl+key combinations
    if (event->key_code >= SIT_KEY_A && event->key_code <= SIT_KEY_Z) {
        int ctrl_char = event->key_code - SIT_KEY_A + 1;
        event->sequence[0] = (char)ctrl_char;
        event->sequence[1] = '\0';
    } else {
        switch (event->key_code) {
            case SIT_KEY_SPACE:      event->sequence[0] = 0x00; break;
            case SIT_KEY_LEFT_BRACKET:  event->sequence[0] = 0x1B; break;
            case SIT_KEY_BACKSLASH:  event->sequence[0] = 0x1C; break;
            case SIT_KEY_RIGHT_BRACKET: event->sequence[0] = 0x1D; break;
            case SIT_KEY_GRAVE_ACCENT:      event->sequence[0] = 0x1E; break;
            case SIT_KEY_MINUS:      event->sequence[0] = 0x1F; break;
            default: event->sequence[0] = '\0'; break;
        }
        if (event->sequence[0] != '\0') event->sequence[1] = '\0';
    }
}

static void KTermSit_HandleAltKey(KTerm* term, KTermSitKeyEvent* event) {
    if (event->key_code >= SIT_KEY_A && event->key_code <= SIT_KEY_Z) {
        char letter = 'a' + (event->key_code - SIT_KEY_A);
        if (event->shift) letter = 'A' + (event->key_code - SIT_KEY_A);
        snprintf(event->sequence, sizeof(event->sequence), "\x1B%c", letter);
    } else if (event->key_code >= SIT_KEY_0 && event->key_code <= SIT_KEY_9) {
        char digit = '0' + (event->key_code - SIT_KEY_0);
        snprintf(event->sequence, sizeof(event->sequence), "\x1B%c", digit);
    } else {
        event->sequence[0] = '\0';
    }
}

static void KTermSit_GenerateVTSequence(KTerm* term, KTermSitKeyEvent* event) {
    memset(event->sequence, 0, sizeof(event->sequence));
    KTermSession* session = GET_SESSION(term);

    switch (event->key_code) {
        case SIT_KEY_UP:
            snprintf(event->sequence, sizeof(event->sequence), session->dec_modes.application_cursor_keys ? "\x1BOA" : "\x1B[A");
            if (event->ctrl) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;5A");
            else if (event->alt) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;3A");
            break;
        case SIT_KEY_DOWN:
            snprintf(event->sequence, sizeof(event->sequence), session->dec_modes.application_cursor_keys ? "\x1BOB" : "\x1B[B");
            if (event->ctrl) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;5B");
            else if (event->alt) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;3B");
            break;
        case SIT_KEY_RIGHT:
            snprintf(event->sequence, sizeof(event->sequence), session->dec_modes.application_cursor_keys ? "\x1BOC" : "\x1B[C");
            if (event->ctrl) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;5C");
            else if (event->alt) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;3C");
            break;
        case SIT_KEY_LEFT:
            snprintf(event->sequence, sizeof(event->sequence), session->dec_modes.application_cursor_keys ? "\x1BOD" : "\x1B[D");
            if (event->ctrl) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;5D");
            else if (event->alt) snprintf(event->sequence, sizeof(event->sequence), "\x1B[1;3D");
            break;

        case SIT_KEY_HOME: strcpy(event->sequence, session->dec_modes.application_cursor_keys ? "\x1BOH" : "\x1B[H"); break;
        case SIT_KEY_END:  strcpy(event->sequence, session->dec_modes.application_cursor_keys ? "\x1BOF" : "\x1B[F"); break;

        case SIT_KEY_PAGE_UP:   strcpy(event->sequence, "\x1B[5~"); break;
        case SIT_KEY_PAGE_DOWN: strcpy(event->sequence, "\x1B[6~"); break;
        case SIT_KEY_INSERT:    strcpy(event->sequence, "\x1B[2~"); break;
        case SIT_KEY_DELETE:    strcpy(event->sequence, "\x1B[3~"); break;

        case SIT_KEY_F1:  strncpy(event->sequence, session->input.function_keys[0], 31); break;
        case SIT_KEY_F2:  strncpy(event->sequence, session->input.function_keys[1], 31); break;
        case SIT_KEY_F3:  strncpy(event->sequence, session->input.function_keys[2], 31); break;
        case SIT_KEY_F4:  strncpy(event->sequence, session->input.function_keys[3], 31); break;
        case SIT_KEY_F5:  strncpy(event->sequence, session->input.function_keys[4], 31); break;
        case SIT_KEY_F6:  strncpy(event->sequence, session->input.function_keys[5], 31); break;
        case SIT_KEY_F7:  strncpy(event->sequence, session->input.function_keys[6], 31); break;
        case SIT_KEY_F8:  strncpy(event->sequence, session->input.function_keys[7], 31); break;
        case SIT_KEY_F9:  strncpy(event->sequence, session->input.function_keys[8], 31); break;
        case SIT_KEY_F10: strncpy(event->sequence, session->input.function_keys[9], 31); break;
        case SIT_KEY_F11: strncpy(event->sequence, session->input.function_keys[10], 31); break;
        case SIT_KEY_F12: strncpy(event->sequence, session->input.function_keys[11], 31); break;

        case SIT_KEY_ENTER:     strcpy(event->sequence, session->ansi_modes.line_feed_new_line ? "\r" : "\n"); break;
        case SIT_KEY_TAB:       strcpy(event->sequence, "\t"); break;
        case SIT_KEY_BACKSPACE: strcpy(event->sequence, session->input.backarrow_sends_bs ? "\b" : "\x7F"); break;
        case SIT_KEY_ESCAPE:    strcpy(event->sequence, "\x1B"); break;

        // Keypad
        case SIT_KEY_KP_0: case SIT_KEY_KP_1: case SIT_KEY_KP_2: case SIT_KEY_KP_3: case SIT_KEY_KP_4:
        case SIT_KEY_KP_5: case SIT_KEY_KP_6: case SIT_KEY_KP_7: case SIT_KEY_KP_8: case SIT_KEY_KP_9:
            if (session->input.keypad_application_mode) {
                snprintf(event->sequence, sizeof(event->sequence), "\x1BO%c", 'p' + (event->key_code - SIT_KEY_KP_0));
            } else {
                snprintf(event->sequence, sizeof(event->sequence), "%c", '0' + (event->key_code - SIT_KEY_KP_0));
            }
            break;
        case SIT_KEY_KP_DECIMAL:  strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOn" : "."); break;
        case SIT_KEY_KP_ENTER:    strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOM" : "\r"); break;
        case SIT_KEY_KP_ADD:      strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOk" : "+"); break;
        case SIT_KEY_KP_SUBTRACT: strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOm" : "-"); break;
        case SIT_KEY_KP_MULTIPLY: strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOj" : "*"); break;
        case SIT_KEY_KP_DIVIDE:   strcpy(event->sequence, session->input.keypad_application_mode ? "\x1BOo" : "/"); break;

        default:
            if (event->ctrl) KTermSit_HandleControlKey(term, event);
            else if (event->alt && session->input.meta_sends_escape) KTermSit_HandleAltKey(term, event);
            break;
    }
}

static void KTermSit_UpdateKeyboard(KTerm* term) {
    // Process Key Events (Queue based to preserve order)
    int rk;
    while ((rk = SituationGetKeyPressed()) != 0) {
        KTermSession* session = GET_SESSION(term);

        // 1. UDK Handling
        bool udk_found = false;
        for (size_t i = 0; i < session->programmable_keys.count; i++) {
            if (session->programmable_keys.keys[i].key_code == rk && session->programmable_keys.keys[i].active) {
                const char* seq = session->programmable_keys.keys[i].sequence;
                KTerm_QueueResponse(term, seq);
                if (session->dec_modes.local_echo) KTerm_WriteString(term, seq);
                udk_found = true;
                break;
            }
        }
        if (udk_found) continue;

        // 2. Standard Key Handling
        KTermSitKeyEvent event = {0};
        event.key_code = rk;
        event.ctrl = SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL);
        event.alt = SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT);
        event.shift = SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT);

        if (rk >= 32 && rk <= 126 && !event.ctrl && !event.alt) continue; // Handled by CharPressed

        // Special Scrollback Handling (Shift + PageUp/Down)
        if (event.shift && (rk == SIT_KEY_PAGE_UP || rk == SIT_KEY_PAGE_DOWN)) {
            if (rk == SIT_KEY_PAGE_UP) session->view_offset += DEFAULT_TERM_HEIGHT / 2;
            else session->view_offset -= DEFAULT_TERM_HEIGHT / 2;

            if (session->view_offset < 0) session->view_offset = 0;
            int max_offset = session->buffer_height - DEFAULT_TERM_HEIGHT;
            if (session->view_offset > max_offset) session->view_offset = max_offset;
            for (int i=0; i<DEFAULT_TERM_HEIGHT; i++) session->row_dirty[i] = true;
            continue;
        }

        KTermSit_GenerateVTSequence(term, &event);
        if (event.sequence[0] != '\0') {
            // Push to Input Queue instead of direct response
            KTerm_QueueInputEvent(term, event);
        }
    }

    // Process Unicode Characters
    int ch_unicode;
    while ((ch_unicode = SituationGetCharPressed()) != 0) {
        KTermSitKeyEvent event = {0};
        event.key_code = ch_unicode;

        // Populate sequence directly
        char sequence[8] = {0};
        bool ctrl = SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL);
        bool alt = SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT);

        event.ctrl = ctrl;
        event.alt = alt;

        if (ctrl && ch_unicode >= 'a' && ch_unicode <= 'z') sequence[0] = (char)(ch_unicode - 'a' + 1);
        else if (ctrl && ch_unicode >= 'A' && ch_unicode <= 'Z') sequence[0] = (char)(ch_unicode - 'A' + 1);
        else if (alt && GET_SESSION(term)->input.meta_sends_escape && !ctrl) {
            sequence[0] = '\x1B';
            KTermSit_EncodeUTF8(ch_unicode, sequence + 1);
        } else {
            KTermSit_EncodeUTF8(ch_unicode, sequence);
        }

        if (sequence[0] != '\0') {
            strncpy(event.sequence, sequence, sizeof(event.sequence));
            KTerm_QueueInputEvent(term, event);
        }
    }
}

static int KTermSit_EncodeUTF8(int codepoint, char* buffer) {
    if (codepoint < 0x80) {
        buffer[0] = (char)codepoint;
        buffer[1] = '\0';
        return 1;
    } else if (codepoint < 0x800) {
        buffer[0] = (char)(0xC0 | (codepoint >> 6));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        buffer[2] = '\0';
        return 2;
    } else if (codepoint < 0x10000) {
        buffer[0] = (char)(0xE0 | (codepoint >> 12));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        buffer[3] = '\0';
        return 3;
    } else {
        buffer[0] = (char)(0xF0 | (codepoint >> 18));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        buffer[4] = '\0';
        return 4;
    }
}

static void KTermSit_UpdateMouse(KTerm* term) {
    Vector2 mouse_pos = SituationGetMousePosition();
    int global_cell_x = (int)mouse_pos.x / (DEFAULT_CHAR_WIDTH * DEFAULT_WINDOW_SCALE);
    int global_cell_y = (int)mouse_pos.y / (DEFAULT_CHAR_HEIGHT * DEFAULT_WINDOW_SCALE);

    int target_session_idx = term->active_session;
    int local_cell_y = global_cell_y;

    if (term->split_screen_active) {
        if (global_cell_y <= term->split_row) {
            target_session_idx = term->session_top;
            local_cell_y = global_cell_y;
        } else {
            target_session_idx = term->session_bottom;
            local_cell_y = global_cell_y - (term->split_row + 1);
        }
    }

    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (term->active_session != target_session_idx) {
            KTerm_SetActiveSession(term, target_session_idx);
        }
    }

    // Temporarily switch to target session for correct context (QueueResponse, State Access)
    int saved_session_idx = term->active_session;
    term->active_session = target_session_idx;
    KTermSession* session = &term->sessions[target_session_idx];

    if (global_cell_x < 0) global_cell_x = 0; if (global_cell_x >= DEFAULT_TERM_WIDTH) global_cell_x = DEFAULT_TERM_WIDTH - 1;
    if (local_cell_y < 0) local_cell_y = 0; if (local_cell_y >= DEFAULT_TERM_HEIGHT) local_cell_y = DEFAULT_TERM_HEIGHT - 1;

    float wheel = SituationGetMouseWheelMove();
    if (wheel != 0) {
        // Application Mouse Reporting for Wheel
        if (session->conformance.features.mouse_tracking &&
            session->mouse.enabled &&
            session->mouse.mode != MOUSE_TRACKING_OFF) {

            int btn = (wheel > 0) ? 64 : 65;
            char report[64] = {0};

            if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) btn += 4;
            if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) btn += 8;
            if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) btn += 16;

            if (session->mouse.sgr_mode || session->mouse.mode == MOUSE_TRACKING_URXVT || session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                 int px = global_cell_x + 1;
                 int py = local_cell_y + 1;
                 if (session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                    px = (int)mouse_pos.x + 1;
                    py = (int)mouse_pos.y + 1;
                 }
                 snprintf(report, sizeof(report), "\x1B[<%d;%d;%dM", btn, px, py);
            } else if (session->mouse.mode >= MOUSE_TRACKING_VT200) {
                int cb = 32 + btn; // 32 + 64/65
                snprintf(report, sizeof(report), "\x1B[M%c%c%c", (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
            }
            if (report[0]) KTerm_QueueResponse(term, report);
        }
        else {
            // Normal Terminal Scrolling
            if (session->dec_modes.alternate_screen) {
                int lines = 3;
                const char* seq = (wheel > 0) ? (session->dec_modes.application_cursor_keys ? "\x1BOA" : "\x1B[A")
                                              : (session->dec_modes.application_cursor_keys ? "\x1BOB" : "\x1B[B");
                for(int i=0; i<lines; i++) KTerm_QueueResponse(term, seq);
            } else {
                int scroll_amount = (int)(wheel * 3.0f);
                session->view_offset += scroll_amount;
                if (session->view_offset < 0) session->view_offset = 0;
                int max_offset = session->buffer_height - DEFAULT_TERM_HEIGHT;
                if (session->view_offset > max_offset) session->view_offset = max_offset;
                for(int i=0; i<DEFAULT_TERM_HEIGHT; i++) session->row_dirty[i] = true;
            }
        }
    }

    // Selection Logic
    if (SituationIsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        session->selection.active = true;
        session->selection.dragging = true;
        session->selection.start_x = global_cell_x;
        session->selection.start_y = local_cell_y;
        session->selection.end_x = global_cell_x;
        session->selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && session->selection.dragging) {
        session->selection.end_x = global_cell_x;
        session->selection.end_y = local_cell_y;
    } else if (SituationIsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT) && session->selection.dragging) {
        session->selection.dragging = false;
        KTerm_CopySelectionToClipboard(term);
    }

    // Focus Tracking
    bool current_focus = SituationHasWindowFocus();
    if (current_focus != session->mouse.focused) {
        session->mouse.focused = current_focus;
        if (session->mouse.focus_tracking) {
            KTerm_QueueResponse(term, current_focus ? "\x1B[I" : "\x1B[O");
        }
    }

    if (session->conformance.features.mouse_tracking) {
        if (!session->mouse.enabled || session->mouse.mode == MOUSE_TRACKING_OFF) {
            SituationShowCursor();
            session->mouse.cursor_x = -1;
            session->mouse.cursor_y = -1;
        } else {
            SituationHideCursor();

            session->mouse.cursor_x = global_cell_x + 1;
            session->mouse.cursor_y = local_cell_y + 1;

            bool current_buttons[3] = {
                SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT),
                SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_MIDDLE),
                SituationIsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)
            };

            for (int i=0; i<3; i++) {
                if (current_buttons[i] != session->mouse.buttons[i]) {
                    session->mouse.buttons[i] = current_buttons[i];
                    bool pressed = current_buttons[i];
                    int btn = i;
                    char report[64] = {0};

                    if (session->mouse.sgr_mode || session->mouse.mode == MOUSE_TRACKING_URXVT || session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                        if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) btn += 4;
                        if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) btn += 8;
                        if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) btn += 16;

                        int px = global_cell_x + 1;
                        int py = local_cell_y + 1;
                        if (session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                            px = (int)mouse_pos.x + 1;
                            py = (int)mouse_pos.y + 1;
                        }
                        snprintf(report, sizeof(report), "\x1B[<%d;%d;%d%c", btn, px, py, pressed ? 'M' : 'm');
                    } else if (session->mouse.mode >= MOUSE_TRACKING_VT200) {
                        int cb = 32 + (pressed ? i : 3);
                        if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) cb += 4;
                        if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) cb += 8;
                        if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) cb += 16;
                        snprintf(report, sizeof(report), "\x1B[M%c%c%c", (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
                    } else if (session->mouse.mode == MOUSE_TRACKING_X10 && pressed) {
                        int cb = 32 + i;
                        snprintf(report, sizeof(report), "\x1B[M%c%c%c", (char)cb, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
                    }
                    if (report[0]) KTerm_QueueResponse(term, report);
                }
            }

            // Motion Reporting
            if (session->mouse.mode == MOUSE_TRACKING_BTN_EVENT ||
                session->mouse.mode == MOUSE_TRACKING_ANY_EVENT) {

                bool any_button_down = current_buttons[0] || current_buttons[1] || current_buttons[2];
                bool report_motion = (session->mouse.mode == MOUSE_TRACKING_ANY_EVENT) ||
                                     (session->mouse.mode == MOUSE_TRACKING_BTN_EVENT && any_button_down);

                // Check if position changed
                if (report_motion && (global_cell_x != session->mouse.last_x || local_cell_y != session->mouse.last_y)) {
                    session->mouse.last_x = global_cell_x;
                    session->mouse.last_y = local_cell_y;

                    char report[64] = {0};
                    int btn_code = 32 + 35; // +32 for motion
                    // Add modifier bits
                    if (SituationIsKeyDown(SIT_KEY_LEFT_SHIFT) || SituationIsKeyDown(SIT_KEY_RIGHT_SHIFT)) btn_code += 4;
                    if (SituationIsKeyDown(SIT_KEY_LEFT_ALT) || SituationIsKeyDown(SIT_KEY_RIGHT_ALT)) btn_code += 8;
                    if (SituationIsKeyDown(SIT_KEY_LEFT_CONTROL) || SituationIsKeyDown(SIT_KEY_RIGHT_CONTROL)) btn_code += 16;

                    if (session->mouse.sgr_mode || session->mouse.mode == MOUSE_TRACKING_URXVT || session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                         int px = global_cell_x + 1;
                         int py = local_cell_y + 1;
                         if (session->mouse.mode == MOUSE_TRACKING_PIXEL) {
                            px = (int)mouse_pos.x + 1;
                            py = (int)mouse_pos.y + 1;
                         }
                         snprintf(report, sizeof(report), "\x1B[<%d;%d;%dM", btn_code - 32, px, py);
                    } else {
                         snprintf(report, sizeof(report), "\x1B[M%c%c%c", (char)btn_code, (char)(32 + global_cell_x + 1), (char)(32 + local_cell_y + 1));
                    }
                    if (report[0]) KTerm_QueueResponse(term, report);
                }
            }
        }
    }

    // Restore active session
    term->active_session = saved_session_idx;
}

#endif // KTERM_IO_SIT_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // KTERM_IO_SIT_H
