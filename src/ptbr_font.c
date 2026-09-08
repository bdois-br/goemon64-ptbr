#include "modding.h"

typedef signed int s32;
typedef signed short s16;
typedef unsigned int u32;
typedef unsigned char u8;
typedef float f32;

extern void* D_80167C48_168848[];
extern u8 D_800629A0_635A0[];
extern u8 D_8005BB10_5C710[];
extern void ptbr_request_wide_dialogue(s32 group, void* ctx);

extern void func_8000D180_DD80(
    s32 group,
    s32 slot,
    const u8* glyph_pair,
    s32 y_a3,
    void* arg4,
    void* arg5,
    s32 pair_plane,
    s32 x,
    s32 y,
    f32 scale
);

#define LOAD_S16(base, off) (*(s16*)((u8*)(base) + (off)))
#define LOAD_U32(base, off) (*(u32*)((u8*)(base) + (off)))
#define LOAD_PTR(base, off) (*(void**)((u8*)(base) + (off)))
#define STORE_S16(base, off, value) (*(s16*)((u8*)(base) + (off)) = (s16)(value))

#define PTBR_WRAP_RIGHT_288 270
#define PTBR_SPACE_CODE 0x00
#define PTBR_MAX_WORD_SLOTS 48
#define PTBR_PORTRAIT_LAYOUT_INSET 48
#define PTBR_LAYOUT_SIG_MAX 36

/*
 * Beta 0.8.52 - Beta 0.8.51 base + FILE 05F portrait clearance + energy-bar text fix.
 *
 * The game renderer normally receives one glyph at a time.  To avoid editing
 * the translated text bytes, we keep track of the glyph slots that belong to
 * the word currently being drawn.  If that word crosses the usable right edge
 * of a 288 px dialogue, the already-created glyph sprites for that word are
 * moved together to the next line, then subsequent glyphs continue there.
 * Words are never split in the middle.
 */
static void* s_wrap_ctx[3];
static s16 s_wrap_word_active[3];
static s16 s_wrap_word_line[3];
static s16 s_wrap_word_start_cursor[3];
static s16 s_wrap_word_chars[3];
static s16 s_wrap_word_slot_count[3];
static s16 s_wrap_word_slots[3][PTBR_MAX_WORD_SLOTS];

/* Beta 0.8.25 reflow state.
 * Automatic wraps create a "line debt": the next native scripted newline is
 * redundant and can be collapsed back into the same visual line. We do this
 * inside the already-proven glyph patch, avoiding any new game-function patch
 * or external runtime reference. */
static void* s_flow_ctx[3];
static s16 s_flow_soft_newline_debt[3];
static s16 s_flow_last_line[3];
static s16 s_flow_last_cursor[3];
static s16 s_flow_last_chars[3];
static s16 s_flow_last_y[3];
static s16 s_flow_last_was_space[3];

/*
 * Beta 0.8.36 - script-driven portrait layout.
 *
 * The old code treated ctx+0x30 == 4 as "portrait present". Testing showed
 * that value also occurs on ordinary dialogue windows, producing false 48 px
 * insets. Later sprite/texture probes were deliberately discarded because
 * they can miss a real portrait or add runtime risk.
 *
 * Instead, portrait clearance is attached to exact translated first-line
 * signatures from the validated FILE 074 portrait scene. These signatures
 * correspond to messages whose original English script contains intentional
 * leading layout spaces on continuation lines beside the portrait.
 *
 * No sprite scan, texture scan, pointer chase or runtime portrait guess is
 * performed. Unlisted messages receive zero portrait inset.
 */
typedef struct PtbrLayoutSignature {
    u8 len;
    u8 codes[PTBR_LAYOUT_SIG_MAX];
} PtbrLayoutSignature;

static const PtbrLayoutSignature s_portrait_layout_sigs[] = {
    {11, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x22,0x45,0x4D,0x0C}},
    {17, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x2F,0x4D,0x49,0x54,0x53,0x55,0x0C}},
    {28, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x2F,0x00,0x51,0x55,0x45,0x00,0x45,0x53,0x54,0x03,0x00,0x46,0x41,0x5A,0x45,0x4E,0x44,0x4F}},
    {13, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x22,0x45,0x4D,0x0E,0x0E,0x0E}},
    {13, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2F,0x00,0x51,0x55,0x0B,0x1F}},
    {30, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x24,0x45,0x53,0x44,0x45,0x00,0x51,0x55,0x45,0x00,0x4E,0x4F,0x53,0x53,0x4F,0x00,0x4E,0x45,0x47,0x1C,0x43,0x49,0x4F}},
    {26, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x4F,0x00,0x54,0x45,0x4C,0x45,0x46,0x4F,0x4E,0x45,0x00,0x4E,0x06,0x4F,0x00,0x50,0x41,0x52,0x41}},
    {28, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x24,0x45,0x53,0x43,0x55,0x4C,0x50,0x45,0x00,0x49,0x4E,0x54,0x45,0x52,0x52,0x4F,0x4D,0x50,0x45,0x52,0x0C}},
    {22, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x30,0x41,0x53,0x53,0x41,0x4D,0x4F,0x53,0x00,0x50,0x4F,0x52}},
    {23, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x21,0x00,0x2F,0x4D,0x49,0x54,0x53,0x55,0x00,0x56,0x45,0x49,0x4F}},
    {36, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x34,0x41,0x4C,0x56,0x45,0x5A,0x00,0x45,0x4C,0x41,0x00,0x53,0x45,0x4A,0x41,0x00,0x42,0x45,0x4D,0x00,0x4D,0x41,0x49,0x53,0x00,0x44,0x55,0x52,0x41}},
    {31, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x24,0x4F,0x00,0x51,0x55,0x45,0x00,0x45,0x53,0x54,0x41,0x4D,0x4F,0x53,0x00,0x46,0x41,0x4C,0x41,0x4E,0x44,0x4F,0x1F,0x01}},
    {14, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2F,0x4D,0x49,0x54,0x53,0x55,0x01}},
    {33, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2F,0x4D,0x49,0x54,0x53,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55}},
    {21, {0x39,0x41,0x45,0x1A,0x2B,0x59,0x55,0x53,0x48,0x55,0x00,0x53,0x45,0x00,0x53,0x45,0x50,0x41,0x52,0x4F,0x55}},
    {15, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x5B,0x00,0x46,0x03,0x43,0x49,0x4C,0x01}},
    {17, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2F,0x55,0x00,0x4D,0x45,0x4C,0x48,0x4F,0x52,0x0C}},
    {33, {0x39,0x41,0x45,0x1A,0x31,0x55,0x41,0x4E,0x44,0x4F,0x00,0x45,0x53,0x54,0x41,0x4D,0x4F,0x53,0x00,0x53,0x45,0x4D,0x00,0x52,0x45,0x53,0x50,0x4F,0x53,0x54,0x41,0x53,0x0C}}
};

