#include "strings.h"

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *) s;
    while (n--) {
        *p++ = (unsigned char) c;
    }
    return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    // Typecast src and dst addresses 
    unsigned char *csrc = (unsigned char *) src; 
    unsigned char *cdst = (unsigned char *) dst; 
  
    // Copy contents of src[] to dst[] 
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = csrc[i];
    }
    return dst;
}

int strlen(const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) ;
    return i;
}

int strcmp(const char *s1, const char *s2)
{
    int i;
    for (i = 0; ; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] < s2[i] ? -1 : 1;
        }

        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

int strlcat(char *dst, const char *src, int maxsize)
{
    int src_len;
    int dst_len;

    int i = 0;
    int init_dst_len = strlen(dst);
    src_len = strlen(src);
    dst_len = init_dst_len;

    while ((dst_len < maxsize - 1) && src[i] != '\0') {
        dst[dst_len] = src[i];
        dst_len++;
        i++;
    }

    dst[dst_len] = '\0';

    return (src_len + init_dst_len);
}

unsigned int strtonum(const char *str, const char **endptr)
{
    // check if first character in str is valid
    if ((str[0] < '0') || (str[0] > '9')) {
        *endptr = &(str[0]);
        return 0;
    }

    int num = 0;
    int len = strlen(str);

    // treat as hexadecimal if str starts with '0x'
    if (len > 1 && str[0] == '0' && 
        (str[1] == 'x' || str[1] == 'X') ) {
        for (int i = 2; i < len; i++) {
            if ((str[i] >= '0') && (str[i] <= '9')) {
                num = num * 16 + (str[i] - '0');
            } else if ((str[i] >= 'a') && (str[i] <= 'f')) {
                num = num * 16 + (str[i] - 'a') + 10;
            } else if ((str[i] >= 'A') && (str[i] <= 'F')) {
                num = num * 16 + (str[i] - 'A') + 10;
            // return if nonvalid digit
            } else {
                *endptr = str + i;
                return num;
            }
        }

        *endptr = str + len;
        return num;

    // else treat as decimal
    } else {
        for (int i = 0; i < len; i++) {
            if ((str[i] >= '0') && (str[i] <= '9')) {
                num = num * 10 + (str[i] - '0');
            // return if nonvalid digit
            } else {
                *endptr = str + i;
                return num;
            }
        }
        *endptr = str + len;
        return num;
    }   
    return num;
}
