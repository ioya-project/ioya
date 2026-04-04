#include <kernel_patches.h>
#include <patcher.h>
#include <string.h>
#include <utils.h>

#define NOP 0xd503201f
#define RET 0xd65f03c0
#define PACIBSP 0xd503237f

#define MOV_IMM_MASK 0x001FFFE0

#define B(target, pc) (0x14000000 | ((((int64_t)(target) - (int64_t)(pc)) >> 2) & 0x03FFFFFF))
#define MOV_IMM(reg, value)                                                                        \
    (0xD2800000 | (((uint32_t)(value) & 0xFFFF) << 5) | ((uint32_t)(reg) & 0x1F))
#define MOV(rd, rm) (0xAA0003E0 | (((uint32_t)(rm)) << 16) | ((uint32_t)(rd)))

static bool skip_pac_key_setup_callback(void *ctx, uint32_t *opcode)
{
    uint32_t *start = find_next_instr(opcode, 48, 0xF2800000, 0xFF800000, 5);
    if (start == NULL)
        return false;

    opcode[5] = B(start, opcode + 5);
    return true;
}

static bool aprr_el1_callback(void *ctx, uint32_t *opcode)
{
    if ((opcode[1] & 0xFF800000) == 0xF2800000) {
        uint32_t rt = opcode[0] & 0x1F;
        memcpy(&opcode[0], &opcode[1], 4 * sizeof(uint32_t));
        opcode[4] = MOV(rt, opcode[0] & 0x1F);
        return true;
    }

    if ((opcode[1] & 0x7FE0001F) == 0x6B00001F) {
        opcode[0] = MOV(opcode[0] & 0x1F, opcode[-1] & 0x1F);
        return true;
    }

    return false;
}

static bool amfi_vmm_callback(void *ctx, uint32_t *opcode)
{
    const char *str = (const char *)(((uint64_t)(opcode) & ~0xFFFULL) + adrp_offset(opcode[0]) +
                                     add_imm(opcode[1]));

    if (strcmp(str, "kern.hv_vmm_present") != 0) {
        return false;
    }

    uint32_t *start = find_next_instr(opcode, 16, 0x34000000, 0x7F000000, 0);
    if (start == NULL)
        return false;

    start[0] = NOP;
    return true;
}

static bool amfi_trust_all_binaries_callback(void *ctx, uint32_t *opcode)
{
    uint32_t *start = find_prev_instr(opcode, 32, PACIBSP, 0xFFFFFFFF, 0);
    if (start == NULL)
        return false;

    start[0] = 0xD2800020; // mov x0, #0x1
    start[1] = 0xB4000042; // cbz x2, #0x4
    start[2] = 0xF9000040; // str x0, [x2]
    start[3] = RET;

    return true;
}

static bool trust_all_binaries_callback(void *ctx, uint32_t *opcode)
{
    uint32_t *start = find_prev_instr(opcode, 48, PACIBSP, 0xFFFFFFFF, 0);
    if (start == NULL)
        return false;

    start[0] = 0x52802020; // mov w0, #0x101
    start[1] = RET;

    return true;
}

static bool kmsg_signature_callback(void *ctx, uint32_t *opcode)
{
    const char *str = (const char *)(((uint64_t)(opcode) & ~0xFFFULL) + adrp_offset(opcode[0]) +
                                     add_imm(opcode[1]));

    if (strcmp(str, "ikm_validate_sig: %s signature mismatch: kmsg=0x%p, id=%d, sig=0x%zx "
                    "(expected 0x%zx) @%s:%d") != 0) {
        return false;
    }

    uint32_t *start = find_prev_instr(opcode, 128, PACIBSP, 0xFFFFFFFF, 0);
    if (start == NULL)
        return false;

    start[0] = RET;

    return true;
}

static bool tbnz_to_b_callback(void *ctx, uint32_t *opcode)
{
    opcode[1] = 0x14000000 | ((opcode[1] << 13) >> 18);
    return true;
}

