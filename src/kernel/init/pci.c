#include <stdbool.h>

#include "io_port.h"
#include "pci.h"

#define PCI_IO_CONFIG_ADDRESS 0xCF8
#define PCI_IO_CONFIG_DATA 0xCFC

#define PCI_MAX_BUS_COUNT 256
#define PCI_MAX_DEVICE_COUNT 32
#define PCI_MAX_FUNCTION_COUNT 8

#define PCI_CONFIG_DEVICE_NUMBER_MASK 0x1f
#define PCI_CONFIG_FUNCTION_NUMBER_MASK 0x07
#define PCI_CONFIG_REGISTER_OFFSET_MASK 0xfc

#define PCI_CONFIG_ENABLE_BIT_OFFSET 31
#define PCI_CONFIG_BUS_NUMBER_OFFSET 16
#define PCI_CONFIG_DEVICE_NUMBER_OFFSET 11
#define PCI_CONFIG_FUNCTION_NUMBER_OFFSET 8

#define PCI_HEADER_BAR_OFFSET(reg_index) (0x10 + (reg_index * sizeof(uint32_t)))

static uint32_t
pci_config_get_addr(const struct pci_function_address *const address,
                    uint8_t byte_offset)
{
    const uint32_t enable = 1U << PCI_CONFIG_ENABLE_BIT_OFFSET;
    const uint32_t bus = ((uint32_t)address->bus_number)
                         << PCI_CONFIG_BUS_NUMBER_OFFSET;
    const uint32_t device =
        ((uint32_t)(address->device_number & PCI_CONFIG_DEVICE_NUMBER_MASK))
        << PCI_CONFIG_DEVICE_NUMBER_OFFSET;
    const uint32_t function =
        ((uint32_t)(address->function_number & PCI_CONFIG_FUNCTION_NUMBER_MASK))
        << PCI_CONFIG_FUNCTION_NUMBER_OFFSET;
    const uint32_t offset =
        (uint32_t)(byte_offset & PCI_CONFIG_REGISTER_OFFSET_MASK);

    const uint32_t addr = enable | bus | device | function | offset;

    return addr;
}

static uint32_t
pci_config_read_dword(const struct pci_function_address *const address,
                      uint8_t byte_offset)
{
    io_port_out_dword(PCI_IO_CONFIG_ADDRESS,
                      pci_config_get_addr(address, byte_offset));
    return io_port_in_dword(PCI_IO_CONFIG_DATA);
}

static void
pci_config_write_dword(const struct pci_function_address *const address,
                       uint8_t byte_offset, uint32_t value)
{
    io_port_out_dword(PCI_IO_CONFIG_ADDRESS,
                      pci_config_get_addr(address, byte_offset));
    io_port_out_dword(PCI_IO_CONFIG_DATA, value);
}

static uint16_t
pci_config_read_word(const struct pci_function_address *const address,
                     uint8_t byte_offset)
{
    const uint32_t dw = pci_config_read_dword(address, byte_offset);
    const uint8_t offset = byte_offset & 0x02;
    const uint16_t word = (uint16_t)(dw >> (8 * offset));

    return word;
}

static uint8_t
pci_config_read_byte(const struct pci_function_address *const address,
                     uint8_t byte_offset)
{
    const uint32_t dw = pci_config_read_dword(address, byte_offset);
    const uint8_t offset = byte_offset & 0x03;
    const uint8_t byte = (uint8_t)(dw >> (8 * offset));

    return byte;
}

static void
pci_read_header_common(const struct pci_function_address *const address,
                       struct pci_header_common *const header)
{
    header->vendor_id = 0;
    header->device_id = 0;
    header->prog_if = 0;
    header->subclass = 0;
    header->class_code = 0;
    header->header_type = 0;

    header->vendor_id = pci_config_read_word(address, 0x00);
    if (header->vendor_id != 0xFFFF) {
        header->device_id = pci_config_read_word(address, 0x02);
        header->prog_if = pci_config_read_byte(address, 0x09);
        header->subclass = pci_config_read_byte(address, 0x0A);
        header->class_code = pci_config_read_byte(address, 0x0B);
        header->header_type = pci_config_read_byte(address, 0x0E);
    }
}

static bool
pci_is_multifunction_device(const struct pci_header_common *const header)
{
    return ((header->vendor_id != PCI_INVALID_VENDOR_ID) &&
            ((header->header_type & 0x80) != 0x00));
}

static bool
pci_is_function_address_end(const struct pci_function_address *const address)
{
    return (address->bus_number >= PCI_MAX_BUS_COUNT);
}

static void
pci_increment_function_address(struct pci_function_address *const address,
                               bool is_multifunction_device)
{
    if (pci_is_function_address_end(address)) {
        address->bus_number = 0;
        address->device_number = 0;
        address->function_number = 0;
        return;
    }

    ++(address->function_number);
    if ((address->function_number >= PCI_MAX_FUNCTION_COUNT) ||
        !is_multifunction_device) {
        address->function_number = 0;

        ++(address->device_number);
        if (address->device_number >= PCI_MAX_DEVICE_COUNT) {
            address->device_number = 0;

            ++(address->bus_number);
        }
    }
}

void pci_function_iterator_init(struct pci_function_address *const address,
                                struct pci_header_common *const header)
{
    address->bus_number = PCI_MAX_BUS_COUNT;

    header->vendor_id = PCI_INVALID_VENDOR_ID;
}

bool pci_function_iterator_next(struct pci_function_address *const address,
                                struct pci_header_common *const header)
{
    for (;;) {
        pci_increment_function_address(address,
                                       pci_is_multifunction_device(header));
        if (pci_is_function_address_end(address)) {
            // All the PCI configuration space has been iterated through
            header->vendor_id = PCI_INVALID_VENDOR_ID;
            break;
        }
        pci_read_header_common(address, header);
        if (header->vendor_id == PCI_INVALID_VENDOR_ID) {
            continue;
        } else {
            break;
        }
    }

    return (header->vendor_id != PCI_INVALID_VENDOR_ID);
}

uint32_t pci_read_bar_register(struct pci_function_address *const address,
                               uint8_t bar_index)
{
    return pci_config_read_dword(address, PCI_HEADER_BAR_OFFSET(bar_index));
}

void pci_write_bar_register(struct pci_function_address *const address,
                            uint8_t bar_index, uint32_t value)
{
    pci_config_write_dword(address, PCI_HEADER_BAR_OFFSET(bar_index), value);
}
