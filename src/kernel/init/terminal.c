/* This code is based on the example from:
 *
 * https://wiki.osdev.org/Bare_Bones
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BASE_ADDRESS 0xB8000

// Max. number of decimal digits to represent a number
#define MAX_DIGIT_COUNT 10
#define HEX_BASE 16
#define DEC_BASE 10
#define HEX_PRINTED_WIDTH 8

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

static uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);
static uint16_t vga_entry(unsigned char uc);
static size_t terminal_strlen(const char *str);
static void terminal_putchar_at(char c, size_t x, size_t y);
static void terminal_putchar(char c);

static uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | (bg << 4);
}

static uint16_t vga_entry(unsigned char uc)
{
    return ((uint16_t)uc) | (((uint16_t)terminal_color) << 8);
}

static size_t terminal_strlen(const char *str)
{
    size_t len = 0;

    while ((len < SIZE_MAX) && (str[len] != '\0')) {
        len++;
    }

    return ((str[len] == '\0') ? len : 0);
}

static void terminal_putchar_at(char c, size_t x, size_t y)
{
    const size_t index = (y * VGA_WIDTH) + x;

    terminal_buffer[index] = vga_entry(c);
}

static void terminal_putchar(char c)
{
    if (c != '\n') {
        terminal_putchar_at(c, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_row = 0;
            }
        }
    } else {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            --terminal_row;
            /* Shift all the rows by one up to make space for the new
             * row at the bottom of the display.
             */
            for (size_t y = 0; y < (VGA_HEIGHT - 1); ++y) {
                for (size_t x = 0; x < VGA_WIDTH; ++x) {
                    const size_t i = (y * VGA_WIDTH) + x;
                    terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];
                }
            }
            /* Clear the bottom row. */
            for (size_t x = 0; x < VGA_WIDTH; ++x) {
                terminal_putchar_at(' ', x, terminal_row);
            }
        }
    }
}

static char terminal_num_to_hexchar(uint8_t number)
{
    if (number >= 16) {
        return '?';
    } else if (number < 10) {
        return '0' + number;
    } else {
        return 'a' + number - 10;
    }
}

static size_t terminal_number_to_string(uint32_t number,
                                        char *const return_string, uint8_t base)
{
    size_t i = 0;

    do {
        return_string[i++] = terminal_num_to_hexchar(number % base);
        number /= base;
    } while (number != 0);

    // Terminate the string
    return_string[i] = '\0';

    const size_t digit_count = i;

    // Reverse order of the digits
    size_t j = i - 1;
    i = 0;
    while (i < j) {
        char c;
        c = return_string[i];
        return_string[i] = return_string[j];
        return_string[j] = c;
        ++i;
        --j;
    }

    return digit_count;
}

static void terminal_print_string(const char *string)
{
    const size_t len = terminal_strlen(string);
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(string[i]);
    }
}

static void terminal_print_number_hex(uint32_t number)
{
    char s[MAX_DIGIT_COUNT + 1];

    size_t digit_count = terminal_number_to_string(number, s, HEX_BASE);
    terminal_print_string("0x");
    for (; digit_count < HEX_PRINTED_WIDTH; ++digit_count) {
        terminal_putchar('0');
    }
    terminal_print_string(s);
}

static void terminal_print_number_dec(int32_t number)
{
    char s[MAX_DIGIT_COUNT + 1];
    uint32_t n;
    bool negative = false;

    if (number < 0) {
        negative = true;
        n = -number;
    } else {
        n = number;
    }

    if (negative) {
        terminal_putchar('-');
    }
    terminal_number_to_string(n, s, DEC_BASE);
    terminal_print_string(s);
}

static void terminal_print_with_conversion(char conv_spec,
                                           va_list *const arg_list)
{
    uint32_t unsigned_value;
    int32_t signed_value;
    char *string_value;

    switch (conv_spec) {
    case 'd':
        signed_value = va_arg(*arg_list, int32_t);
        terminal_print_number_dec(signed_value);
        break;

    case 's':
        string_value = va_arg(*arg_list, char *);
        terminal_print_string(string_value);
        break;

    case 'u':
        unsigned_value = va_arg(*arg_list, uint32_t);
        terminal_print_number_dec(unsigned_value);
        break;

    case 'x':
        unsigned_value = va_arg(*arg_list, uint32_t);
        terminal_print_number_hex(unsigned_value);
        break;

    default:
        terminal_putchar('%');
        terminal_putchar(conv_spec);
        break;
    }
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t *)VGA_BASE_ADDRESS;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putchar(' ');
        }
    }
}

void terminal_printf(const char *const format, ...)
{
    const size_t format_length = terminal_strlen(format);
    bool conversion_spec = false;
    va_list ap;

    va_start(ap, format);

    for (size_t i = 0; i < format_length; ++i) {
        const char c = format[i];

        if (conversion_spec) {
            conversion_spec = false;
            terminal_print_with_conversion(c, &ap);
        } else if (c == '%') {
            conversion_spec = true;
        } else {
            terminal_putchar(c);
        }
    }

    va_end(ap);
}
