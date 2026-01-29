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

// Helper Macros for Safe String Handling
#ifndef SAFE_STRNCPY
#define SAFE_STRNCPY(dest, src, size) do { \
    if ((size) > 0) { \
        strncpy((dest), (src), (size) - 1); \
        (dest)[(size) - 1] = '\0'; \
    } \
} while(0)
#endif

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
    if (!term || !in) return;
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
    if (!term || !in) return;
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
    if (!params) return false;
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

    if (!params) return;

    KTermLexer lexer;
    KTerm_LexerInit(&lexer, params);
    KTermToken token = KTerm_LexerNext(&lexer);
    bool first_token = true;

    while (token.type != KT_TOK_EOF) {
        if (token.type == KT_TOK_IDENTIFIER) {
            char keyBuffer[64];
            int len = token.length < 63 ? token.length : 63;
            strncpy(keyBuffer, token.start, len);
            keyBuffer[len] = '\0';

            // Check for legacy positional flags "KERNED" or "FIXED" at start
            if (first_token) {
                if (KTerm_Strcasecmp(keyBuffer, "KERNED") == 0) {
                    options->kerned = true;
                    token = KTerm_LexerNext(&lexer);
                    if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
                    if (token.type != KT_TOK_EOF) {
                         const char* remainder = token.start;
                         SAFE_STRNCPY(options->text, remainder, sizeof(options->text));
                    }
                    return;
                } else if (KTerm_Strcasecmp(keyBuffer, "FIXED") == 0) {
                    options->kerned = false;
                    token = KTerm_LexerNext(&lexer);
                    if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
                    if (token.type != KT_TOK_EOF) {
                         const char* remainder = token.start;
                         SAFE_STRNCPY(options->text, remainder, sizeof(options->text));
                    }
                    return;
                }
            }

            // Normal processing
            KTermToken next = KTerm_LexerNext(&lexer);
            if (next.type == KT_TOK_EQUALS) {
                // Key=Val
                KTermToken val = KTerm_LexerNext(&lexer);
                char valBuffer[256];
                int vlen = val.length < 255 ? val.length : 255;
                strncpy(valBuffer, val.start, vlen);
                valBuffer[vlen] = '\0';

                if (KTerm_Strcasecmp(keyBuffer, "TEXT") == 0) {
                    SAFE_STRNCPY(options->text, valBuffer, sizeof(options->text));
                } else if (KTerm_Strcasecmp(keyBuffer, "FONT") == 0) {
                    SAFE_STRNCPY(options->font_name, valBuffer, sizeof(options->font_name));
                } else if (KTerm_Strcasecmp(keyBuffer, "ALIGN") == 0) {
                    if (KTerm_Strcasecmp(valBuffer, "CENTER") == 0) options->align = 1;
                    else if (KTerm_Strcasecmp(valBuffer, "RIGHT") == 0) options->align = 2;
                    else options->align = 0;
                } else if (KTerm_Strcasecmp(keyBuffer, "GRADIENT") == 0) {
                    // Check for composite value C1|C2
                    // First part is in valBuffer
                    // Peek next token
                    KTermToken sep = KTerm_LexerNext(&lexer);
                    if (sep.type == KT_TOK_UNKNOWN && sep.length == 1 && *sep.start == '|') {
                        KTermToken val2 = KTerm_LexerNext(&lexer);
                        char val2Buffer[64];
                        int v2len = val2.length < 63 ? val2.length : 63;
                        strncpy(val2Buffer, val2.start, v2len);
                        val2Buffer[v2len] = '\0';

                        if (KTerm_ParseColor(valBuffer, &options->gradient_start) &&
                            KTerm_ParseColor(val2Buffer, &options->gradient_end)) {
                            options->gradient_enabled = true;
                        }
                        // Consume next token (semicolon or eof)
                        token = KTerm_LexerNext(&lexer);
                    } else {
                        // Not composite or using quotes handled by valBuffer
                        char* pipesep = strchr(valBuffer, '|');
                        if (pipesep) {
                            *pipesep = '\0';
                            if (KTerm_ParseColor(valBuffer, &options->gradient_start) &&
                                KTerm_ParseColor(pipesep + 1, &options->gradient_end)) {
                                options->gradient_enabled = true;
                            }
                        }
                        token = sep;
                    }
                    if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
                    continue; // Loop
                } else if (KTerm_Strcasecmp(keyBuffer, "MODE") == 0) {
                     if (KTerm_Strcasecmp(valBuffer, "KERNED") == 0) options->kerned = true;
                }

                token = KTerm_LexerNext(&lexer); // Expecting Semicolon or EOF
                if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
            } else {
                // No Equals? Positional Text
                SAFE_STRNCPY(options->text, keyBuffer, sizeof(options->text));
                if (next.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
                else token = next;
            }
        } else if (token.type == KT_TOK_STRING) {
             // Positional Text
             char valBuffer[256];
             int vlen = token.length < 255 ? token.length : 255;
             strncpy(valBuffer, token.start, vlen);
             valBuffer[vlen] = '\0';
             SAFE_STRNCPY(options->text, valBuffer, sizeof(options->text));
             token = KTerm_LexerNext(&lexer);
             if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
        } else {
            // Skip
            token = KTerm_LexerNext(&lexer);
        }
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

    // Allocate Line Buffer on Heap (Hardening)
    size_t line_buffer_size = 32768; // 32KB
    char* line_buffer = (char*)malloc(line_buffer_size);
    if (!line_buffer) return;

    for (int y = 0; y < height; y++) {
        int line_pos = 0;
        line_buffer[0] = '\0';

        // Padding
        for(int p=0; p<padding; p++) {
             if (line_pos < (int)line_buffer_size - 1) line_buffer[line_pos++] = ' ';
        }
        line_buffer[line_pos] = '\0';

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

                if (line_pos + seq_len < (int)line_buffer_size) {
                    memcpy(line_buffer + line_pos, color_seq, seq_len);
                    line_pos += seq_len;
                    line_buffer[line_pos] = '\0';
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
                if (line_pos >= (int)line_buffer_size - 5) break;

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
                if (line_pos < (int)line_buffer_size - 1) line_buffer[line_pos++] = ' ';
            }
        }

        // Reset Color at end of line if gradient
        if (options->gradient_enabled) {
             const char* reset = "\x1B[0m";
             int r_len = strlen(reset);
             if (line_pos + r_len < (int)line_buffer_size) {
                 memcpy(line_buffer + line_pos, reset, r_len);
                 line_pos += r_len;
             }
        }

        line_buffer[line_pos] = '\0';
        KTerm_WriteString(term, line_buffer);
        KTerm_WriteString(term, "\r\n");
    }

    free(line_buffer);
}