#define PTBR_PORTRAIT_LAYOUT_SIG_COUNT ((s32)(sizeof(s_portrait_layout_sigs) / sizeof(s_portrait_layout_sigs[0])))

/*
 * Beta 0.8.47 - targeted FILE 069 portrait layout.
 *
 * The Sogen/restaurant cutscene shown in the latest test screenshots has a
 * different portrait geometry from FILE 074: lines 0 and 1 are already above
 * the portrait, while line 2 and later can pass behind it.  We therefore use
 * exact first-line signatures only for the confirmed FILE 069 messages and
 * start the 48 px inset at native line index 2.
 *
 * This is deliberately not a dynamic portrait detector.  No sprite scan,
 * texture scan, pointer chase, text patch or BPS change is involved.
 */
static const PtbrLayoutSignature s_file069_portrait_layout_sigs[] = {
    {25, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x25,0x55,0x00,0x4E,0x06,0x4F,0x00,0x53,0x41,0x42,0x49,0x41,0x00,0x4F,0x00,0x51,0x55,0x45}},
    {27, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x2E,0x41,0x00,0x48,0x4F,0x52,0x41,0x00,0x4D,0x45,0x00,0x41,0x53,0x53,0x55,0x53,0x54,0x45,0x49,0x0C}},
    {31, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2F,0x00,0x50,0x4F,0x44,0x45,0x52,0x00,0x44,0x4F,0x00,0x32,0x41,0x49,0x4F,0x00,0x44,0x45,0x00,0x30,0x41,0x4C,0x43,0x4F}},
    {22, {0x33,0x41,0x53,0x55,0x4B,0x45,0x1A,0x29,0x4D,0x41,0x47,0x49,0x4E,0x45,0x00,0x53,0x45,0x00,0x45,0x4C,0x45,0x53}},
    {20, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x5B,0x00,0x56,0x45,0x52,0x44,0x41,0x44,0x45,0x01}},
    {27, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x25,0x4E,0x54,0x06,0x4F,0x00,0x56,0x4F,0x55,0x00,0x53,0x4F,0x50,0x52,0x41,0x52,0x0C}},
    {13, {0x33,0x41,0x53,0x55,0x4B,0x45,0x1A,0x5B,0x00,0x45,0x4C,0x45,0x01}},
    {34, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x36,0x4F,0x43,0x0B,0x00,0x4E,0x06,0x4F,0x00,0x50,0x4F,0x44,0x45,0x52,0x49,0x41,0x00,0x45,0x53,0x54,0x41,0x52}},
    {16, {0x39,0x41,0x45,0x1A,0x33,0x49,0x4D,0x0C,0x00,0x48,0x03,0x00,0x41,0x4C,0x47,0x4F}},
    {30, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x22,0x45,0x4D,0x0E,0x0E,0x0E,0x00,0x45,0x4D,0x00,0x54,0x52,0x4F,0x43,0x41,0x00,0x44,0x45}},
    {27, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x2D,0x41,0x53,0x00,0x49,0x53,0x53,0x4F,0x00,0x0A,0x00,0x45,0x53,0x54,0x52,0x41,0x4E,0x48,0x4F,0x0E}},
    {29, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x2F,0x00,0x51,0x55,0x45,0x00,0x56,0x4F,0x43,0x0B,0x00,0x41,0x43,0x41,0x42,0x4F,0x55}},
    {18, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x27,0x52,0x52,0x52,0x52,0x01}},
    {25, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x21,0x43,0x52,0x45,0x44,0x49,0x54,0x41,0x4D,0x4F,0x53,0x00,0x51,0x55,0x45}},
    {24, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x2E,0x06,0x4F,0x00,0x50,0x41,0x52,0x45,0x43,0x45,0x00,0x51,0x55,0x45}},
    /* Added after wider FILE 069 test coverage supplied by the user. */
    {22, {0x2F,0x4D,0x49,0x54,0x53,0x55,0x1A,0x34,0x45,0x4E,0x48,0x4F,0x00,0x43,0x45,0x52,0x54,0x45,0x5A,0x41,0x00,0x44}},
    {16, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x36,0x06,0x4F,0x00}},
    {25, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x25,0x0E,0x0E,0x0E,0x00,0x44,0x45,0x49,0x00,0x41,0x00,0x45,0x4C}},
    {20, {0x36,0x45,0x4C,0x48,0x4F,0x00,0x33,0x03,0x42,0x49,0x4F,0x1A,0x21,0x48,0x0C,0x00,0x41,0x48,0x0C,0x00}},
    {24, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x22,0x45,0x4D,0x0C,0x00,0x41,0x47,0x4F,0x52,0x41,0x00,0x54,0x55,0x44}}
};

