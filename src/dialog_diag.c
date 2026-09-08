#include "modding.h"

typedef signed int s32;
typedef signed short s16;
typedef unsigned char u8;

extern s32 func_8000B8F0_C4F0(s32 group);
extern void* func_800148F0_154F0(void* pool, s32 size);
extern void* D_80167C48_168848[];
extern u8* D_8015C5C8_15D1C8;
extern u8 D_8005BB70_5C770[];
extern s32 D_801C77E0_1C83E0;
extern void* D_801C7800_1C8400;
extern u8 D_801C7808_1C8408[];

#define LOAD_S16(base, off) (*(s16*)((u8*)(base) + (off)))
#define STORE_S16(base, off, value) (*(s16*)((u8*)(base) + (off)) = (s16)(value))
#define LOAD_PTR(base, off) (*(void**)((u8*)(base) + (off)))
#define STORE_PTR(base, off, value) (*(void**)((u8*)(base) + (off)) = (void*)(value))

#define PTBR_NATIVE_WIDTH 256
#define PTBR_WIDE_WIDTH_NORMAL   288
#define PTBR_WIDE_WIDTH_PORTRAIT 304
#define PTBR_PIXEL_BYTES  2
#define PTBR_MAX_HEIGHT   72
#define PTBR_WIDE_BUFFER_BYTES ((PTBR_WIDE_WIDTH_PORTRAIT * PTBR_MAX_HEIGHT) / PTBR_PIXEL_BYTES)
#define PTBR_MSG_POOL_OFFSET 0xC7FA4

/*
 * Beta 0.8.36
 *
 * Safe base: Beta 0.8.33 / 0.8.25 reflow plus 9-row safety.
 * Portrait clearance is no longer inferred from runtime window state.
 *
 * Safe base: the same RECOMP_PATCH path at B8B0 proven by DIAG D and Beta 0.8.7.
 * No RECOMP_HOOK and no patch on C904.
 *
 * Width:
 * - Standard native dialogue contexts are 256 px wide and 7 strips tall.
 * - We allocate a normal game-RDRAM texture buffer (not mod-owned texture memory)
 *   and rebuild each 256-pixel scanline as 288 pixels.
 * - The extra 32 pixels are inserted immediately before the original right edge,
 *   so the native right border is moved to the new x=287 edge instead of being
 *   left as an internal border at x=255.
 * - The context texture width and right clipping edge are then widened to 288.
 *
 * Height:
 * - The working Beta 0.8.7 rule is preserved: a native 56 px / 7-row dialogue
 *   that reaches text line 3 is promoted to the game's native 72 px / 9-row
 *   texture. The widened RDRAM copy is rebuilt from that 9-row native texture.
 *
 * Overflow-driven width:
 * - The glyph renderer requests 288 px only when a line reaches the original
 *   safe horizontal limit.
 * - Beta 0.8.33 safety rule: only windows that were CREATED as 7-row / 56 px
 *   dialogues can widen. Windows created natively as 9-row / 72 px are left
 *   completely untouched by the horizontal width system.
 * - A 7-row window promoted to 9 rows by the existing height logic remains
 *   eligible, because its initial geometry is still recorded as 7 rows.
 * - Compact 5-row windows remain excluded.
 */

static void* s_last_ctx[3];
static void* s_native_texture[3];
static void* s_wide_request_ctx[3];
static s16 s_wide_requested[3];
static u8* s_wide_texture[3];
static s16 s_width_active[3];
static s16 s_target_width[3];
static s16 s_height_expanded[3];
static s16 s_initial_rows[3];

/*
 * Selection-menu cursor correction (Beta 0.8.12)
 *
 * The original selection-node builder records the red cursor at exactly the
 * same X coordinate where the option text starts. In PT-BR this makes the
 * cursor cover the first one or two letters of options such as "Passar a
 * noite" and "Nada".
 *
 * The selection nodes live in a small doubly-linked list rooted at
 * D_801C7808. State 2 in D_801C77E0 is the active selection-menu state.
 * We shift only the cursor-node X coordinates 16 px to the left once per
 * selection session. Text positions, bytes, line breaks, menus themselves,
 * and dialogue geometry are not changed.
 */
static s16 s_menu_cursor_shifted;

static void ptbr_adjust_selection_cursor(void) {
    u8* node;
    s32 count;

    if (D_801C77E0_1C83E0 != 2) {
        s_menu_cursor_shifted = 0;
        return;
    }

    if (s_menu_cursor_shifted) {
        return;
    }

    node = D_801C7808_1C8408;
    for (count = 0; count < 10 && node != (u8*)0; count++) {
        /* Node layout: +0 X, +4 Y, +8 prev, +0x0C next, +0x10 result. */
        (*(s32*)(node + 0x00)) -= 16;
        node = (u8*)LOAD_PTR(node, 0x0C);
    }

    s_menu_cursor_shifted = 1;
}

