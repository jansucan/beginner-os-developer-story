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
    terminal_write_string("\n");
    terminal_write_string("PCI devices:\n");

    terminal_write_string(
        "  VendorID   DeviceID   Class      Subclass   ProgIF\n");
}

static void
print_pci_header_common(const struct pci_header_common *const pci_header)
{
    terminal_write_string("  ");
    terminal_write_uint32(pci_header->vendor_id);
    terminal_write_string(" ");
    terminal_write_uint32(pci_header->device_id);
    terminal_write_string(" ");
    terminal_write_uint32(pci_header->class_code);
    terminal_write_string(" ");
    terminal_write_uint32(pci_header->subclass);
    terminal_write_string(" ");
    terminal_write_uint32(pci_header->prog_if);
    terminal_write_string("\n");
}

static void
print_usb_controller_info(const struct pci_function_address *const addr)
{
    terminal_write_string("Found USB controller:\n");
    terminal_write_string("  name: ");
    terminal_write_string(USB_CONTROLLER_NAME);
    terminal_write_string("\n");
    terminal_write_string("  PCI:  bus=");
    terminal_write_uint32(addr->bus_number);
    terminal_write_string("  device=");
    terminal_write_uint32(addr->device_number);
    terminal_write_string("  function=");
    terminal_write_uint32(addr->function_number);
    terminal_write_string("\n");
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
    terminal_write_string("\n");

    ASSERT(
        (usb_controller_pci_header.vendor_id == USB_CONTROLLER_PCI_VENDOR_ID) &&
            (usb_controller_pci_header.device_id ==
             USB_CONTROLLER_PCI_DEVICE_ID),
        "USB controller not found");

    print_usb_controller_info(&usb_controller_pci_address);
}