#define PTBR_FILE069_PORTRAIT_SIG_COUNT ((s32)(sizeof(s_file069_portrait_layout_sigs) / sizeof(s_file069_portrait_layout_sigs[0])))

/*
 * Beta 0.8.51 - targeted FILE 067 portrait layout.
 *
 * In the Grand Finale / Fernandez scene the portrait occupies the text area
 * one line earlier than in FILE 069.  The first line is clear, while line 1
 * (the second visual line) and later can pass behind the portrait.  Only the
 * exact first-line signatures confirmed by the user's screenshots are listed
 * here, and they receive the same 48 px inset starting at line_index >= 1.
 *
 * No dynamic portrait detection, sprite scan, texture scan, BPS change or
 * translated-text rewrite is involved.
 */
static const PtbrLayoutSignature s_file067_portrait_layout_sigs[] = {
    {19, {0x2F,0x00,0x27,0x52,0x41,0x4E,0x44,0x45,0x00,0x26,0x49,0x4E,0x41,0x4C,0x00,0x45,0x53,0x54,0x03}},
    {24, {0x49,0x53,0x53,0x4F,0x00,0x4E,0x06,0x4F,0x00,0x50,0x4F,0x44,0x45,0x00,0x53,0x45,0x52,0x0E,0x0E,0x0E,0x00,0x25,0x55,0x0C}},
    {29, {0x2E,0x06,0x4F,0x00,0x50,0x45,0x4E,0x53,0x45,0x00,0x4E,0x45,0x4D,0x00,0x50,0x4F,0x52,0x00,0x55,0x4D,0x00,0x49,0x4E,0x53,0x54,0x41,0x4E,0x54,0x45}},
    {27, {0x54,0x45,0x4E,0x48,0x4F,0x00,0x43,0x45,0x52,0x54,0x45,0x5A,0x41,0x00,0x44,0x45,0x00,0x51,0x55,0x45,0x00,0x55,0x4D,0x00,0x44,0x49,0x41}},
    {16, {0x25,0x00,0x4D,0x45,0x55,0x00,0x4E,0x4F,0x4D,0x45,0x00,0x4E,0x06,0x4F,0x00,0x0A}}
};

#define PTBR_FILE067_PORTRAIT_SIG_COUNT ((s32)(sizeof(s_file067_portrait_layout_sigs) / sizeof(s_file067_portrait_layout_sigs[0])))

/*
 * Beta 0.8.52 - targeted FILE 05F portrait layout.
 *
 * The Peach Mountain stage/finale dialogue uses the same vertical portrait
 * clearance pattern as FILE 069: native lines 0 and 1 remain above the
 * portrait, while line index 2 and later can overlap it.  Keep the same
 * conservative signature-driven approach and add only the messages confirmed
 * by the user's screenshots.
 */
static const PtbrLayoutSignature s_file05f_portrait_layout_sigs[] = {
    {20, {0x24,0x41,0x4E,0x43,0x49,0x4E,0x07,0x1A,0x23,0x4F,0x4D,0x50,0x4F,0x52,0x54,0x45,0x0D,0x53,0x45,0x01}},
    {23, {0x39,0x41,0x45,0x1A,0x2E,0x06,0x4F,0x00,0x53,0x45,0x49,0x00,0x51,0x55,0x45,0x00,0x50,0x4F,0x44,0x45,0x52,0x45,0x53}},
    {27, {0x33,0x41,0x53,0x55,0x4B,0x45,0x1A,0x33,0x49,0x4D,0x0C,0x00,0x56,0x4F,0x43,0x0B,0x00,0x54,0x45,0x4D,0x00,0x52,0x41,0x5A,0x06,0x4F,0x01}},
    {26, {0x24,0x41,0x4E,0x43,0x49,0x4E,0x07,0x1A,0x33,0x1C,0x00,0x50,0x52,0x45,0x43,0x49,0x53,0x41,0x4D,0x4F,0x53,0x00,0x55,0x4E,0x49,0x52}},
    {11, {0x24,0x41,0x4E,0x43,0x49,0x4E,0x07,0x1A,0x24,0x4F,0x53}},
    {23, {0x25,0x42,0x49,0x53,0x55,0x4D,0x41,0x52,0x55,0x1A,0x25,0x55,0x00,0x4E,0x06,0x4F,0x00,0x53,0x45,0x49,0x0E,0x0E,0x0E}},
    {16, {0x27,0x4F,0x45,0x4D,0x4F,0x4E,0x1A,0x23,0x41,0x4E,0x41,0x4C,0x48,0x41,0x53,0x01}}
};

#define PTBR_FILE05F_PORTRAIT_SIG_COUNT ((s32)(sizeof(s_file05f_portrait_layout_sigs) / sizeof(s_file05f_portrait_layout_sigs[0])))

static void* s_layout_ctx[3];
static s16 s_layout_last_line[3];
static s16 s_layout_last_cursor[3];
static s16 s_layout_prefix_count[3];
static s16 s_layout_active[3];
static s16 s_layout_start_line[3];
static u8 s_layout_prefix[3][PTBR_LAYOUT_SIG_MAX];

