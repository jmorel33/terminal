#ifndef KT_GATEWAY_H
#define KT_GATEWAY_H

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations to avoid including kterm.h if possible,
// but kterm.h types are required.
// We assume kterm.h is included before this file or this is included BY kterm.h.
#ifndef KTERM_H
// If not included via kterm.h, we might need types.
// But this module is designed to be embedded.
#endif

// Gateway Protocol Entry Point
// Parses and executes Gateway commands (DCS GATE ...)
// Format: DCS GATE <Class>;<ID>;<Command>[;<Params>] ST
// Example: DCS GATE MAT;1;SET;COLOR;RED ST
// This function replaces the internal handling in the main parser.
void KTerm_GatewayProcess(KTerm* term, KTermSession* session, const char* class_id, const char* id, const char* command, const char* params);

#ifdef __cplusplus
}
#endif

#endif // KT_GATEWAY_H

#ifdef KTERM_GATEWAY_IMPLEMENTATION

// Internal Structures
typedef struct {
    char text[256];
    char font_name[64];
    bool kerned;
    int align; // 0=LEFT, 1=CENTER, 2=RIGHT
    RGB_KTermColor gradient_start;
    RGB_KTermColor gradient_end;
    bool gradient_enabled;
} BannerOptions;

// Internal Helper Declarations
static bool KTerm_ParseColor(const char* str, RGB_KTermColor* color);
static void KTerm_ProcessBannerOptions(const char* params, BannerOptions* options);
static void KTerm_GenerateBanner(KTerm* term, KTermSession* session, const BannerOptions* options);

