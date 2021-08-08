#ifndef PCI_H
#define PCI_H

#include <stdbool.h>
#include <stdint.h>

#define PCI_INVALID_VENDOR_ID 0xFFFF

struct pci_function_address {
    uint16_t bus_number;
    uint8_t device_number;
    uint8_t function_number;
};

struct pci_header_common {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t header_type;
};

void pci_function_iterator_init(struct pci_function_address *const address,
                                struct pci_header_common *const header);
bool pci_function_iterator_next(struct pci_function_address *const address,
                                struct pci_header_common *const header);

#endif