/*
 * Beta 0.8.39 - visual-only hyphenated break for
 * "Castelo dos Brinquedos Fantasmas".
 *
 * Desired visual layout:
 *     [Castelo dos Brin-
 *     quedos Fantasmas]
 *
 * The native text bytes, cursor, line index, Y state and 9-row / 72 px box
 * geometry are NOT modified. When the exact sequence "Castelo dos Brin" is
 * followed by the 'q' of "quedos", that 'q' and every subsequent glyph are
 * drawn one visual line lower. A native hyphen glyph is drawn at the end of
 * "Brin" without advancing the game's text cursor.
 */
static void* s_manual_visual_ctx[3];
static s16 s_manual_visual_match[3];
static s16 s_manual_visual_active[3];
static s16 s_manual_visual_break_cursor[3];
static s16 s_manual_visual_draw_hyphen[3];

static const u8 s_manual_visual_prefix[] = {
    0x23, /* C */
    0x41,0x53,0x54,0x45,0x4C,0x4F, /* astelo */
    0x00, /* space */
    0x44,0x4F,0x53, /* dos */
    0x00, /* space */
    0x22, /* B */
    0x52,0x49,0x4E /* rin */
};

#define PTBR_MANUAL_VISUAL_PREFIX_LEN ((s32)sizeof(s_manual_visual_prefix))
#define PTBR_MANUAL_VISUAL_BREAK_CODE 0x51 /* q */
#define PTBR_MANUAL_VISUAL_HYPHEN_CODE 0x0D /* '-' */

static void ptbr_manual_visual_reset(s32 group, u8* ctx) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_manual_visual_ctx[group] = (void*)ctx;
    s_manual_visual_match[group] = 0;
    s_manual_visual_active[group] = 0;
    s_manual_visual_break_cursor[group] = 0;
    s_manual_visual_draw_hyphen[group] = 0;
}

static void ptbr_manual_visual_prepare(s32 group, u8* ctx) {
    if ((unsigned int)group >= 3u || ctx == (u8*)0) {
        return;
    }

    if (s_manual_visual_ctx[group] != (void*)ctx) {
        ptbr_manual_visual_reset(group, ctx);
        return;
    }

    /* Context allocations are reused between messages. At the start of a new
     * message/page, native text state returns to line 0 / cursor 0. */
    if (LOAD_S16(ctx, 0x72) == 0 && LOAD_S16(ctx, 0x6E) == 0) {
        s_manual_visual_match[group] = 0;
        s_manual_visual_active[group] = 0;
        s_manual_visual_break_cursor[group] = 0;
        s_manual_visual_draw_hyphen[group] = 0;
    }
}

static void ptbr_manual_visual_feed(s32 group, u8* ctx, s32 code) {
    s32 match;

    if ((unsigned int)group >= 3u || ctx == (u8*)0) {
        return;
    }

    ptbr_manual_visual_prepare(group, ctx);

    if (s_manual_visual_active[group]) {
        return;
    }

    match = s_manual_visual_match[group];

    if (match == PTBR_MANUAL_VISUAL_PREFIX_LEN) {
        if (code == PTBR_MANUAL_VISUAL_BREAK_CODE) {
            s_manual_visual_active[group] = 1;
            s_manual_visual_break_cursor[group] = LOAD_S16(ctx, 0x6E);
            s_manual_visual_draw_hyphen[group] = 1;
            s_manual_visual_match[group] = 0;
            return;
        }
        match = 0;
        s_manual_visual_match[group] = 0;
    }

    if (code == (s32)s_manual_visual_prefix[match]) {
        s_manual_visual_match[group] = (s16)(match + 1);
    } else {
        s_manual_visual_match[group] =
            (s16)(code == (s32)s_manual_visual_prefix[0] ? 1 : 0);
    }
}

static s32 ptbr_manual_visual_is_active(s32 group) {
    return ((unsigned int)group < 3u) && s_manual_visual_active[group];
}

static void ptbr_manual_visual_adjust_xy(s32 group, u8* ctx, s32* x, s32* y) {
    s32 line_height;

    if (!ptbr_manual_visual_is_active(group) || ctx == (u8*)0) {
        return;
    }

    line_height = LOAD_S16(ctx, 0x76);
    if (line_height <= 0) {
        line_height = 16;
    }

    *x -= s_manual_visual_break_cursor[group];
    *y += line_height;
}

static void ptbr_layout_reset(s32 group, u8* ctx) {
    s_layout_ctx[group] = (void*)ctx;
    s_layout_last_line[group] = LOAD_S16(ctx, 0x72);
    s_layout_last_cursor[group] = LOAD_S16(ctx, 0x6E);
    s_layout_prefix_count[group] = 0;
    s_layout_active[group] = 0;
    s_layout_start_line[group] = 1;
}

static void ptbr_layout_prepare(s32 group, u8* ctx) {
    s32 line;
    s32 cursor;

    if ((unsigned int)group >= 3u) {
        return;
    }

    line = LOAD_S16(ctx, 0x72);
    cursor = LOAD_S16(ctx, 0x6E);

    if (s_layout_ctx[group] != (void*)ctx) {
        ptbr_layout_reset(group, ctx);
        return;
    }

    /* Same context allocation can be reused for the next message/page.
     * Reset when the native text state returns to the start of line zero. */
    if (line < s_layout_last_line[group] ||
        (line == 0 && cursor == 0 &&
         (s_layout_last_line[group] != 0 || s_layout_last_cursor[group] != 0))) {
        ptbr_layout_reset(group, ctx);
    }
}

