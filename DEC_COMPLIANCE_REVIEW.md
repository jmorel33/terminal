# KTerm v2.2.13 - DEC Command Sequence Compliance Review

## Overview
This document provides a detailed review of the DEC (Digital Equipment Corporation) command sequence support in KTerm v2.2.13. The review covers Private Modes, Control Sequences, Graphics protocols, and device reporting.

## 1. DEC Private Modes (DECSET/DECRST)
Managed via `CSI ? Pm h` (Set) and `CSI ? Pm l` (Reset).

| Mode | Name | Status | Notes |
| :--- | :--- | :--- | :--- |
| **1** | **DECCKM** (Cursor Keys) | ✅ Supported | Switching between Application/Normal Cursor Keys. |
| **2** | **DECANM** (ANSI/VT52) | ❌ Missing | VT52 mode is supported via separate parser state, but explicit switching via Mode 2 is not evident in `ExecuteSM`. |
| **3** | **DECCOLM** (Column Mode) | ✅ Supported | Resizes internal buffer to 80 or 132 columns. Triggers clear screen/reset margins unless Mode 95 is active. |
| **4** | **DECSCLM** (Scroll Mode) | ✅ Supported | Toggles smooth scrolling flag. |
| **5** | **DECSCNM** (Screen Mode) | ✅ Supported | Toggles reverse video (screen inversion). |
| **6** | **DECOM** (Origin Mode) | ✅ Supported | Cursor addressing relative to margins. |
| **7** | **DECAWM** (Auto Wrap) | ✅ Supported | Toggles auto-wrap behavior. |
| **8** | **DECARM** (Auto Repeat) | ✅ Supported | Toggles key auto-repeat. |
| **9** | **X10 Mouse** | ✅ Supported | Legacy X10 mouse tracking. |
| **12** | **Send/Receive** | ✅ Supported | Toggles Local Echo (`SRM` inverted logic in implementation). |
| **25** | **DECTCEM** (Cursor) | ✅ Supported | Show/Hide Cursor. |
| **38** | **DECTEK** (Tektronix) | ✅ Supported | Enters Tektronix 4010/4014 emulation mode. |
| **40** | **Allow 80/132** | ⚠️ Stubbed | Logging only. Does not enable resizing capability. |
| **47** | **Alternate Screen** | ✅ Supported | Legacy buffer switch. |
| **66** | **DECNKM** (Keypad) | ✅ Supported | Explicitly toggles `keypad_application_mode`. |
| **67** | **DECBKM** (Backarrow) | ❌ Missing | Backspace/Delete mapping configuration via Mode 67 is missing. |
| **69** | **DECLRMM** (Margins) | ✅ Supported | Enables/Disables Left/Right Margin capability. Essential for `DECSLRM`. |
| **95** | **DECNCSM** (No Clear) | ✅ Supported | Prevents screen clear when switching columns (Mode 3). |
| **1000+** | **Mouse Modes** | ✅ Supported | VT200, Button, Any-Event, Focus, SGR, URXVT, Pixel. |
| **1047** | **Alt Screen** | ✅ Supported | xterm standard. |
| **1048** | **Save/Restore** | ✅ Supported | Save/Restore Cursor. |
| **1049** | **Alt + Save** | ✅ Supported | Best practice buffer switching. |
| **2004** | **Bracketed Paste**| ✅ Supported | |
| **8246**| **BDSM** (BiDi) | ✅ Supported | Private mode for Bi-Directional Support. |

## 2. Control Sequences (CSI)

### Rectangular Operations (VT420)
| Sequence | Name | Status | Notes |
| :--- | :--- | :--- | :--- |
| `DECCRA` | Copy Rectangular Area | ✅ Supported | `CSI ... $ v` |
| `DECFRA` | Fill Rectangular Area | ✅ Supported | `CSI ... $ x` |
| `DECERA` | Erase Rectangular Area | ✅ Supported | `CSI ... $ z` |
| `DECSERA`| Selective Erase Rect | ✅ Supported | `CSI ... {` |
| `DECRQCRA`| Checksum Rect Area | ✅ Supported | `CSI ... * y`. Corrected syntax in v2.2.13. |

### Scrolling & Margins
| Sequence | Name | Status | Notes |
| :--- | :--- | :--- | :--- |
| `DECSTBM` | Top/Bottom Margins | ✅ Supported | `CSI r` |
| `DECSLRM` | Left/Right Margins | ✅ Supported | `CSI s` (with params). Parsed as DECSLRM only when `DECLRMM` (Mode 69) is enabled, otherwise treated as SCOSC (Save Cursor). |

### Cursor & Text
| Sequence | Name | Status | Notes |
| :--- | :--- | :--- | :--- |
| `DECSCUSR`| Set Cursor Style | ✅ Supported | `CSI ... q` (Space) |
| `DECSTR` | Soft Terminal Reset | ✅ Supported | `CSI ! p` |
| `DECUDK` | User Defined Keys | ✅ Supported | Implemented via DCS processing. |
| `DECDLD` | DownLoadable Fonts | ✅ Supported | Soft fonts via DCS. |

## 3. Device Status Reports (DSR)
Managed via `CSI ... n`.

- **5/6**: Status / Cursor Position (Standard) - ✅ Supported.
- **15**: Printer Status - ✅ Supported.
- **21**: Report Session Status (`DECRS`) - ✅ Supported (Multi-session awareness).
- **25**: UDK Status - ✅ Supported.
- **26**: Keyboard Status - ✅ Supported.
- **53**: Locator Status - ✅ Supported.
- **62**: Macro Space Report - ✅ Supported.
- **63**: Checksum Report (`DECCKSR`) - ✅ Supported.

## 4. Graphics Protocols
- **Sixel**: ✅ Supported (Parsing, Color Palettes, Transparency).
- **ReGIS**: ✅ Supported (Vectors, Curves, Polygons, Text).
- **Tektronix**: ✅ Supported (4010/4014 emulation).
- **Kitty Graphics**: ✅ Supported (Full protocol implementation).

## 5. Identified Gaps & Issues
1.  **Mode 2 (DECANM)**: Explicit switching via Mode 2 is not fully implemented in `ExecuteSM`.
2.  **Mode 67 (DECBKM)**: Backspace/Delete mapping configuration is missing.
3.  **BiDi Reordering**: While `BDSM` (Mode 8246) is supported, the reordering logic is internal and simplified (`BiDiReorderRow`), lacking full `fribidi` parity.

## Conclusion
KTerm v2.2.13 has achieved **Full VT420 Compliance** regarding page layout (margins) and rectangular operations syntax. The resolution of `DECLRMM`/`DECSLRM` syntax conflicts and `DECCOLM` resizing ensures compatibility with legacy VT420/VT520 applications.

The implementation of "Advanced DEC Command Sequences" is now **Excellent**.