static bool apple_hid_callback(void *ctx, uint32_t *opcode)
{
    uint32_t *start = find_next_instr(opcode, 128, 0xB5000015, 0xFF00001F, 3);
    if (start == NULL)
        return false;

    opcode[3] = B(start, opcode + 3);
    return true;
}

static bool vm_map_entry_cs_associate_callback(void *ctx, uint32_t *opcode)
{
    uint32_t *start = find_prev_instr(opcode, 128, PACIBSP, 0xFFFFFFFF, 0);
    if (start == NULL)
        return false;

    start[0] = MOV(0, 0);
    start[1] = RET;
    return true;
}

void apply_kernel_patches()
{
    {
        // We cannot emulate these PAC system registers because the vbar_el1 is already set
        // to the vector table with virtual address, but MMU is not enabled yet
        const uint32_t matches[] = {
            0x90000000, // adrp x?, #?
            0x91000000, // add x?, x?, #?
            0x8B000000, // add x?, x?, x??
            0xCB000000, // sub x?, x?, x??
            0xD518C000, // msr vbar_el1, x?
        };
        const uint32_t masks[] = {
            0x9F000000, // adrp x?, #?
            0xFFC00000, // add x?, x?, #?
            0xFFE00000, // add x?, x?, x??
            0xFFE00000, // sub x?, x?, x??
            0xFFFFFFE0, // msr vbar_el1, x?
        };
        patcher_callback_patch("Skip PAC key setup", "com.apple.kernel", "__TEXT_EXEC", "__text",
                               matches, masks, ARRAY_LEN(matches), skip_pac_key_setup_callback,
                               false);
    }

    // We cannot emulate system registers in the exception handler because when an exception occurs
    // on the msr/mrs instruction, exception context registers will be rewritten
    {
        const uint32_t matches[] = {
            0xD53CF220, // mrs x?, S3_4_c15_c2_1
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // mrs x?, S3_4_c15_c2_1
        };
        patcher_callback_patch("Patch APRR_EL1 in exception handlers", NULL, NULL, NULL, matches,
                               masks, ARRAY_LEN(matches), aprr_el1_callback, false);
    }

    {
        const uint32_t matches[] = {
            0x91000000, // add x?, x?, #?
            0xF90003E0, // str x?, [sp, #?]
            0x2A000000, // orr w?, w?, w?
            0x35000000, // cbnz w?, #?
        };
        const uint32_t masks[] = {
            0xFFC00000, // add x?, x?, #?
            0xFFC003E0, // str x?, [sp, #?]
            0xFFE00000, // orr w?, w?, w?
            0xFF000000, // cbnz w?, #?
        };
        const uint32_t replace[] = {
            0xD2800000,
        };
        const uint32_t replace_mask[] = {
            0x0000001F,
        };
        patcher_replace_patch("Enable kprintf", "com.apple.kernel", "__TEXT_EXEC", "__text",
                              matches, masks, ARRAY_LEN(matches), replace, replace_mask, 2,
                              ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0x90000000, // adrp x?, 0x?
            0x91000000, // add x?, x?, 0x?
        };
        const uint32_t masks[] = {
            0x9F000000, // adrp x?, 0x?
            0xFFC00000, // add x?, x?, 0x?
        };
        patcher_callback_patch("Force AMFI to treat this as VM",
                               "com.apple.driver.AppleMobileFileIntegrity", "__TEXT_EXEC", "__text",
                               matches, masks, ARRAY_LEN(matches), amfi_vmm_callback, false);
    }

    {
        const uint32_t matches[] = {
            0x94000000, // bl 0x?
            0xAA0003E0, // mov x?, x?
            0xAA0003E0, // mov x?, x?
            0xAA0003E0, // mov x?, x?
            0x94000000, // bl 0x?
            0xAA0003E0, // mov x?, x?
            0x94000000, // bl 0x?
            0xAA0003E0, // mov x?, x?
            0x34000000, // cbz w?, #?
        };
        const uint32_t masks[] = {
            0xFC000000, // bl 0x?
            0xFFE003E0, // mov x?, x?
            0xFFE003E0, // mov x?, x?
            0xFFE003E0, // mov x?, x?
            0xFC000000, // bl 0x?
            0xFFE003E0, // mov x?, x?
            0xFC000000, // bl 0x?
            0xFFE003E0, // mov x?, x?
            0xFF000000, // cbz w?, #?
        };
        const uint32_t replace[] = {
            MOV_IMM(0, 0),
        };
        patcher_replace_patch("Skip fipspost_post_integrity call", "com.apple.kec.corecrypto",
                              "__TEXT_EXEC", "__text", matches, masks, ARRAY_LEN(matches), replace,
                              NULL, 4, ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0x910003E0, // mov x?, sp
            0x94000000, // bl #?
            0xAA0003E0, // mov x?, x?
            0x35000000, // cbnz x?, #?
            0xB4000000, // cbz x?, #?
        };
        const uint32_t masks[] = {
            0xFFE003E0, // mov x?, sp
            0xFC000000, // bl #?
            0xFFE003E0, // mov x?, x?
            0xFF000000, // cbnz x?, #?
            0xFF000000, // cbz x?, #?
        };
        patcher_callback_patch(
            "Trust all binaries (AMFI)", "com.apple.driver.AppleMobileFileIntegrity", "__TEXT_EXEC",
            "__text", matches, masks, ARRAY_LEN(matches), amfi_trust_all_binaries_callback, false);
    }

    {
        const uint32_t matches[] = {
            0xD29DCFC0, // mov x?, #0xEE7E
            0xD73F0800, // blraa x?, x?
            0xF9400000, // ldr x?, [x?, #?]
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // mov x?, #0xEE7E
            0xFFFFFC00, // blraa x?, x?
            0xFFC00000, // ldr x?, [x?, #?]
        };
        patcher_callback_patch("Trust all binaries", "com.apple.kernel", "__PPLTEXT", "__text",
                               matches, masks, ARRAY_LEN(matches), trust_all_binaries_callback,
                               false);
    }

    {
        const uint32_t matches[] = {
            0xA94013A3, // ldp x3, x4, [x29, #?]
            0x94000000, // bl #?
        };
        const uint32_t masks[] = {
            0xFFC07FFF, // ldp x3, x4, [x?, #?]
            0xFC000000, // bl #?
        };
        const uint32_t replace[] = {
            MOV_IMM(0, 0), // mov x0, #0
        };
        patcher_replace_patch("Bypass Code Integrity", "com.apple.kernel", "__PPLTEXT", "__text",
                              matches, masks, ARRAY_LEN(matches), replace, NULL, 1,
                              ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0x90000000, // adrp x?, 0x?
            0x91000000, // add x?, x?, 0x?
        };
        const uint32_t masks[] = {
            0x9F000000, // adrp x?, 0x?
            0xFFC00000, // add x?, x?, 0x?
        };
        patcher_callback_patch("Patch kmsg signature validation", "com.apple.kernel", "__TEXT_EXEC",
                               "__text", matches, masks, ARRAY_LEN(matches),
                               kmsg_signature_callback, false);
    }

    {
        const uint32_t matches[] = {
            0x39400000, // ldrb/ldr w?, [x?, #?]
            0x37280000, // tbnz w?, #5, 0x?
            0xB81003BF, // stur wzr, [x29, #?]
        };
        const uint32_t masks[] = {
            0x7FE00400, // ldrb/ldr w?, [x?, #?]
            0xFFF80000, // tbnz w?, #5, 0x?
            0xFFF003FF, // stur wzr, [x29, #?]
        };
        patcher_callback_patch("Disable userspace aslr", "com.apple.kernel", "__TEXT_EXEC",
                               "__text", matches, masks, ARRAY_LEN(matches), tbnz_to_b_callback,
                               false);
    }

    {
        const uint32_t matches[] = {
            0xD5300240, // mrs x?, mdscr_el1
            0xB2740000, // orr x?, x?, #0x1000
            0xD5100240, // msr mdscr_el1, x?
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // mrs x?, mdscr_el1
            0xFFFFFC00, // orr x?, x?, #0x1000
            0xFFFFFFE0, // msr mdscr_el1, x?
        };
        patcher_callback_patch("Skip apple hid regs setup", "com.apple.kernel", "__TEXT_EXEC",
                               "__text", matches, masks, ARRAY_LEN(matches), apple_hid_callback,
                               false);
    }

    {
        const uint32_t matches[] = {
            0xB9404800, // ldr w?, [x?, #0x48]
            0x2A602C00, // orn w?, w?, w?, LSR #0xb
            0x6A000000, // tst w?, w?
            0x54000001, // b.ne #?
        };
        const uint32_t masks[] = {
            0xFFFFFC00, // ldr w?, [x?, #0x48]
            0xFFE0FC00, // orn w?, w?, w?, LSR #0xb
            0xFFE00000, // tst w?, w?
            0xFF00000F, // b.ne #?
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("vm_map_protect", "com.apple.kernel", "__TEXT_EXEC", "__text",
                              matches, masks, ARRAY_LEN(matches), replace, NULL, 3,
                              ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0x37480000, // tbnz w?, #0x9 #?
            0x52800000, // mov w?, #0
            0xB6600000, // tbz x?, #0x2c #?
            0x36680000, // tbz w?, #0xd #?
        };
        const uint32_t masks[] = {
            0xFFF80000, // tbnz w?, #0x9 #?
            0xFF800000, // mov w?, #0
            0xFFE00000, // tbz x?, #0x2c #?
            0xFFF80000, // tbz w?, #0xd #?
        };
        patcher_callback_patch("vm_map_entry_cs_associate", "com.apple.kernel", "__TEXT_EXEC",
                               "__text", matches, masks, ARRAY_LEN(matches),
                               vm_map_entry_cs_associate_callback, false);
    }

    {
        const uint32_t matches[] = {
            0xD5381000, // mrs x?, sctlr_el1
            0x927FF800, // and x?, x?, #-0x2
            0xD5181000, // msr sctlr_el1, x?
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // mrs x?, sctlr_el1
            0xFFFFFC00, // and x?, x?, #-0x2
            0xFFFFFFE0, // msr sctlr_el1, x?
        };
        const uint32_t replace[] = {
            RET,
        };
        patcher_replace_patch("ml_enable_monitor", "com.apple.kernel", "__TEXT_EXEC", "__text",
                              matches, masks, ARRAY_LEN(matches), replace, NULL, 0,
                              ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0xD5381000, // mrs x?, sctlr_el1
            0x37F80000, // tbnz w?, #0x1f, #?
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // mrs x?, sctlr_el1
            0xFFF80000, // tbnz w?, #0x1f, #?
        };
        patcher_callback_patch("Disable PAC for EL0", "com.apple.kernel", "__TEXT_EXEC", "__text",
                               matches, masks, ARRAY_LEN(matches), tbnz_to_b_callback, false);
    }

    {
        const uint32_t matches[] = {
            0xF2BF8A80, // movk x?, #0xfc54, LSL #16
            0xF28F27A0, // movk x?, #0x793d, LSL #0
        };
        const uint32_t masks[] = {
            0xFFFFFFE0, // movk x?, #0xfc54, LSL #16
            0xFFFFFFE0, // movk x?, #0x793d, LSL #0
        };
        const uint32_t replace[] = {
            0xF2A68A80, // movk x?, #0x3454, LSL #16
            0xF28B27A0, // movk x?, #0x593d, LSL #0
        };
        const uint32_t replace_mask[] = {
            0x0000001F,
            0x0000001F,
        };
        patcher_replace_patch("Disable PAC in sctlr_el1", "com.apple.kernel", "__TEXT_EXEC",
                              "__text", matches, masks, ARRAY_LEN(matches), replace, replace_mask,
                              0, ARRAY_LEN(replace), false);
    }

    {
        const uint32_t matches[] = {
            0xD518F400, // mrs/msr x?, s3_0_c15_c4_?
        };
        const uint32_t masks[] = {
            0xFFDFFF00, // mrs/msr x?, s3_0_c15_c4_?
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("SYS_APL_HID4/EHID4", "com.apple.kernel", "__TEXT_EXEC", "__text",
                              matches, masks, ARRAY_LEN(matches), replace, NULL, 0,
                              ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0x00201420, // genter
        };
        const uint32_t masks[] = {
            0xFFFFFFFF, // genter
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("genter", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0x00201400, // gexit
        };
        const uint32_t masks[] = {
            0xFFFFFFFF, // gexit
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("gexit", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC10000, // pacia, paciza
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // pacia, paciza
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("pacia/paciza", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC10400, // pacib, pacizb
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // pacib, pacizb
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("pacib/pacizb", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC10800, // pacda, pacdza
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // pacda, pacdza
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("pacda/pacdza", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC10C00, // pacdb, pacdzb
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // pacdb, pacdzb
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("pacdb/pacdzb", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC11000, // autia, autiza
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // autia, autiza
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("autia/autiza", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC11800, // autda, autdza
        };
        const uint32_t masks[] = {
            0xFFFFDC00, // autda, autdza
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("autda/autdza", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xD65F0BFF, // retaa, retab
        };
        const uint32_t masks[] = {
            0xFFFFFBFF, // retaa, retab
        };
        const uint32_t replace[] = {
            RET,
        };
        patcher_replace_patch("retaa/retab", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xDAC143E0, // xpaci, xpacd
        };
        const uint32_t masks[] = {
            0xFFFFFBE0, // xpaci, xpacd
        };
        const uint32_t replace[] = {
            NOP,
        };
        patcher_replace_patch("xpaci/xpacd", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, NULL, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0x9AC03000, // pacga
        };
        const uint32_t masks[] = {
            0xFFE0FC00, // pacga
        };
        const uint32_t replace[] = {
            MOV_IMM(0, 1),
        };
        const uint32_t replace_mask[] = {
            0x0000001F,
        };
        patcher_replace_patch("pacga", NULL, NULL, NULL, matches, masks, ARRAY_LEN(matches),
                              replace, replace_mask, 0, ARRAY_LEN(replace), true);
    }

    {
        const uint32_t matches[] = {
            0xD61F0800, // braa, braaz, brab, brabz
        };
        const uint32_t masks[] = {
            0xFEFFF800, // braa, braaz, brab, brabz
        };
        const uint32_t replace[] = {
            0xD61F0000,
        };
        const uint32_t replace_mask[] = {
            0x000003E0,
        };
        patcher_replace_patch("braa/braaz/brab/brabz", NULL, NULL, NULL, matches, masks,
                              ARRAY_LEN(matches), replace, replace_mask, 0, ARRAY_LEN(replace),
                              true);
    }

    {
        const uint32_t matches[] = {
            0xD63F0800, // blraa, blraaz, blrab, blrabz
        };
        const uint32_t masks[] = {
            0xFEFFF800, // blraa, blraaz, blrab, blrabz
        };
        const uint32_t replace[] = {
            0xD63F0000,
        };
        const uint32_t replace_mask[] = {
            0x000003E0,
        };
        patcher_replace_patch("blraa/blraaz/blrab/blrabz", NULL, NULL, NULL, matches, masks,
                              ARRAY_LEN(matches), replace, replace_mask, 0, ARRAY_LEN(replace),
                              true);
    }
}