static void ptbr_layout_feed_code(s32 group, u8* ctx, s32 code) {
    s32 count;
    s32 sig_i;
    s32 i;

    ptbr_layout_prepare(group, ctx);
    if ((unsigned int)group >= 3u || LOAD_S16(ctx, 0x72) != 0) {
        return;
    }

    count = s_layout_prefix_count[group];
    if (count >= PTBR_LAYOUT_SIG_MAX) {
        return;
    }

    s_layout_prefix[group][count] = (u8)code;
    count++;
    s_layout_prefix_count[group] = (s16)count;

    for (sig_i = 0; sig_i < PTBR_PORTRAIT_LAYOUT_SIG_COUNT; sig_i++) {
        const PtbrLayoutSignature* sig = &s_portrait_layout_sigs[sig_i];
        if (count != (s32)sig->len) {
            continue;
        }
        for (i = 0; i < count; i++) {
            if (s_layout_prefix[group][i] != sig->codes[i]) {
                break;
            }
        }
        if (i == count) {
            s_layout_active[group] = 1;
            s_layout_start_line[group] = 1;
            return;
        }
    }

    for (sig_i = 0; sig_i < PTBR_FILE069_PORTRAIT_SIG_COUNT; sig_i++) {
        const PtbrLayoutSignature* sig = &s_file069_portrait_layout_sigs[sig_i];
        if (count != (s32)sig->len) {
            continue;
        }
        for (i = 0; i < count; i++) {
            if (s_layout_prefix[group][i] != sig->codes[i]) {
                break;
            }
        }
        if (i == count) {
            s_layout_active[group] = 1;
            s_layout_start_line[group] = 2;
            return;
        }
    }


    for (sig_i = 0; sig_i < PTBR_FILE067_PORTRAIT_SIG_COUNT; sig_i++) {
        const PtbrLayoutSignature* sig = &s_file067_portrait_layout_sigs[sig_i];
        if (count != (s32)sig->len) {
            continue;
        }
        for (i = 0; i < count; i++) {
            if (s_layout_prefix[group][i] != sig->codes[i]) {
                break;
            }
        }
        if (i == count) {
            s_layout_active[group] = 1;
            s_layout_start_line[group] = 1;
            return;
        }
    }

    for (sig_i = 0; sig_i < PTBR_FILE05F_PORTRAIT_SIG_COUNT; sig_i++) {
        const PtbrLayoutSignature* sig = &s_file05f_portrait_layout_sigs[sig_i];
        if (count != (s32)sig->len) {
            continue;
        }
        for (i = 0; i < count; i++) {
            if (s_layout_prefix[group][i] != sig->codes[i]) {
                break;
            }
        }
        if (i == count) {
            s_layout_active[group] = 1;
            s_layout_start_line[group] = 2;
            return;
        }
    }
}

static void ptbr_layout_snapshot(s32 group, u8* ctx) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_layout_ctx[group] = (void*)ctx;
    s_layout_last_line[group] = LOAD_S16(ctx, 0x72);
    s_layout_last_cursor[group] = LOAD_S16(ctx, 0x6E);
}

static s32 ptbr_script_layout_inset(s32 group, u8* ctx, s32 line_index) {
    (void)ctx;
    if ((unsigned int)group < 3u && s_layout_active[group] &&
        line_index >= s_layout_start_line[group]) {
        return PTBR_PORTRAIT_LAYOUT_INSET;
    }
    return 0;
}

typedef enum PtbrAccentKind {
    PTBR_NONE = 0,
    PTBR_ACUTE,
    PTBR_GRAVE,
    PTBR_CIRC,
    PTBR_TILDE,
    PTBR_CEDILLA
} PtbrAccentKind;

static s32 ptbr_base_code(s32 code) {
    switch (code) {
        case 0x03: return 0x41; /* á -> a */
        case 0x04: return 0x41; /* à -> a */
        case 0x05: return 0x41; /* â -> a */
        case 0x06: return 0x41; /* ã -> a */
        case 0x0A: return 0x45; /* é -> e */
        case 0x0B: return 0x45; /* ê -> e */
        case 0x1B: return 0x49; /* í -> i */
        case 0x1C: return 0x4F; /* ó -> o */
        case 0x1D: return 0x4F; /* ô -> o */
        case 0x1E: return 0x4F; /* õ -> o */
        case 0x20: return 0x55; /* ú -> u */
        case 0x3C: return 0x43; /* ç -> c */
        case 0x3E: return 0x21; /* Á -> A */
        case 0x3F: return 0x21; /* Ã -> A */
        case 0x40: return 0x23; /* Ç -> C */
        case 0x5B: return 0x25; /* É -> E */
        case 0x5C: return 0x25; /* Ê -> E */
        case 0x5D: return 0x2F; /* Ó -> O */
        case 0x5E: return 0x35; /* Ú -> U */
        default: return code;
    }
}

static PtbrAccentKind ptbr_accent_kind(s32 code) {
    switch (code) {
        case 0x03: case 0x0A: case 0x1B: case 0x1C: case 0x20:
        case 0x3E: case 0x5B: case 0x5D: case 0x5E:
            return PTBR_ACUTE;
        case 0x04:
            return PTBR_GRAVE;
        case 0x05: case 0x0B: case 0x1D: case 0x5C:
            return PTBR_CIRC;
        case 0x06: case 0x1E: case 0x3F:
            return PTBR_TILDE;
        case 0x3C: case 0x40:
            return PTBR_CEDILLA;
        default:
            return PTBR_NONE;
    }
}

