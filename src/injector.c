#include <injector.h>
#include <mach_o.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>

void *mach_o_inject_rally(void *image, void *rally, size_t rally_size)
{
    struct mach_o_header *header = image;
    union mach_o_command *lc = NULL;
    struct mach_o_segment_command *seg = NULL;
    uint64_t max_vm_addr = 0;
    uint64_t max_file_off = 0;

    if (header->magic != MACH_O_MAGIC || header->file_type != MH_FILESET) {
        panic("Invalid mach-o magic(%x) or file_type(%x)\n", header->magic, header->file_type);
    }

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_SEGMENT_64) {
            if (lc->segment.vm_addr + lc->segment.vm_size > max_vm_addr) {
                max_vm_addr = lc->segment.vm_addr + lc->segment.vm_size;
            }

            if (lc->segment.file_off + lc->segment.file_size > max_file_off) {
                max_file_off = lc->segment.file_off + lc->segment.file_size;
            }
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    max_vm_addr = ALIGN_UP(max_vm_addr, 0x1000);
    max_file_off = ALIGN_UP(max_file_off, 0x1000);

    memcpy(image + max_file_off, rally, rally_size);

    seg = (struct mach_o_segment_command *)((uint8_t *)(header + 1) + header->size_of_cmds);

    seg->cmd.cmd = LC_SEGMENT_64;
    seg->cmd.cmd_size = sizeof(*seg);
    strcpy(seg->segname, "__RALLY");
    seg->vm_addr = max_vm_addr;
    seg->vm_size = rally_size;
    seg->file_off = max_file_off;
    seg->file_size = rally_size;
    seg->max_prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    seg->init_prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
    seg->n_sects = 1;
    seg->flags = 0;

    header->n_cmds++;
    header->size_of_cmds += seg->cmd.cmd_size;

    return (uint64_t *)max_vm_addr;
}