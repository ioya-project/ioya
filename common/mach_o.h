#ifndef MACHO_H_
#define MACHO_H_

#include <stddef.h>
#include <stdint.h>

#define MACH_O_MAGIC (0xfeedfacf)

#define LC_UNIXTHREAD (0x5)
#define LC_SEGMENT_64 (0x19)
#define LC_BUILD_VERSION (0x32)
#define LC_REQ_DYLD (0x80000000)
#define LC_FILESET_ENTRY (0x35 | LC_REQ_DYLD)

#define MH_EXECUTE (0x2)
#define MH_FILESET (0xc)

#define VM_PROT_NONE (0x0)
#define VM_PROT_READ (0x1)
#define VM_PROT_WRITE (0x2)
#define VM_PROT_EXECUTE (0x4)

#define S_ATTR_SOME_INSTRUCTIONS 0x00000400
#define S_ATTR_PURE_INSTRUCTIONS 0x80000000

#define XNU_CMDLINE_LEN 608

struct mach_o_header {
    uint32_t magic;
    uint32_t cpu_type;
    uint32_t cpu_subtype;
    uint32_t file_type;
    uint32_t n_cmds;
    uint32_t size_of_cmds;
    uint32_t flags;
    uint32_t reserved;
};

struct mach_o_load_command {
    uint32_t cmd;
    uint32_t cmd_size;
};

struct mach_o_segment_command {
    struct mach_o_load_command cmd;
    char segname[16];
    uintptr_t vm_addr;
    uintptr_t vm_size;
    uintptr_t file_off;
    uintptr_t file_size;
    uint32_t max_prot;
    uint32_t init_prot;
    uint32_t n_sects;
    uint32_t flags;
};

struct mach_o_thread_command {
    struct mach_o_load_command cmd;
    uint32_t flavor;
    uint32_t count;
    struct {
        uint64_t x[29];
        uint64_t fp;
        uint64_t lr;
        uint64_t sp;
        uint64_t pc;
        uint32_t cpsr;
        uint32_t flags;
    } state;
};

struct mach_o_fileset_entry_command {
    struct mach_o_load_command cmd;
    uint64_t vm_addr;
    uint64_t file_off;
    uint32_t entry_id;
    uint32_t reserved;
};

struct mach_o_build_version_command {
    struct mach_o_load_command cmd;
    uint32_t platform;
    uint32_t min_os;
    uint32_t sdk;
    uint32_t n_tools;
};

union mach_o_command {
    struct mach_o_load_command cmd;
    struct mach_o_segment_command segment;
    struct mach_o_thread_command thread;
    struct mach_o_fileset_entry_command fileset;
    struct mach_o_build_version_command version;
};

struct mach_o_range {
    uint64_t base;
    uint64_t end;
};

struct mach_o_load_info {
    struct mach_o_range range;
    uint64_t text_base;
    void *kernel;
    void *entry;
};

struct mach_o_section {
    char sectname[16];
    char segname[16];
    uint64_t vm_addr;
    uint64_t size;
    uint32_t file_off;
    uint32_t align;
    uint32_t rel_off;
    uint32_t n_reloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
};

struct xnu_boot_arguments {
    uint16_t revision;
    uint16_t version;

    uint64_t virt_base;
    uint64_t phys_base;
    uint64_t mem_size;
    uint64_t kernel_top;

    struct {
        uint64_t base_addr;
        uint64_t display;
        uint64_t row_bytes;
        uint64_t width;
        uint64_t height;
        union {
            struct {
                uint8_t depth : 8;
                uint8_t rotate : 8;
                uint8_t scale : 8;
                uint8_t boot_rotate : 8;
            };
            uint64_t raw;
        } depth;
    } video_args;

    uint32_t machine_type;
    uint64_t device_tree_addr;
    uint32_t device_tree_size;
    char cmdline[XNU_CMDLINE_LEN];
    uint64_t boot_flags;
    uint64_t mem_size_actual;
};

struct mach_o_header *mach_o_get_fileset_header(struct mach_o_header *header, const char *entry);
struct mach_o_segment_command *mach_o_get_segment(struct mach_o_header *header,
                                                  const char *segment_name);
struct mach_o_section *mach_o_get_section(struct mach_o_header *header, const char *segment_name,
                                          const char *section_name);
struct mach_o_range mach_o_get_section_va_range(struct mach_o_header *header,
                                                const char *segment_name, const char *section_name);
struct mach_o_range mach_o_get_section_file_range(struct mach_o_header *header,
                                                  const char *segment_name,
                                                  const char *section_name);
uint32_t mach_o_get_build_version(struct mach_o_header *header);
struct mach_o_load_info mach_o_load_image(void *image, uint64_t load_address);

#endif