// VT Pipe Helpers
static int KTerm_Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static void KTerm_Base64StreamDecode(KTerm* term, int session_idx, const char* in) {
    size_t len = strlen(in);
    unsigned int val = 0;
    int valb = -8;
    for (size_t i = 0; i < len; i++) {
        if (in[i] == '=') break;
        int c = KTerm_Base64Value(in[i]);
        if (c == -1) continue;
        val = (val << 6) + c;
        valb += 6;
        if (valb >= 0) {
            KTerm_WriteCharToSession(term, session_idx, (unsigned char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
}

static int KTerm_HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void KTerm_HexStreamDecode(KTerm* term, int session_idx, const char* in) {
    size_t len = strlen(in);
    for (size_t i = 0; i < len; i += 2) {
        if (i + 1 >= len) break;
        int h1 = KTerm_HexValue(in[i]);
        int h2 = KTerm_HexValue(in[i+1]);
        if (h1 != -1 && h2 != -1) {
            KTerm_WriteCharToSession(term, session_idx, (unsigned char)((h1 << 4) | h2));
        }
    }
}

static bool KTerm_Gateway_HandlePipe(KTerm* term, KTermSession* session, const char* id, const char* params) {
    if (strncmp(params, "VT;", 3) != 0) return false;

    const char* encoding_start = params + 3;
    const char* payload_start = strchr(encoding_start, ';');
    if (!payload_start) return false;

    char encoding[16];
    size_t enc_len = (size_t)(payload_start - encoding_start);
    if (enc_len >= sizeof(encoding)) enc_len = sizeof(encoding) - 1;
    strncpy(encoding, encoding_start, enc_len);
    encoding[enc_len] = '\0';

    const char* payload = payload_start + 1;

    int session_idx = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (&term->sessions[i] == session) {
            session_idx = i;
            break;
        }
    }

    if (session_idx != -1) {
        if (KTerm_Strcasecmp(encoding, "B64") == 0) {
            KTerm_Base64StreamDecode(term, session_idx, payload);
        } else if (KTerm_Strcasecmp(encoding, "HEX") == 0) {
            KTerm_HexStreamDecode(term, session_idx, payload);
        } else if (KTerm_Strcasecmp(encoding, "RAW") == 0) {
            // RAW is simple injection
            for (size_t i = 0; payload[i] != '\0'; i++) {
                KTerm_WriteCharToSession(term, session_idx, (unsigned char)payload[i]);
            }
        }
    }

    return true;
}


static bool KTerm_ParseColor(const char* str, RGB_KTermColor* color) {
    if (!str || !color) return false;

    if (str[0] == '#') {
        unsigned int r, g, b;
        if (sscanf(str + 1, "%02x%02x%02x", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            color->a = 255;
            return true;
        }
    } else {
        int r, g, b;
        if (sscanf(str, "%d,%d,%d", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            color->a = 255;
            return true;
        }
    }

    // Try to parse named colors
    for (int i = 0; i < 16; i++) {
        // Simple named colors map could be added here, but for now rely on RGB/Hex
    }
    return false;
}

static void KTerm_ProcessBannerOptions(const char* params, BannerOptions* options) {
    // Default values
    memset(options, 0, sizeof(BannerOptions));
    options->align = 0; // LEFT
    options->kerned = false;
    options->gradient_enabled = false;

    char buffer[512];
    strncpy(buffer, params, sizeof(buffer)-1);
    buffer[511] = '\0';

    // Check for Legacy Format (FIXED;... or KERNED;...)
    // We check the first token.
    char legacy_check[64];
    const char* first_semi = strchr(params, ';');
    size_t token_len = first_semi ? (size_t)(first_semi - params) : strlen(params);
    if (token_len < sizeof(legacy_check)) {
        strncpy(legacy_check, params, token_len);
        legacy_check[token_len] = '\0';

        bool is_legacy = false;
        if (KTerm_Strcasecmp(legacy_check, "FIXED") == 0) {
            options->kerned = false;
            is_legacy = true;
        } else if (KTerm_Strcasecmp(legacy_check, "KERNED") == 0) {
            options->kerned = true;
            is_legacy = true;
        }

        if (is_legacy) {
            if (first_semi) {
                strncpy(options->text, first_semi + 1, sizeof(options->text)-1);
            }
            return;
        }
    }

    char* token = strtok(buffer, ";");
    while (token) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char* key = token;
            char* val = eq + 1;

            if (KTerm_Strcasecmp(key, "TEXT") == 0) {
                strncpy(options->text, val, sizeof(options->text)-1);
            } else if (KTerm_Strcasecmp(key, "FONT") == 0) {
                strncpy(options->font_name, val, sizeof(options->font_name)-1);
            } else if (KTerm_Strcasecmp(key, "ALIGN") == 0) {
                if (KTerm_Strcasecmp(val, "CENTER") == 0) options->align = 1;
                else if (KTerm_Strcasecmp(val, "RIGHT") == 0) options->align = 2;
                else options->align = 0;
            } else if (KTerm_Strcasecmp(key, "GRADIENT") == 0) {
                char* sep = strchr(val, '|');
                if (sep) {
                    *sep = '\0';
                    if (KTerm_ParseColor(val, &options->gradient_start) &&
                        KTerm_ParseColor(sep + 1, &options->gradient_end)) {
                        options->gradient_enabled = true;
                    }
                }
            } else if (KTerm_Strcasecmp(key, "MODE") == 0) {
                 if (KTerm_Strcasecmp(val, "KERNED") == 0) options->kerned = true;
            }
        } else {
            // Positional argument fallback (treated as text if not legacy keyword)
            strncpy(options->text, token, sizeof(options->text)-1);
        }
        token = strtok(NULL, ";");
    }
}


static void KTerm_GenerateBanner(KTerm* term, KTermSession* session, const BannerOptions* options) {
    const char* text = options->text;
    if (!text || strlen(text) == 0) return;
    int len = strlen(text);

    // Default Font
    const uint8_t* font_data = (const uint8_t*)term->current_font_data;
    bool is_16bit = term->current_font_is_16bit;
    int height = term->font_data_height;
    int width = term->font_data_width;

    KTermFontMetric temp_metrics[256];
    bool use_temp_metrics = false;

    // Check soft font
    if (session->soft_font.active) {
        font_data = (const uint8_t*)session->soft_font.font_data;
        height = session->soft_font.char_height;
        width = session->soft_font.char_width;
        is_16bit = (width > 8);
    }

    // Check requested font overrides
    if (strlen(options->font_name) > 0) {
        for (int i = 0; available_fonts[i].name != NULL; i++) {
             if (KTerm_Strcasecmp(available_fonts[i].name, options->font_name) == 0) {
                 font_data = (const uint8_t*)available_fonts[i].data;
                 width = available_fonts[i].data_width;
                 height = available_fonts[i].data_height;
                 is_16bit = available_fonts[i].is_16bit;

                 if (options->kerned) {
                      KTerm_CalculateFontMetrics(font_data, 256, width, height, 0, is_16bit, temp_metrics);
                      use_temp_metrics = true;
                 }
                 break;
             }
        }
    }

    // Calculate total width for alignment
    int total_width = 0;
    if (options->align != 0) {
        for (int i = 0; i < len; i++) {
            unsigned char c = (unsigned char)text[i];
            int w = width;
            if (options->kerned) {
                KTermFontMetric* m;
                if (use_temp_metrics) m = &temp_metrics[c];
                else if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) m = &session->soft_font.metrics[c];
                else m = &term->font_metrics[c];

                if (m->end_x >= m->begin_x) {
                     w = m->end_x - m->begin_x + 1;
                } else if (c == ' ') {
                     w = width / 2;
                } else {
                     w = 0;
                }
                // Kerning adds 1 space
                 if (w > 0) w++;
            }
            total_width += w;
        }
    }

    int padding = 0;
    if (options->align == 1) { // Center
        padding = (term->width - total_width) / 2;
    } else if (options->align == 2) { // Right
        padding = term->width - total_width;
    }
    if (padding < 0) padding = 0;

    for (int y = 0; y < height; y++) {
        char line_buffer[16384];
        int line_pos = 0;

        // Padding
        for(int p=0; p<padding; p++) {
             if (line_pos < (int)sizeof(line_buffer)-1) line_buffer[line_pos++] = ' ';
        }

        for (int i = 0; i < len; i++) {
            unsigned char c = (unsigned char)text[i];

            // Apply Gradient Color
            if (options->gradient_enabled) {
                float t = 0.0f;
                if (len > 1) t = (float)i / (float)(len - 1);

                unsigned char r = (unsigned char)(options->gradient_start.r + (options->gradient_end.r - options->gradient_start.r) * t);
                unsigned char g = (unsigned char)(options->gradient_start.g + (options->gradient_end.g - options->gradient_start.g) * t);
                unsigned char b = (unsigned char)(options->gradient_start.b + (options->gradient_end.b - options->gradient_start.b) * t);

                char color_seq[32];
                snprintf(color_seq, sizeof(color_seq), "\x1B[38;2;%d;%d;%dm", r, g, b);
                int seq_len = strlen(color_seq);
                if (line_pos + seq_len < (int)sizeof(line_buffer)) {
                    strcpy(line_buffer + line_pos, color_seq);
                    line_pos += seq_len;
                }
            }

            // Get Glyph Row Data
            uint32_t row_data = 0;

            if (is_16bit) {
                if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) {
                     uint8_t b1 = session->soft_font.font_data[c][y * 2];
                     uint8_t b2 = session->soft_font.font_data[c][y * 2 + 1];
                     row_data = (b1 << 8) | b2;
                } else {
                    const uint16_t* font_data16 = (const uint16_t*)font_data;
                    row_data = font_data16[c * height + y];
                }
            } else {
                if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) {
                    row_data = session->soft_font.font_data[c][y];
                } else {
                    row_data = font_data[c * height + y];
                }
            }

            // Determine render range
            int start_x = 0;
            int end_x = width - 1;

            if (options->kerned) {
                KTermFontMetric* m;
                if (use_temp_metrics) m = &temp_metrics[c];
                else if (session->soft_font.active && font_data == (const uint8_t*)session->soft_font.font_data) m = &session->soft_font.metrics[c];
                else m = &term->font_metrics[c];

                if (m->end_x >= m->begin_x) {
                    start_x = m->begin_x;
                    end_x = m->end_x;
                } else {
                    if (c == ' ') {
                        start_x = 0;
                        end_x = width / 2;
                    } else {
                         start_x = 0; end_x = -1; // Skip
                    }
                }
            }

            // Append bits
            for (int x = start_x; x <= end_x; x++) {
                if (line_pos >= (int)sizeof(line_buffer) - 5) break;

                bool bit_set = false;
                if ((row_data >> (width - 1 - x)) & 1) {
                     bit_set = true;
                }

                if (bit_set) {
                    line_buffer[line_pos++] = '\xE2';
                    line_buffer[line_pos++] = '\x96';
                    line_buffer[line_pos++] = '\x88';
                } else {
                    line_buffer[line_pos++] = ' ';
                }
            }

            // Spacing
            if (options->kerned) {
                if (line_pos < (int)sizeof(line_buffer) - 1) line_buffer[line_pos++] = ' ';
            }
        }

        // Reset Color at end of line if gradient
        if (options->gradient_enabled) {
             const char* reset = "\x1B[0m";
             if (line_pos + strlen(reset) < sizeof(line_buffer)) {
                 strcpy(line_buffer + line_pos, reset);
                 line_pos += strlen(reset);
             }
        }

        line_buffer[line_pos] = '\0';
        KTerm_WriteString(term, line_buffer);
        KTerm_WriteString(term, "\r\n");
    }
}