static s32 ptbr_is_uppercase_accent(s32 code) {
    switch (code) {
        case 0x3E: case 0x3F: case 0x40:
        case 0x5B: case 0x5C: case 0x5D: case 0x5E:
            return 1;
        default:
            return 0;
    }
}

static const u8* ptbr_original_pair(s32 code) {
    return D_800629A0_635A0 + ((code / 2) * 48);
}

static void ptbr_draw_original_glyph_scaled(
    u8* ctx,
    s32 group,
    s32 slot,
    s32 code,
    s32 x,
    s32 y,
    f32 scale
) {
    func_8000D180_DD80(
        group,
        slot,
        ptbr_original_pair(code),
        y,
        LOAD_PTR(ctx, 0x18),
        LOAD_PTR(ctx, 0x1C),
        (s32)LOAD_U32(ctx, 0x24) + (code & 1),
        x,
        y,
        scale
    );
}

static void ptbr_draw_original_glyph(
    u8* ctx,
    s32 group,
    s32 slot,
    s32 code,
    s32 x,
    s32 y
) {
    ptbr_draw_original_glyph_scaled(ctx, group, slot, code, x, y, 1.0f);
}

static void ptbr_reset_wrap_word(s32 group, void* ctx);

static void ptbr_flow_init(s32 group, u8* ctx) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_flow_ctx[group] = (void*)ctx;
    s_flow_soft_newline_debt[group] = 0;
    s_flow_last_line[group] = LOAD_S16(ctx, 0x72);
    s_flow_last_cursor[group] = LOAD_S16(ctx, 0x6E);
    s_flow_last_chars[group] = LOAD_S16(ctx, 0x6C);
    s_flow_last_y[group] = LOAD_S16(ctx, 0x74);
    s_flow_last_was_space[group] = 0;
}

/* Detect the native newline by observing the context at the next glyph call.
 * If an automatic wrap has already consumed that visual line, undo only that
 * redundant newline and turn it into a normal inter-word separator. */
static void ptbr_flow_collapse_redundant_newline(s32 group, u8* ctx) {
    s32 line;
    s32 sep_width;

    if ((unsigned int)group >= 3u) {
        return;
    }

    if (s_flow_ctx[group] != (void*)ctx) {
        ptbr_flow_init(group, ctx);
        return;
    }

    line = LOAD_S16(ctx, 0x72);

    /* Same allocation reused for a new page/message: never carry debt across
     * a backwards native line reset. */
    if (line < s_flow_last_line[group]) {
        ptbr_flow_init(group, ctx);
        return;
    }

    if (s_flow_soft_newline_debt[group] > 0 &&
        line == (s32)s_flow_last_line[group] + 1) {
        sep_width = s_flow_last_was_space[group] ? 0 :
            ((s32)D_8005BB10_5C710[PTBR_SPACE_CODE] + 1);

        STORE_S16(ctx, 0x72, s_flow_last_line[group]);
        STORE_S16(ctx, 0x74, s_flow_last_y[group]);
        STORE_S16(ctx, 0x6E, (s32)s_flow_last_cursor[group] + sep_width);
        STORE_S16(ctx, 0x6C, (s32)s_flow_last_chars[group] + (sep_width != 0));
        s_flow_soft_newline_debt[group]--;

        /* A softened newline is still a word boundary. */
        ptbr_reset_wrap_word(group, (void*)ctx);
    }
}

static void ptbr_flow_snapshot(s32 group, u8* ctx, s32 code) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_flow_ctx[group] = (void*)ctx;
    s_flow_last_line[group] = LOAD_S16(ctx, 0x72);
    s_flow_last_cursor[group] = LOAD_S16(ctx, 0x6E);
    s_flow_last_chars[group] = LOAD_S16(ctx, 0x6C);
    s_flow_last_y[group] = LOAD_S16(ctx, 0x74);
    s_flow_last_was_space[group] = (s16)(code == PTBR_SPACE_CODE);
}

static void ptbr_reset_wrap_word(s32 group, void* ctx) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_wrap_ctx[group] = ctx;
    s_wrap_word_active[group] = 0;
    s_wrap_word_line[group] = 0;
    s_wrap_word_start_cursor[group] = 0;
    s_wrap_word_chars[group] = 0;
    s_wrap_word_slot_count[group] = 0;
}

static void ptbr_begin_wrap_word(s32 group, u8* ctx) {
    if ((unsigned int)group >= 3u) {
        return;
    }
    s_wrap_ctx[group] = (void*)ctx;
    s_wrap_word_active[group] = 1;
    s_wrap_word_line[group] = LOAD_S16(ctx, 0x72);
    s_wrap_word_start_cursor[group] = LOAD_S16(ctx, 0x6E);
    s_wrap_word_chars[group] = 0;
    s_wrap_word_slot_count[group] = 0;
}

static void ptbr_record_wrap_slot(s32 group, s32 slot) {
    s16 count;
    if ((unsigned int)group >= 3u || slot < 0 || !s_wrap_word_active[group]) {
        return;
    }
    count = s_wrap_word_slot_count[group];
    if (count < PTBR_MAX_WORD_SLOTS) {
        s_wrap_word_slots[group][count] = (s16)slot;
        s_wrap_word_slot_count[group] = count + 1;
    }
}

static s32 ptbr_line_inset(s32 group, u8* ctx, s32 line_index) {
    return ptbr_script_layout_inset(group, ctx, line_index);
}

