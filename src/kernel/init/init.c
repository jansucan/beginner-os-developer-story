/* This code is based on the example from:
 *
 * https://wiki.osdev.org/Bare_Bones
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "assert.h"
#include "multiboot.h"
#include "pci.h"
#include "terminal.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error                                                                         \
    "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#define USB_CONTROLLER_PCI_VENDOR_ID 0x8086
#define USB_CONTROLLER_PCI_DEVICE_ID 0x24cd
#define USB_CONTROLLER_NAME                                                    \
    "Intel 82801DB/DBM (ICH4/ICH4-M) USB2 EHCI Controller"

static void print_pci_device_list_header(void)
{
    terminal_printf("\n");
    terminal_printf("PCI devices:\n");
    terminal_printf("  VendorID   DeviceID   Class      Subclass   ProgIF\n");
}

static void
print_pci_header_common(const struct pci_header_common *const pci_header)
{
    terminal_printf("  %x", pci_header->vendor_id);
    terminal_printf(" %x", pci_header->device_id);
    terminal_printf(" %x", pci_header->class_code);
    terminal_printf(" %x", pci_header->subclass);
    terminal_printf(" %x\n", pci_header->prog_if);
}

static void
print_usb_controller_info(const struct pci_function_address *const addr)
{
    terminal_printf("Found USB controller:\n");
    terminal_printf("  name: %s\n", USB_CONTROLLER_NAME);
    terminal_printf("  PCI:  bus=%x  device=%x  function=%x\n",
                    addr->bus_number, addr->device_number,
                    addr->function_number);
}

// cppcheck-suppress unusedFunction
void kernel_main(void)
{
    terminal_initialize();

    multiboot_print_memory_map();

    // Print out list of PCI functions and find the USB controller
    print_pci_device_list_header();

    struct pci_function_address pci_address;
    struct pci_header_common pci_header;

    struct pci_function_address usb_controller_pci_address;
    struct pci_header_common usb_controller_pci_header;

    // Suppress compiler warning about uninitialized variable
    usb_controller_pci_address.bus_number = 0;
    usb_controller_pci_header.vendor_id = PCI_INVALID_VENDOR_ID;

    pci_function_iterator_init(&pci_address, &pci_header);
    while (pci_function_iterator_next(&pci_address, &pci_header)) {
        print_pci_header_common(&pci_header);
        if ((pci_header.vendor_id == USB_CONTROLLER_PCI_VENDOR_ID) &&
            (pci_header.device_id == USB_CONTROLLER_PCI_DEVICE_ID)) {
            usb_controller_pci_address = pci_address;
            usb_controller_pci_header = pci_header;
        }
    }
    terminal_printf("\n");

    ASSERT(
        (usb_controller_pci_header.vendor_id == USB_CONTROLLER_PCI_VENDOR_ID) &&
            (usb_controller_pci_header.device_id ==
             USB_CONTROLLER_PCI_DEVICE_ID),
        "USB controller not found");

    print_usb_controller_info(&usb_controller_pci_address);

    for (uint8_t i = 0; i < PCI_BASE_ADDRESS_REGISTER_COUNT; ++i) {
        terminal_printf("  BAR register %d: %x\n", i,
                        pci_read_bar_register(&usb_controller_pci_address, i));
    }
}