void KTerm_GatewayProcess(KTerm* term, KTermSession* session, const char* class_id, const char* id, const char* command, const char* params) {
    // Determine Target Session
    KTermSession* target_session = session;
    if (term->gateway_target_session >= 0 && term->gateway_target_session < MAX_SESSIONS) {
        target_session = &term->sessions[term->gateway_target_session];
    }

    // Check Class ID
    if (strcmp(class_id, "KTERM") != 0) {
        goto call_user_callback;
    }

    // Original KTerm Internal Logic
    if (strcmp(command, "SET") == 0) {
        if (strncmp(params, "SESSION;", 8) == 0) {
            // SET;SESSION;ID
            int s_idx = atoi(params + 8);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) {
                term->gateway_target_session = s_idx;
            }
            return;
        } else if (strncmp(params, "ATTR;", 5) == 0) {
            // SET;ATTR;KEY=VAL;...
            char attr_buffer[256];
            strncpy(attr_buffer, params + 5, sizeof(attr_buffer)-1);
            attr_buffer[255] = '\0';

            char* token = strtok(attr_buffer, ";");
            while (token) {
                char* eq = strchr(token, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = token;
                    char* val = eq + 1;
                    int v = atoi(val);

                    if (strcmp(key, "BOLD") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_BOLD;
                        else target_session->current_attributes &= ~KTERM_ATTR_BOLD;
                    } else if (strcmp(key, "DIM") == 0) {
                         if (v) target_session->current_attributes |= KTERM_ATTR_FAINT;
                         else target_session->current_attributes &= ~KTERM_ATTR_FAINT;
                    } else if (strcmp(key, "ITALIC") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_ITALIC;
                        else target_session->current_attributes &= ~KTERM_ATTR_ITALIC;
                    } else if (strcmp(key, "UNDERLINE") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_UNDERLINE;
                        else target_session->current_attributes &= ~KTERM_ATTR_UNDERLINE;
                    } else if (strcmp(key, "BLINK") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_BLINK;
                        else target_session->current_attributes &= ~KTERM_ATTR_BLINK;
                    } else if (strcmp(key, "REVERSE") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_REVERSE;
                        else target_session->current_attributes &= ~KTERM_ATTR_REVERSE;
                    } else if (strcmp(key, "HIDDEN") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_CONCEAL;
                        else target_session->current_attributes &= ~KTERM_ATTR_CONCEAL;
                    } else if (strcmp(key, "STRIKE") == 0) {
                        if (v) target_session->current_attributes |= KTERM_ATTR_STRIKE;
                        else target_session->current_attributes &= ~KTERM_ATTR_STRIKE;
                    } else if (strcmp(key, "FG") == 0) {
                        target_session->current_fg.color_mode = 0;
                        target_session->current_fg.value.index = v & 0xFF;
                    } else if (strcmp(key, "BG") == 0) {
                        target_session->current_bg.color_mode = 0;
                        target_session->current_bg.value.index = v & 0xFF;
                    } else if (strcmp(key, "UL") == 0) {
                        // Simple parser for R,G,B or Index
                        int r, g, b;
                        if (sscanf(val, "%d,%d,%d", &r, &g, &b) == 3) {
                            target_session->current_ul_color.color_mode = 1; // RGB
                            target_session->current_ul_color.value.rgb = (RGB_KTermColor){r, g, b, 255};
                        } else {
                            target_session->current_ul_color.color_mode = 0;
                            target_session->current_ul_color.value.index = v & 0xFF;
                        }
                    } else if (strcmp(key, "ST") == 0) {
                        int r, g, b;
                        if (sscanf(val, "%d,%d,%d", &r, &g, &b) == 3) {
                            target_session->current_st_color.color_mode = 1; // RGB
                            target_session->current_st_color.value.rgb = (RGB_KTermColor){r, g, b, 255};
                        } else {
                            target_session->current_st_color.color_mode = 0;
                            target_session->current_st_color.value.index = v & 0xFF;
                        }
                    }
                }
                token = strtok(NULL, ";");
            }
            return;
        } else if (strncmp(params, "GRID;", 5) == 0) {
            // SET;GRID;ON;R=255;G=0;...
            char grid_buffer[256];
            strncpy(grid_buffer, params + 5, sizeof(grid_buffer)-1);
            grid_buffer[255] = '\0';

            char* token = strtok(grid_buffer, ";");
            while (token) {
                if (strcmp(token, "ON") == 0) {
                    target_session->grid_enabled = true;
                } else if (strcmp(token, "OFF") == 0) {
                    target_session->grid_enabled = false;
                } else {
                    char* eq = strchr(token, '=');
                    if (eq) {
                        *eq = '\0';
                        char* key = token;
                        int v = atoi(eq + 1);
                        if (v < 0) v = 0; if (v > 255) v = 255;

                        if (strcmp(key, "R") == 0) target_session->grid_color.r = v;
                        else if (strcmp(key, "G") == 0) target_session->grid_color.g = v;
                        else if (strcmp(key, "B") == 0) target_session->grid_color.b = v;
                        else if (strcmp(key, "A") == 0) target_session->grid_color.a = v;
                    }
                }
                token = strtok(NULL, ";");
            }
            return;
        } else if (strncmp(params, "CONCEAL;", 8) == 0) {
            // SET;CONCEAL;VALUE
            const char* val_str = params + 8;
            int val = atoi(val_str);
            if (val >= 0) target_session->conceal_char_code = (uint32_t)val;
            return;
        } else if (strncmp(params, "BLINK;", 6) == 0) {
            // SET;BLINK;FAST=slot;SLOW=slot;BG=slot
            char blink_buffer[256];
            strncpy(blink_buffer, params + 6, sizeof(blink_buffer)-1);
            blink_buffer[255] = '\0';

            char* token = strtok(blink_buffer, ";");
            while (token) {
                char* eq = strchr(token, '=');
                if (eq) {
                    *eq = '\0';
                    char* key = token;
                    int v = atoi(eq + 1);
                    if (v > 0) {
                        if (strcmp(key, "FAST") == 0) target_session->fast_blink_rate = v;
                        else if (strcmp(key, "SLOW") == 0) target_session->slow_blink_rate = v;
                        else if (strcmp(key, "BG") == 0) target_session->bg_blink_rate = v;
                    }
                }
                token = strtok(NULL, ";");
            }
            return;
        }

        // Params: PARAM;VALUE
        char p_buffer[256];
        strncpy(p_buffer, params, sizeof(p_buffer)-1);
        p_buffer[255] = '\0';

        char* param = p_buffer;
        char* val = strchr(param, ';');
        char* val2 = NULL;
        if (val) {
            *val = '\0';
            val++;
            val2 = strchr(val, ';');
            if (val2) {
                *val2 = '\0';
                val2++;
            }
        }

        if (param && val) {
            if (strcmp(param, "LEVEL") == 0) {
                int level = atoi(val);
                if (strcmp(val, "XTERM") == 0) level = VT_LEVEL_XTERM;
                KTerm_SetLevel(term, (VTLevel)level);
                return;
            } else if (strcmp(param, "DEBUG") == 0) {
                bool enable = (strcmp(val, "ON") == 0 || strcmp(val, "1") == 0 || strcmp(val, "TRUE") == 0);
                KTerm_EnableDebug(term, enable);
                return;
            } else if (strcmp(param, "OUTPUT") == 0) {
                bool enable = (strcmp(val, "ON") == 0 || strcmp(val, "1") == 0 || strcmp(val, "TRUE") == 0);
                target_session->response_enabled = enable;
                return;
            } else if (strcmp(param, "FONT") == 0) {
                KTerm_SetFont(term, val);
                return;
            } else if (strcmp(param, "SIZE") == 0 && val2) {
                int cols = atoi(val);
                int rows = atoi(val2);
                if (cols > 0 && rows > 0) {
                    KTerm_Resize(term, cols, rows);
                }
                return;
            }
        }
    } else if (strcmp(command, "PIPE") == 0) {
        // 1. Check for VT Pipe
        if (KTerm_Gateway_HandlePipe(term, target_session, id, params)) {
            return;
        }

        // PIPE;BANNER;...
        if (strncmp(params, "BANNER;", 7) == 0) {
            BannerOptions options;
            KTerm_ProcessBannerOptions(params + 7, &options);
            KTerm_GenerateBanner(term, target_session, &options);
            return;
        }
    } else if (strcmp(command, "RESET") == 0) {
        if (strcmp(params, "SESSION") == 0) {
             term->gateway_target_session = -1;
             return;
        } else if (strcmp(params, "ATTR") == 0) {
             // Reset to default attributes (White on Black, no flags)
             target_session->current_attributes = 0;
             target_session->current_fg.color_mode = 0;
             target_session->current_fg.value.index = COLOR_WHITE;
             target_session->current_bg.color_mode = 0;
             target_session->current_bg.value.index = COLOR_BLACK;
             return;
        } else if (strcmp(params, "BLINK") == 0) {
             target_session->fast_blink_rate = 255;
             target_session->slow_blink_rate = 500;
             target_session->bg_blink_rate = 500;
             return;
        }
    } else if (strcmp(command, "GET") == 0) {
        // Params: PARAM
        if (strcmp(params, "LEVEL") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;LEVEL=%d\x1B\\", id, KTerm_GetLevel(term));
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "VERSION") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;VERSION=3.3.11\x1B\\", id);
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "OUTPUT") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;OUTPUT=%d\x1B\\", id, target_session->response_enabled ? 1 : 0);
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "FONTS") == 0) {
             char response[1024];
             snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;FONTS=", id);
             for (int i=0; available_fonts[i].name != NULL; i++) {
                 strncat(response, available_fonts[i].name, sizeof(response) - strlen(response) - 2);
                 if (available_fonts[i+1].name != NULL) strncat(response, ",", sizeof(response) - strlen(response) - 1);
             }
             strncat(response, "\x1B\\", sizeof(response) - strlen(response) - 1);
             KTerm_QueueResponse(term, response);
             return;
        } else if (strcmp(params, "UNDERLINE_COLOR") == 0) {
            char response[256];
            if (session->current_ul_color.color_mode == 1) {
                RGB_KTermColor c = session->current_ul_color.value.rgb;
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=%d,%d,%d\x1B\\", id, c.r, c.g, c.b);
            } else if (session->current_ul_color.color_mode == 2) {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=DEFAULT\x1B\\", id);
            } else {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;UNDERLINE_COLOR=%d\x1B\\", id, session->current_ul_color.value.index);
            }
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "STRIKE_COLOR") == 0) {
            char response[256];
            if (session->current_st_color.color_mode == 1) {
                RGB_KTermColor c = session->current_st_color.value.rgb;
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=%d,%d,%d\x1B\\", id, c.r, c.g, c.b);
            } else if (session->current_st_color.color_mode == 2) {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=DEFAULT\x1B\\", id);
            } else {
                snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;STRIKE_COLOR=%d\x1B\\", id, session->current_st_color.value.index);
            }
            KTerm_QueueResponse(term, response);
            return;
        }
    }

    return; // Consumed but unknown command for KTERM class

call_user_callback:
    if (term->gateway_callback) {
        term->gateway_callback(term, class_id, id, command, params);
    } else {
        KTerm_LogUnsupportedSequence(term, "Unknown Gateway Command");
    }
}

#endif // KTERM_GATEWAY_IMPLEMENTATION