static s32 ptbr_should_wrap_word(s32 group, u8* ctx) {
    s32 cursor;
    s32 line;
    s32 inset;

    if ((unsigned int)group >= 3u || !s_wrap_word_active[group]) {
        return 0;
    }

    /* Beta 0.8.38: the manual second line is visual-only. Do not let the
     * automatic reflow mutate native line/cursor state while it is active. */
    if (ptbr_manual_visual_is_active(group)) {
        return 0;
    }

    /* Never repeatedly wrap a single word that is itself wider than a line. */
    if (s_wrap_word_start_cursor[group] <= 0) {
        return 0;
    }

    /* Only standard dialogue-window geometries participate. */
    if (LOAD_S16(ctx, 0x40) != 8 ||
        (LOAD_S16(ctx, 0x42) != 7 && LOAD_S16(ctx, 0x42) != 9)) {
        return 0;
    }

    line = LOAD_S16(ctx, 0x72);
    /* The validated 72 px dialogue geometry safely accommodates four text
     * lines (indices 0..3). Do not invent a fifth visual line. */
    if (line >= 3) {
        return 0;
    }
    inset = ptbr_line_inset(group, ctx, line);
    cursor = LOAD_S16(ctx, 0x6E);
    return (cursor + inset) > PTBR_WRAP_RIGHT_288;
}

static void ptbr_wrap_current_word(s32 group, u8* ctx) {
    s32 old_line;
    s32 new_line;
    s32 old_inset;
    s32 new_inset;
    s32 start_cursor;
    s32 word_width;
    s32 line_height;
    s32 dx;
    s32 dy;
    s32 i;

    if ((unsigned int)group >= 3u || !s_wrap_word_active[group]) {
        return;
    }

    old_line = s_wrap_word_line[group];
    new_line = old_line + 1;
    start_cursor = s_wrap_word_start_cursor[group];
    word_width = (s32)LOAD_S16(ctx, 0x6E) - start_cursor;
    if (start_cursor <= 0 || word_width <= 0) {
        return;
    }

    line_height = LOAD_S16(ctx, 0x76);
    if (line_height <= 0) {
        line_height = 16;
    }

    old_inset = ptbr_line_inset(group, ctx, old_line);
    new_inset = ptbr_line_inset(group, ctx, new_line);
    dx = new_inset - old_inset - start_cursor;
    dy = line_height;

    /* func_8000D180 stores each glyph sprite at ctx + slot*0x30 + 0x198.
     * Its screen X/Y fields are +0x08/+0x0A inside that record.  Moving every
     * slot accumulated for this word keeps base letters and accent overlays
     * together while preserving their relative placement. */
    for (i = 0; i < s_wrap_word_slot_count[group]; i++) {
        s32 slot = s_wrap_word_slots[group][i];
        u8* rec = ctx + (slot * 0x30) + 0x198;
        if (LOAD_S16(rec, 0x04) != 0) {
            STORE_S16(rec, 0x08, LOAD_S16(rec, 0x08) + dx);
            STORE_S16(rec, 0x0A, LOAD_S16(rec, 0x0A) + dy);
        }
    }

    /* Same state changes as the game's native newline helper at C3EC, except
     * the cursor/character count already include the word we just moved. */
    STORE_S16(ctx, 0x6E, word_width);
    STORE_S16(ctx, 0x6C, s_wrap_word_chars[group]);
    STORE_S16(ctx, 0x74, LOAD_S16(ctx, 0x74) + line_height);
    STORE_S16(ctx, 0x72, new_line);

    /* Keep this word active so remaining letters continue on the new line,
     * but prevent a second wrap of the same word by setting start cursor to 0. */
    s_wrap_word_line[group] = (s16)new_line;
    s_wrap_word_start_cursor[group] = 0;
    s_flow_soft_newline_debt[group] = (s16)(s_flow_soft_newline_debt[group] + 1);

    ptbr_request_wide_dialogue(group, ctx);
}

static s32 ptbr_find_free_slot(u8* ctx) {
    for (s32 slot = 0; slot < 120; slot++) {
        if (LOAD_S16(ctx + (slot * 0x30), 0x19C) == 0) {
            return slot;
        }
    }
    return -1;
}

static void ptbr_draw_mark(u8* ctx, s32 group, s32 code, s32 x, s32 y) {
    PtbrAccentKind kind = ptbr_accent_kind(code);
    s32 upper = ptbr_is_uppercase_accent(code);
    s32 slot;

    if (kind == PTBR_NONE) {
        return;
    }

    if (kind == PTBR_ACUTE) {
        /* Beta 0.6: stronger acute accent for visible in-game change. */
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x0F, x + (upper ? 1 : 2), y + (upper ? -3 : -4), 0.75f);
        }
        return;
    }

    if (kind == PTBR_GRAVE) {
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x07, x + 1, y + (upper ? -4 : -3), 0.70f);
        }
        return;
    }

    if (kind == PTBR_CIRC) {
        /* Beta 0.8.49: use one native caret glyph, reduced and centered.
         * The previous 1.00x version was too large; the 0.8.48 two-stroke
         * experiment looked fragmented. 0.70x keeps the original symmetric
         * caret silhouette and places it close to the vowel without changing
         * text advance or layout state. */
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x3E, x + 1, y + (upper ? -3 : -2), 0.70f);
        }
        return;
    }

    if (kind == PTBR_TILDE) {
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x0D, x + 1, y + (upper ? -4 : -3), 0.45f);
        }
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x0D, x + 4, y + (upper ? -5 : -4), 0.45f);
        }
        return;
    }

    if (kind == PTBR_CEDILLA) {
        slot = ptbr_find_free_slot(ctx);
        if (slot >= 0) {
            ptbr_record_wrap_slot(group, slot);
            ptbr_draw_original_glyph_scaled(ctx, group, slot, 0x0C, x + 2, y + (upper ? 3 : 2), 0.75f);
        }
    }
}

