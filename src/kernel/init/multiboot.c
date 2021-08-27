#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "multiboot.h"
#include "terminal.h"

struct multiboot_info_struct {
    uint32_t flags;
    uint8_t not_used[40];
    uint32_t mmap_length;
    uint32_t mmap_addr;
};

struct multiboot_mmap_entry {
    uint32_t entry_size;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

static uint32_t multiboot_get_info_struct_addr(void)
{
    uint32_t a;
    /* Bootloader passed an address of the multiboot information structure in
     * EBX.
     */
    __asm__("mov %%ebx, %0" : "=g"(a));

    return a;
}

void multiboot_get_memory_map(void) { ; }

void multiboot_print_memory_map(void)
{
    struct __attribute__((packed)) multiboot_info_struct *info_struct =
        (struct multiboot_info_struct *)multiboot_get_info_struct_addr();

    terminal_printf("Multiboot info structure address: %x\n",
                    (uint32_t)info_struct);
    terminal_printf("Memory map length: %u\n", info_struct->mmap_length);
    terminal_printf("Memory map address: %x\n", info_struct->mmap_addr);
    terminal_printf("Memory map:\n");
    terminal_printf(
        "  BaseAddrHigh  BaseAddrLow  LengthHigh  LengthLow   Type\n");

    uint8_t l = 0;
    while (l < info_struct->mmap_length) {
        struct __attribute__((packed)) multiboot_mmap_entry *me =
            (struct multiboot_mmap_entry *)(info_struct->mmap_addr + l);

        terminal_printf("  %x    %x   %x  %x  ", me->base_addr_high,
                        me->base_addr_low, me->length_high, me->length_low);

        switch (me->type) {
        case 1:
            terminal_printf("AddressRangeMemory");
            break;

        case 2:
            terminal_printf("AddressRangeReserved");
            break;

        case 3:
            terminal_printf("AddressRangeACPI");
            break;

        case 4:
            terminal_printf("AddressRangeNVS");
            break;

        case 5:
            terminal_printf("AddressRangeUnusable");
            break;

        default:
            terminal_printf("Undefined");
            break;
        }
        terminal_printf("\n");

        l += me->entry_size;
    }
}
