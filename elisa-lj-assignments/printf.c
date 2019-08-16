#include "printf.h"
#include "strings.h"
#include "uart.h"
#include <stdarg.h>

#define MAX_OUTPUT_LEN 1024

char HEX_DIGITS[] = "0123456789abcdef";

int unsigned_to_base(char *buf, int bufsize, unsigned int val, int base, int min_width) 
{

    // collect digits in reverse
    char tmp[MAX_OUTPUT_LEN];
    int i = 0;

    // handle edge case of 0
    if (val == 0) {
        tmp[i] = '0';
        i++;
    }

    while (val > 0) {
        tmp[i] = HEX_DIGITS[val % base];
        val /= base;
        i++;
    }
    tmp[i] = '\0';

    int tmp_len = strlen(tmp);

    // must add extra zeros for padding if string length
    // doesn't meet minimum width
    if (tmp_len < min_width) {
        int k;
        int extra_zeros = min_width - tmp_len;
        for (k = 0; k < extra_zeros; k++) {
            tmp[tmp_len + k] = '0';
        }
        tmp[tmp_len + k] = '\0';
        // update the length of string;
        tmp_len = strlen(tmp);
    }

    // if buffer size is 0 nothing should write
    if (bufsize == 0) {
        return tmp_len;
    }

    // must check if buffer has enough space for string
    // and null terminator
    int rewrite_max = tmp_len;
    if (tmp_len > bufsize - 1) {
        rewrite_max = bufsize - 1;
    }

    int j;
    for (j = 0; j < rewrite_max; j++) {
        buf[j] = tmp[tmp_len - 1 - j];
    }
    buf[j] = '\0';

    // return the number of chars that were supposed
    // to be written
    return tmp_len;
}

int signed_to_base(char *buf, int bufsize, int val, int base, int min_width) 
{
    int minus_written = 0;
    if (val < 0) {
        buf[0] = '-';
        val *= -1;
        minus_written++;
        bufsize--;
        min_width--;
    }
    return unsigned_to_base(buf + minus_written, bufsize, val, base, min_width) + minus_written;
}

int vsnprintf(char *buf, int bufsize, const char *format, va_list args) 
{
    char tmp[MAX_OUTPUT_LEN];
    int i;
    // keep track of offset of additional characters
    // from inputting formats
    int format_offset = 0;
    int format_len = strlen(format);

    // handle empty format string
    if (format_len == 0) {
        return 0;
    }

    for (i = 0; i < format_len; i++) {
        // handle format codes
        if (format[i] == '%') {
            char format_code = format[i + 1];

            int min_width = 0;
            // a minimum width has been specified
            if (format_code == '0') {
                // will point to format code after
                // specified width
                const char *endptr;
                min_width = strtonum(&format[i + 2], &endptr);
                format_code = (char) *endptr;
                int width_of_width = endptr - (format + i);
                i += width_of_width - 1;             
            }
            // handle a signed decimal
            if (format_code == 'd') {
                char num_buf[MAX_OUTPUT_LEN];
                int num = va_arg(args, int);
                int num_len = signed_to_base(num_buf, MAX_OUTPUT_LEN, num, 10, min_width);
                for (int k = 0; k < num_len; k++) {
                    tmp[format_offset + k] = num_buf[k];
                }
                i++;
                format_offset += num_len;
            }
            // handle an unsigned hex
            if (format_code == 'x') {
                char num_buf[MAX_OUTPUT_LEN];
                int num = va_arg(args, int);
                int num_len = unsigned_to_base(num_buf, MAX_OUTPUT_LEN, num, 16, min_width);
                for (int k = 0; k < num_len; k++) {
                    tmp[format_offset + k] = num_buf[k];
                }
                i++;
                format_offset += num_len;
            }
            // handle a pointer
            if (format_code == 'p') {
                char num_buf[MAX_OUTPUT_LEN];
                num_buf[0] = '0';
                num_buf[1] = 'x';
                min_width = 8;
                int num = va_arg(args, int);
                int num_len = unsigned_to_base(&num_buf[2], MAX_OUTPUT_LEN, num, 16, min_width);
                for (int k = 0; k < num_len + 2; k++) {
                    tmp[format_offset + k] = num_buf[k];
                }
                i++;
                // extra 2 for the "0x"
                format_offset += num_len + 2;
            }
            // handle a character
            if (format_code == 'c') {
                char letter = va_arg(args, int);
                tmp[format_offset] = letter;
                i++;
                format_offset++;
            }
            // handle a string
            if (format_code == 's') {
                char *string = va_arg(args, char *);
                tmp[format_offset] = '\0';
                strlcat(tmp + format_offset, string, MAX_OUTPUT_LEN);
                format_offset += strlen(string);
                i++;
            }
        } else {
            tmp[format_offset] = format[i];
            format_offset++;
        }
    }
    tmp[format_offset] = '\0';

    int tmp_len = strlen(tmp);

    // must check if buffer has enough space for string
    // and null terminator
    int rewrite_max = tmp_len;
    if (tmp_len > bufsize - 1) {
        rewrite_max = bufsize - 1;
    }

    int j;
    for (j = 0; j < rewrite_max; j++) {
        buf[j] = tmp[j];
    }
    buf[j] = '\0';
    
    return tmp_len;
}

int snprintf(char *buf, int bufsize, const char *format, ...) 
{
    va_list args;
    va_start(args, format);

    int len = vsnprintf(buf, bufsize, format, args);
    va_end(args);
    return len;
}

int printf(const char *format, ...) 
{
    char buf[MAX_OUTPUT_LEN];

    va_list args;
    va_start(args, format);

    int len = vsnprintf(buf, MAX_OUTPUT_LEN, format, args);
    va_end(args);

    for (int i = 0; i < len; i++) {
        uart_putchar(buf[i]);
    }

    return len;
}