/*
 * Beta 0.6: renderer overlay approach.
 *
 * The previous safe build passed custom glyph bytes linked at 0x81000000 to the
 * N64 display-list texture path. The RDP texture command expects a normal game
 * RDRAM address, so those mod-data pointers rendered as large corrupt blocks.
 *
 * This version never supplies mod-owned texture data to the renderer. Every
 * texture pointer comes from D_800629A0_635A0, the game's original font in
 * normal RDRAM. Accented letters are composed from the original base letter and
 * one or more original punctuation glyphs positioned as accent marks.
 */
RECOMP_PATCH s32 func_8000D060_DC60(s32 group, s32 code) {
    u8* ctx = (u8*)D_80167C48_168848[group];
    s32 slot;
    s32 x;
    s32 y;
    s32 base;
    s32 line_index;
    PtbrAccentKind kind;

    if (ctx == (u8*)0) {
        return -1;
    }

    ptbr_flow_collapse_redundant_newline(group, ctx);
    ptbr_manual_visual_feed(group, ctx, code);
    ptbr_layout_feed_code(group, ctx, code);
    line_index = LOAD_S16(ctx, 0x72);
    if (s_wrap_ctx[group] != (void*)ctx ||
        (s_wrap_word_active[group] && s_wrap_word_line[group] != line_index)) {
        ptbr_reset_wrap_word(group, (void*)ctx);
    }

    if (code != PTBR_SPACE_CODE && !s_wrap_word_active[group]) {
        ptbr_begin_wrap_word(group, ctx);
    }

    slot = ptbr_find_free_slot(ctx);
    if (slot < 0) {
        return -1;
    }

    x = (s32)LOAD_U32(ctx, 0x50) + (s32)LOAD_S16(ctx, 0x6E);
    y = (s32)LOAD_U32(ctx, 0x54) + (s32)LOAD_S16(ctx, 0x74);

    /* Beta 0.8.39: keep the native cursor untouched. If this glyph is the
     * first 'q' after "Castelo dos Brin", remember its original draw point
     * for a visual hyphen, then move the q and all following glyphs down. */
    s32 manual_hyphen_x = x;
    s32 manual_hyphen_y = y;
    s32 manual_hyphen_pending =
        ((unsigned int)group < 3u) && s_manual_visual_draw_hyphen[group];
    ptbr_manual_visual_adjust_xy(group, ctx, &x, &y);

    /* Restore only script-confirmed portrait clearance. The validated
     * English FILE 074 scene uses intentional leading layout spaces on
     * continuation lines beside character portraits. Beta 0.8.36 recreates
     * that 48 px clearance only for whitelisted translated messages. */
    if (ptbr_script_layout_inset(group, ctx, LOAD_S16(ctx, 0x72)) != 0) {
        x += PTBR_PORTRAIT_LAYOUT_INSET;
        ptbr_request_wide_dialogue(group, ctx);
    }

    base = ptbr_base_code(code);
    kind = ptbr_accent_kind(code);
    if (code != PTBR_SPACE_CODE) {
        ptbr_record_wrap_slot(group, slot);
    }
    ptbr_draw_original_glyph(ctx, group, slot, base, x, y);

    if (kind != PTBR_NONE) {
        ptbr_draw_mark(ctx, group, code, x, y);
    }

    /* Draw '-' at the original first-line position without advancing cursor.
     * The q itself is already rendered at the second-line visual position. */
    if (manual_hyphen_pending) {
        s32 hyphen_slot;
        s_manual_visual_draw_hyphen[group] = 0;
        hyphen_slot = ptbr_find_free_slot(ctx);
        if (hyphen_slot >= 0) {
            ptbr_draw_original_glyph(
                ctx, group, hyphen_slot, PTBR_MANUAL_VISUAL_HYPHEN_CODE,
                manual_hyphen_x, manual_hyphen_y);
        }
    }

    LOAD_S16(ctx, 0x6E) = (s16)(LOAD_S16(ctx, 0x6E) + (s32)D_8005BB10_5C710[base] + 1);
    LOAD_S16(ctx, 0x6C) = (s16)(LOAD_S16(ctx, 0x6C) + 1);

    if (code != PTBR_SPACE_CODE) {
        s_wrap_word_chars[group] = (s16)(s_wrap_word_chars[group] + 1);
        if (ptbr_should_wrap_word(group, ctx)) {
            ptbr_wrap_current_word(group, ctx);
        }
    } else {
        /* A space closes the current word. The next visible glyph starts a
         * fresh word and can be moved as one unit if it does not fit. */
        ptbr_reset_wrap_word(group, (void*)ctx);
    }

    /* Original text starts at x=10. Request the 288 px dialogue background
     * before a line reaches the native right edge.  Beta 0.8.25 may also
     * create a visual newline, but it never edits the script bytes. */
    if (LOAD_S16(ctx, 0x6E) >= 238) {
        ptbr_request_wide_dialogue(group, ctx);
    }
    ptbr_flow_snapshot(group, ctx, code);
    ptbr_layout_snapshot(group, ctx);
    return slot;
}