static void ptbr_copy_bytes(u8* dst, const u8* src, s32 count) {
    s32 i;
    for (i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

/*
 * Build a widened I4 window while preserving the native right border.
 * 288 px: duplicate a 32 px interior segment before the final 32 px tail.
 * 304 px: duplicate a 48 px interior segment before the same final tail.
 */
static void ptbr_build_wide_texture(u8* dst, const u8* src, s32 height, s32 target_width) {
    const s32 src_stride = PTBR_NATIVE_WIDTH / PTBR_PIXEL_BYTES;
    const s32 dst_stride = target_width / PTBR_PIXEL_BYTES;
    const s32 prefix_x = 224;
    const s32 prefix_bytes = prefix_x / PTBR_PIXEL_BYTES;
    const s32 tail_src = prefix_x / PTBR_PIXEL_BYTES;
    const s32 tail_bytes = (PTBR_NATIVE_WIDTH - prefix_x) / PTBR_PIXEL_BYTES;
    const s32 copy_x = (target_width >= PTBR_WIDE_WIDTH_PORTRAIT) ? 176 : 192;
    const s32 insert_src = copy_x / PTBR_PIXEL_BYTES;
    const s32 insert_bytes = (prefix_x - copy_x) / PTBR_PIXEL_BYTES;
    s32 y;

    for (y = 0; y < height; y++) {
        const u8* row_src = src + (y * src_stride);
        u8* row_dst = dst + (y * dst_stride);
        ptbr_copy_bytes(row_dst, row_src, prefix_bytes);
        ptbr_copy_bytes(row_dst + prefix_bytes, row_src + insert_src, insert_bytes);
        ptbr_copy_bytes(row_dst + prefix_bytes + insert_bytes, row_src + tail_src, tail_bytes);
    }
}

static u8* ptbr_alloc_wide_buffer(void) {
    void* pool;

    if (D_8015C5C8_15D1C8 == (u8*)0) {
        return (u8*)0;
    }

    /* Same game allocator/pool used by the native message-window constructor. */
    pool = (void*)(D_8015C5C8_15D1C8 + PTBR_MSG_POOL_OFFSET);
    return (u8*)func_800148F0_154F0(pool, PTBR_WIDE_BUFFER_BYTES);
}

static void ptbr_reset_group_state(s32 group, void* ctx) {
    s_last_ctx[group] = ctx;
    s_native_texture[group] = (void*)0;
    s_width_active[group] = 0;
    s_target_width[group] = 0;
    s_height_expanded[group] = 0;
    s_initial_rows[group] = (ctx != (void*)0) ? LOAD_S16(ctx, 0x42) : 0;
    /* Keep an overflow request that was raised by the font renderer for
     * this exact newly-created context before the state updater saw it. */
    if (ctx != (void*)0 && s_wide_request_ctx[group] == ctx) {
        s_wide_requested[group] = 1;
    } else {
        s_wide_requested[group] = 0;
        s_wide_request_ctx[group] = (void*)0;
    }
}

/* Called by the PT-BR glyph renderer when the current line is approaching
 * the original 256 px right edge. This does not alter the text or insert a
 * newline; it only requests a wider background for the same message. */
void ptbr_request_wide_dialogue(s32 group, void* ctx) {
    if ((unsigned int)group >= 3u || ctx == (void*)0) {
        return;
    }
    s_wide_request_ctx[group] = ctx;
    s_wide_requested[group] = 1;
}

static void ptbr_activate_width(s32 group, u8* ctx) {
    s16 rows = LOAD_S16(ctx, 0x42);
    s16 strip_height = LOAD_S16(ctx, 0x40);
    s32 native_height;
    s16 right_clip = LOAD_S16(ctx, 0x68);
    void* native_texture;
    s32 target_width;

    if (s_width_active[group] || !s_wide_requested[group]) {
        return;
    }

    /* Beta 0.8.33: carry the proven 0.8.32 crash fix into the 0.8.25
     * reflow base. Only contexts that were BORN as 7-row / 56 px dialogue
     * windows are eligible for horizontal widening. Native 9-row / 72 px
     * windows are intentionally left untouched by the width system.
     *
     * A 7-row window that the existing height logic later promotes to 9 rows
     * remains eligible because s_initial_rows[group] stays 7. */
    if (s_initial_rows[group] != 7) {
        s_wide_requested[group] = 0;
        s_wide_request_ctx[group] = (void*)0;
        return;
    }

    if (LOAD_S16(ctx, 0x3E) != PTBR_NATIVE_WIDTH ||
        strip_height != 8 || (rows != 7 && rows != 9)) {
        return;
    }
    native_height = (s32)rows * (s32)strip_height;
    /* Beta 0.8.22: keep portrait dialogues at the proven 288 px width.
     * The 304 px experiment fixed text clearance but pushed the visible right
     * frame beyond the safe window geometry in-game. The 48 px text inset is
     * preserved independently by the font renderer, while the background uses
     * the same 288 px width already validated by Beta 0.8.12. */
    target_width = PTBR_WIDE_WIDTH_NORMAL;

    native_texture = LOAD_PTR(ctx, 0x04);
    if (native_texture == (void*)0) {
        return;
    }

    if (s_wide_texture[group] == (u8*)0) {
        s_wide_texture[group] = ptbr_alloc_wide_buffer();
        if (s_wide_texture[group] == (u8*)0) {
            return;
        }
    }

    s_native_texture[group] = native_texture;
    ptbr_build_wide_texture(s_wide_texture[group], (const u8*)native_texture, native_height, target_width);

    STORE_PTR(ctx, 0x04, s_wide_texture[group]);
    STORE_S16(ctx, 0x3E, target_width);

    /* Preserve the current open/close progress, with a +16 px full-open edge. */
    if (right_clip > 0) {
        s32 widened_clip = ((s32)right_clip * target_width + 127) / PTBR_NATIVE_WIDTH;
        if (widened_clip > target_width) {
            widened_clip = target_width;
        }
        STORE_S16(ctx, 0x68, widened_clip);
    }

    s_target_width[group] = (s16)target_width;
    s_width_active[group] = 1;
    s_wide_requested[group] = 0;
    s_wide_request_ctx[group] = (void*)0;
}

static void ptbr_update_dialogue_box(s32 group) {
    u8* ctx;
    s16 line_index;
    s16 rows;

    if ((unsigned int)group >= 3u) {
        return;
    }

    ctx = (u8*)D_80167C48_168848[group];
    if (ctx == (u8*)0) {
        ptbr_reset_group_state(group, (void*)0);
        return;
    }

    if (s_last_ctx[group] != (void*)ctx) {
        ptbr_reset_group_state(group, (void*)ctx);
    } else if (s_width_active[group] && LOAD_S16(ctx, 0x3E) == PTBR_NATIVE_WIDTH) {
        /* The game reused/reinitialized the same allocation for a new window. */
        ptbr_reset_group_state(group, (void*)ctx);
    }

    line_index = LOAD_S16(ctx, 0x72);
    rows = LOAD_S16(ctx, 0x42);

    ptbr_activate_width(group, ctx);

    /* Preserve Beta 0.8.7 behavior: third line promotes 56 -> 72 px. */
    if (!s_height_expanded[group] && line_index >= 2 && rows == 7) {
        if (s_width_active[group]) {
            ptbr_build_wide_texture(s_wide_texture[group], D_8005BB70_5C770, 72, s_target_width[group]);
            STORE_PTR(ctx, 0x04, s_wide_texture[group]);
        } else {
            STORE_PTR(ctx, 0x04, D_8005BB70_5C770);
        }

        STORE_S16(ctx, 0x42, 9);
        STORE_S16(ctx, 0x6A, LOAD_S16(ctx, 0x6A) + 16);
        s_height_expanded[group] = 1;
        return;
    }

    /* A new short page returns to 7 rows and keeps 288 px if overflow activated it. */
    if (s_height_expanded[group] && line_index < 2) {
        if (s_width_active[group] && s_native_texture[group] != (void*)0) {
            ptbr_build_wide_texture(
                s_wide_texture[group],
                (const u8*)s_native_texture[group],
                56,
                s_target_width[group]
            );
            STORE_PTR(ctx, 0x04, s_wide_texture[group]);
        } else if (s_native_texture[group] != (void*)0) {
            STORE_PTR(ctx, 0x04, s_native_texture[group]);
        }
        STORE_S16(ctx, 0x42, 7);
        STORE_S16(ctx, 0x6A, LOAD_S16(ctx, 0x6A) - 16);
        s_height_expanded[group] = 0;
    }
}

RECOMP_PATCH void func_8000B8B0_C4B0(void) {
    s32 group;
    for (group = 0; group < 3; group++) {
        /* Let the native state machine update the window first. */
        (void)func_8000B8F0_C4F0(group);

        /* Apply PT-BR geometry after native state updates, so D400/D4E0
         * cannot overwrite the widened right edge before the next frame. */
        ptbr_update_dialogue_box(group);

        /* Selection nodes are global, so this is idempotent and only acts
         * once while the menu state is active. */
        ptbr_adjust_selection_cursor();

        if (D_80167C48_168848[group] != (void*)0 && s_width_active[group]) {
            u8* ctx = (u8*)D_80167C48_168848[group];
            STORE_S16(ctx, 0x3E, s_target_width[group]);

            /* ctx+0x30 == 4 is the native normal/full-open window state.
             * It is retained only for the proven 288 px clip correction.
             * Beta 0.8.36 no longer interprets this value as portrait state. */
            if (LOAD_S16(ctx, 0x30) == 4) {
                STORE_S16(ctx, 0x64, 0);
                STORE_S16(ctx, 0x68, s_target_width[group]);
            }
        }
    }
}