void KTerm_GatewayProcess(KTerm* term, KTermSession* session, const char* class_id, const char* id, const char* command, const char* params) {
    // Input Hardening
    if (!term || !session || !class_id || !id || !command) return;
    if (!params) params = ""; // Treat NULL params as empty

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
            int s_idx = (int)strtol(params + 8, NULL, 10);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) {
                term->gateway_target_session = s_idx;
            }
            return;
        } else if (strncmp(params, "REGIS_SESSION;", 14) == 0) {
            int s_idx = (int)strtol(params + 14, NULL, 10);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) term->regis_target_session = s_idx;
            return;
        } else if (strncmp(params, "TEKTRONIX_SESSION;", 18) == 0) {
            int s_idx = (int)strtol(params + 18, NULL, 10);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) term->tektronix_target_session = s_idx;
            return;
        } else if (strncmp(params, "KITTY_SESSION;", 14) == 0) {
            int s_idx = (int)strtol(params + 14, NULL, 10);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) term->kitty_target_session = s_idx;
            return;
        } else if (strncmp(params, "SIXEL_SESSION;", 14) == 0) {
            int s_idx = (int)strtol(params + 14, NULL, 10);
            if (s_idx >= 0 && s_idx < MAX_SESSIONS) term->sixel_target_session = s_idx;
            return;
        } else if (strncmp(params, "ATTR;", 5) == 0) {
            // SET;ATTR;KEY=VAL;...
            KTermLexer lexer;
            KTerm_LexerInit(&lexer, params + 5);
            KTermToken token = KTerm_LexerNext(&lexer);

            while (token.type != KT_TOK_EOF) {
                if (token.type == KT_TOK_IDENTIFIER) {
                    char key[64];
                    int klen = token.length < 63 ? token.length : 63;
                    strncpy(key, token.start, klen);
                    key[klen] = '\0';

                    KTermToken next = KTerm_LexerNext(&lexer);
                    if (next.type == KT_TOK_EQUALS) {
                        KTermToken val = KTerm_LexerNext(&lexer);
                        int v = (val.type == KT_TOK_NUMBER) ? val.value.i : 0;

                        // Parse complex value if string/identifier
                        char valBuf[256] = {0};
                        if (val.type == KT_TOK_IDENTIFIER || val.type == KT_TOK_STRING || val.type == KT_TOK_NUMBER) {
                            int vlen = val.length < 255 ? val.length : 255;
                            strncpy(valBuf, val.start, vlen);
                            valBuf[vlen] = '\0'; // Ensure null termination
                            // Attempt to parse number from identifier or string
                            if (val.type != KT_TOK_NUMBER) {
                                char* endptr;
                                long parsed_v = strtol(valBuf, &endptr, 0);
                                if (endptr != valBuf) v = (int)parsed_v;
                            }
                        }

                        // Check for RGB components for UL/ST
                        bool is_rgb = false;
                        int r=0, g=0, b=0;
                        KTermToken lookahead = KTerm_LexerNext(&lexer);

                        if ((strcmp(key, "UL") == 0 || strcmp(key, "ST") == 0) && lookahead.type == KT_TOK_COMMA) {
                            // UL=R,G,B
                            r = v; // First val
                            KTermToken tok_g = KTerm_LexerNext(&lexer);
                            if (tok_g.type == KT_TOK_NUMBER || tok_g.type == KT_TOK_IDENTIFIER) {
                                if (tok_g.type==KT_TOK_NUMBER) g = tok_g.value.i;
                                else g = (int)strtol(tok_g.start, NULL, 10);
                            }
                            KTermToken sep2 = KTerm_LexerNext(&lexer); // Comma
                            KTermToken tok_b = KTerm_LexerNext(&lexer);
                            if (tok_b.type == KT_TOK_NUMBER || tok_b.type == KT_TOK_IDENTIFIER) {
                                if (tok_b.type==KT_TOK_NUMBER) b = tok_b.value.i;
                                else b = (int)strtol(tok_b.start, NULL, 10);
                            }
                            is_rgb = true;
                            token = KTerm_LexerNext(&lexer); // Next token (likely semicolon)
                        } else {
                            // Standard processing
                            if (lookahead.type == KT_TOK_SEMICOLON) {
                                token = KTerm_LexerNext(&lexer);
                            } else {
                                token = lookahead;
                            }
                        }

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
                            if (is_rgb) {
                                target_session->current_ul_color.color_mode = 1; // RGB
                                target_session->current_ul_color.value.rgb = (RGB_KTermColor){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
                            } else if (valBuf[0] && sscanf(valBuf, "%d,%d,%d", &r, &g, &b) == 3) { // Legacy Quoted RGB
                                target_session->current_ul_color.color_mode = 1; // RGB
                                target_session->current_ul_color.value.rgb = (RGB_KTermColor){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
                            } else {
                                target_session->current_ul_color.color_mode = 0;
                                target_session->current_ul_color.value.index = v & 0xFF;
                            }
                        } else if (strcmp(key, "ST") == 0) {
                            if (is_rgb) {
                                target_session->current_st_color.color_mode = 1; // RGB
                                target_session->current_st_color.value.rgb = (RGB_KTermColor){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
                            } else if (valBuf[0] && sscanf(valBuf, "%d,%d,%d", &r, &g, &b) == 3) {
                                target_session->current_st_color.color_mode = 1; // RGB
                                target_session->current_st_color.value.rgb = (RGB_KTermColor){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
                            } else {
                                target_session->current_st_color.color_mode = 0;
                                target_session->current_st_color.value.index = v & 0xFF;
                            }
                        }
                    } else {
                        token = next;
                    }
                } else {
                    token = KTerm_LexerNext(&lexer);
                }

                if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
            }
            return;
        } else if (strncmp(params, "KEYBOARD;", 9) == 0) {
            KTermLexer lexer;
            KTerm_LexerInit(&lexer, params + 9);
            KTermToken token = KTerm_LexerNext(&lexer);

            while (token.type != KT_TOK_EOF) {
                if (token.type == KT_TOK_IDENTIFIER) {
                    char key[64];
                    int klen = token.length < 63 ? token.length : 63;
                    strncpy(key, token.start, klen);
                    key[klen] = '\0';

                    KTermToken next = KTerm_LexerNext(&lexer);
                    if (next.type == KT_TOK_EQUALS) {
                        KTermToken val = KTerm_LexerNext(&lexer);
                        int v = (val.type == KT_TOK_NUMBER) ? val.value.i : 0;
                        if (val.type == KT_TOK_IDENTIFIER) {
                             if (KTerm_TokenIs(val, "HOST")) {
                                 if (strcmp(key, "REPEAT") == 0) target_session->input.use_software_repeat = false;
                             } else if (KTerm_TokenIs(val, "SOFTWARE")) {
                                 if (strcmp(key, "REPEAT") == 0) target_session->input.use_software_repeat = true;
                             }
                        }

                        if (strcmp(key, "REPEAT_RATE") == 0) {
                            if (v < 0) v = 0; if (v > 31) v = 31;
                            target_session->auto_repeat_rate = v;
                        } else if (strcmp(key, "DELAY") == 0) {
                            if (v < 0) v = 0;
                            target_session->auto_repeat_delay = v;
                        }
                        token = KTerm_LexerNext(&lexer);
                    } else token = next;
                } else token = KTerm_LexerNext(&lexer);

                if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
            }
            return;
        } else if (strncmp(params, "GRID;", 5) == 0) {
            KTermLexer lexer;
            KTerm_LexerInit(&lexer, params + 5);
            KTermToken token = KTerm_LexerNext(&lexer);

            while (token.type != KT_TOK_EOF) {
                if (token.type == KT_TOK_IDENTIFIER) {
                    if (KTerm_TokenIs(token, "ON")) target_session->grid_enabled = true;
                    else if (KTerm_TokenIs(token, "OFF")) target_session->grid_enabled = false;
                    else {
                        char key[32];
                        int klen = token.length < 31 ? token.length : 31;
                        strncpy(key, token.start, klen);
                        key[klen] = '\0';

                        KTermToken next = KTerm_LexerNext(&lexer);
                        if (next.type == KT_TOK_EQUALS) {
                            KTermToken val = KTerm_LexerNext(&lexer);
                            int v = (val.type == KT_TOK_NUMBER) ? val.value.i : 0;
                            if (v < 0) v = 0; if (v > 255) v = 255;

                            if (strcmp(key, "R") == 0) target_session->grid_color.r = (unsigned char)v;
                            else if (strcmp(key, "G") == 0) target_session->grid_color.g = (unsigned char)v;
                            else if (strcmp(key, "B") == 0) target_session->grid_color.b = (unsigned char)v;
                            else if (strcmp(key, "A") == 0) target_session->grid_color.a = (unsigned char)v;

                            token = KTerm_LexerNext(&lexer);
                        } else token = next;
                    }
                } else token = KTerm_LexerNext(&lexer);

                if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
            }
            return;
        } else if (strncmp(params, "CONCEAL;", 8) == 0) {
            KTermLexer lexer;
            KTerm_LexerInit(&lexer, params + 8);
            KTermToken token = KTerm_LexerNext(&lexer);
            if (token.type == KT_TOK_NUMBER) {
                if (token.value.i >= 0) target_session->conceal_char_code = (uint32_t)token.value.i;
            }
            return;
        } else if (strncmp(params, "BLINK;", 6) == 0) {
            KTermLexer lexer;
            KTerm_LexerInit(&lexer, params + 6);
            KTermToken token = KTerm_LexerNext(&lexer);

            while (token.type != KT_TOK_EOF) {
                if (token.type == KT_TOK_IDENTIFIER) {
                    char key[32];
                    int klen = token.length < 31 ? token.length : 31;
                    strncpy(key, token.start, klen);
                    key[klen] = '\0';

                    KTermToken next = KTerm_LexerNext(&lexer);
                    if (next.type == KT_TOK_EQUALS) {
                        KTermToken val = KTerm_LexerNext(&lexer);
                        int v = (val.type == KT_TOK_NUMBER) ? val.value.i : 0;
                        if (v > 0) {
                            if (strcmp(key, "FAST") == 0) target_session->fast_blink_rate = v;
                            else if (strcmp(key, "SLOW") == 0) target_session->slow_blink_rate = v;
                            else if (strcmp(key, "BG") == 0) target_session->bg_blink_rate = v;
                        }
                        token = KTerm_LexerNext(&lexer);
                    } else token = next;
                } else token = KTerm_LexerNext(&lexer);

                if (token.type == KT_TOK_SEMICOLON) token = KTerm_LexerNext(&lexer);
            }
            return;
        }

        // Params: PARAM;VALUE[;VALUE2]
        KTermLexer lexer;
        KTerm_LexerInit(&lexer, params);
        KTermToken paramTok = KTerm_LexerNext(&lexer);

        if (paramTok.type == KT_TOK_IDENTIFIER) {
            char param[64];
            int plen = paramTok.length < 63 ? paramTok.length : 63;
            strncpy(param, paramTok.start, plen);
            param[plen] = '\0';

            KTermToken sep = KTerm_LexerNext(&lexer);
            if (sep.type == KT_TOK_SEMICOLON) {
                KTermToken valTok = KTerm_LexerNext(&lexer);
                char val[256] = {0};
                if (valTok.type != KT_TOK_EOF && valTok.type != KT_TOK_SEMICOLON) {
                    int vlen = valTok.length < 255 ? valTok.length : 255;
                    strncpy(val, valTok.start, vlen);
                    val[vlen] = '\0';
                }

                if (strcmp(param, "LEVEL") == 0) {
                    int level = (valTok.type == KT_TOK_NUMBER) ? valTok.value.i : (int)strtol(val, NULL, 10);
                    if (strcmp(val, "XTERM") == 0) level = VT_LEVEL_XTERM;
                    KTerm_SetLevel(term, target_session, (VTLevel)level);
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
                } else if (strcmp(param, "WIDTH") == 0) {
                    int cols = (valTok.type == KT_TOK_NUMBER) ? valTok.value.i : (int)strtol(val, NULL, 10);
                    if (cols > 0) {
                        if (cols > KTERM_MAX_COLS) cols = KTERM_MAX_COLS;
                        KTerm_Resize(term, cols, term->height);
                    }
                    return;
                } else if (strcmp(param, "HEIGHT") == 0) {
                    int rows = (valTok.type == KT_TOK_NUMBER) ? valTok.value.i : (int)strtol(val, NULL, 10);
                    if (rows > 0) {
                        if (rows > KTERM_MAX_ROWS) rows = KTERM_MAX_ROWS;
                        KTerm_Resize(term, term->width, rows);
                    }
                    return;
                } else if (strcmp(param, "SIZE") == 0) {
                    // Expecting second value: SIZE;W;H
                    KTermToken sep2 = KTerm_LexerNext(&lexer);
                    if (sep2.type == KT_TOK_SEMICOLON) {
                        KTermToken val2Tok = KTerm_LexerNext(&lexer);
                        int cols = (valTok.type == KT_TOK_NUMBER) ? valTok.value.i : (int)strtol(val, NULL, 10);

                        char val2[256] = {0};
                        if (val2Tok.type != KT_TOK_EOF) {
                             int v2len = val2Tok.length < 255 ? val2Tok.length : 255;
                             strncpy(val2, val2Tok.start, v2len);
                             val2[v2len] = '\0';
                        }
                        int rows = (val2Tok.type == KT_TOK_NUMBER) ? val2Tok.value.i : (int)strtol(val2, NULL, 10);

                        if (cols > 0 && rows > 0) {
                            if (cols > KTERM_MAX_COLS) cols = KTERM_MAX_COLS;
                            if (rows > KTERM_MAX_ROWS) rows = KTERM_MAX_ROWS;
                            KTerm_Resize(term, cols, rows);
                        }
                    }
                    return;
                }
            }
        }
    } else if (strcmp(command, "PIPE") == 0) {
        if (KTerm_Gateway_HandlePipe(term, target_session, id, params)) {
            return;
        }

        if (strncmp(params, "BANNER;", 7) == 0) {
            BannerOptions options;
            KTerm_ProcessBannerOptions(params + 7, &options);
            KTerm_GenerateBanner(term, target_session, &options);
            return;
        }
    } else if (strcmp(command, "INIT") == 0) {
        if (strcmp(params, "REGIS_SESSION") == 0) {
             int s_idx = -1;
             for(int i=0; i<MAX_SESSIONS; i++) if(&term->sessions[i] == session) { s_idx = i; break; }
             if (s_idx != -1) {
                 term->regis_target_session = s_idx;
                 KTerm_InitReGIS(term);
             }
             return;
        } else if (strcmp(params, "TEKTRONIX_SESSION") == 0) {
             int s_idx = -1;
             for(int i=0; i<MAX_SESSIONS; i++) if(&term->sessions[i] == session) { s_idx = i; break; }
             if (s_idx != -1) {
                 term->tektronix_target_session = s_idx;
                 KTerm_InitTektronix(term);
             }
             return;
        } else if (strcmp(params, "KITTY_SESSION") == 0) {
             int s_idx = -1;
             for(int i=0; i<MAX_SESSIONS; i++) if(&term->sessions[i] == session) { s_idx = i; break; }
             if (s_idx != -1) {
                 term->kitty_target_session = s_idx;
                 KTerm_InitKitty(session);
             }
             return;
        } else if (strcmp(params, "SIXEL_SESSION") == 0) {
             int s_idx = -1;
             for(int i=0; i<MAX_SESSIONS; i++) if(&term->sessions[i] == session) { s_idx = i; break; }
             if (s_idx != -1) {
                 term->sixel_target_session = s_idx;
                 KTerm_InitSixelGraphics(term, session);
             }
             return;
        }
    } else if (strcmp(command, "RESET") == 0) {
        if (strcmp(params, "GRAPHICS") == 0 || strcmp(params, "ALL_GRAPHICS") == 0) {
            KTerm_ResetGraphics(term, target_session, GRAPHICS_RESET_ALL);
            return;
        } else if (strcmp(params, "KITTY") == 0) {
            KTerm_ResetGraphics(term, target_session, GRAPHICS_RESET_KITTY);
            return;
        } else if (strcmp(params, "REGIS") == 0) {
            KTerm_ResetGraphics(term, target_session, GRAPHICS_RESET_REGIS);
            return;
        } else if (strcmp(params, "TEK") == 0 || strcmp(params, "TEKTRONIX") == 0) {
            KTerm_ResetGraphics(term, target_session, GRAPHICS_RESET_TEK);
            return;
        } else if (strcmp(params, "SIXEL") == 0) {
            KTerm_ResetGraphics(term, target_session, GRAPHICS_RESET_SIXEL);
            return;
        } else if (strcmp(params, "SESSION") == 0) {
             term->gateway_target_session = -1;
             return;
        } else if (strcmp(params, "REGIS_SESSION") == 0) {
             term->regis_target_session = -1;
             return;
        } else if (strcmp(params, "TEKTRONIX_SESSION") == 0) {
             term->tektronix_target_session = -1;
             return;
        } else if (strcmp(params, "KITTY_SESSION") == 0) {
             term->kitty_target_session = -1;
             return;
        } else if (strcmp(params, "SIXEL_SESSION") == 0) {
             term->sixel_target_session = -1;
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
        } else if (strncmp(params, "TABS", 4) == 0) {
             // RESET;TABS;DEFAULT8
             if (strcmp(params, "TABS;DEFAULT8") == 0) {
                 KTerm_ClearAllTabStops(term);
                 for (int i = 8; i < term->width; i += 8) {
                     KTerm_SetTabStop(term, i);
                 }
             } else {
                 KTerm_ClearAllTabStops(term);
             }
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
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;VERSION=%d.%d.%d\x1B\\", id, KTERM_VERSION_MAJOR, KTERM_VERSION_MINOR, KTERM_VERSION_PATCH);
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "OUTPUT") == 0) {
            char response[256];
            snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;OUTPUT=%d\x1B\\", id, target_session->response_enabled ? 1 : 0);
            KTerm_QueueResponse(term, response);
            return;
        } else if (strcmp(params, "FONTS") == 0) {
             // Safe bounded concatenation for fonts
             char response[4096]; // Increased buffer
             snprintf(response, sizeof(response), "\x1BPGATE;KTERM;%s;REPORT;FONTS=", id);
             size_t current_len = strlen(response);

             for (int i=0; available_fonts[i].name != NULL; i++) {
                 size_t name_len = strlen(available_fonts[i].name);
                 size_t remaining = sizeof(response) - current_len - 5; // Reserve space for , and ST
                 if (remaining > name_len) {
                     strcat(response, available_fonts[i].name);
                     current_len += name_len;
                     if (available_fonts[i+1].name != NULL) {
                         strcat(response, ",");
                         current_len++;
                     }
                 } else {
                     break; // Truncate if full
                 }
             }
             // Ensure termination sequence
             if (current_len < sizeof(response) - 3) {
                 strcat(response, "\x1B\\");
             }
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
