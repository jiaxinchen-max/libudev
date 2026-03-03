/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "config.h"
#include "libudev.h"
#include "libudev-private.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UTIL_PATH_SIZE 1024

/* Helper functions */
static size_t strscpy(char *dest, size_t size, const char *src) {
        size_t ret = strlen(src);
        if (size) {
                size_t len = (ret >= size) ? size - 1 : ret;
                memcpy(dest, src, len);
                dest[len] = '\0';
        }
        return ret;
}

static size_t strscpyl(char *dest, size_t size, ...) {
        va_list ap;
        char *src;
        size_t l, len = 0;

        va_start(ap, size);
        while ((src = va_arg(ap, char *))) {
                l = strlen(src);
                if (len + l >= size) {
                        if (size > 1)
                                dest[size-1] = '\0';
                        break;
                }
                memcpy(dest + len, src, l);
                len += l;
        }
        va_end(ap);
        
        if (len < size)
                dest[len] = '\0';
        return len;
}

static bool whitelisted_char_for_devnode(char c, const char *white) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            strchr("#+-.:=@_", c) != NULL)
                return true;
        if (white != NULL && strchr(white, c) != NULL)
                return true;
        return false;
}

static int utf8_encoded_valid_unichar(const char *str) {
        unsigned char c = (unsigned char)*str;
        
        if (c < 0x80)
                return 1;
        if ((c & 0xe0) == 0xc0)
                return 2;
        if ((c & 0xf0) == 0xe0)
                return 3;
        if ((c & 0xf8) == 0xf0)
                return 4;
        return 1;
}

/**
 * SECTION:libudev-util
 * @short_description: utils
 *
 * Utilities useful when dealing with devices and device node names.
 */

/* handle "[<SUBSYSTEM>/<KERNEL>]<attribute>" format */
int util_resolve_sys_link(struct udev *udev, char *result, size_t size, const char *syspath, const char *slink) {
        char path[UTIL_PATH_SIZE];
        char target[UTIL_PATH_SIZE];
        ssize_t len;
        char *pos;
        char *tail;
        int i;
        int back;

        strscpy(result, size, syspath);
        strscpyl(path, sizeof(path), syspath, "/", slink, NULL);
        len = readlink(path, target, sizeof(target));
        if (len <= 0)
                return -1;
        target[len] = '\0';

        pos = target;
        tail = result + strlen(result);
        for (i = 0; i <= back; i++) {
                pos = strrchr(target, '/');
                if (pos == NULL)
                        return -1;
                pos[0] = '\0';
                tail = strrchr(result, '/');
                if (tail == NULL)
                        return -1;
                tail[0] = '\0';
        }
        strscpyl(tail, result + size - tail, "/", pos+1, NULL);
        return 0;
}

size_t util_path_encode(const char *src, char *dest, size_t size) {
        size_t i, j;

        for (i = 0, j = 0; src[i] != '\0'; i++) {
                if (src[i] == '/') {
                        if (j+4 >= size) {
                                j = 0;
                                break;
                        }
                        memcpy(&dest[j], "\\x2f", 4);
                        j += 4;
                } else if (src[i] == '\\') {
                        if (j+4 >= size) {
                                j = 0;
                                break;
                        }
                        memcpy(&dest[j], "\\x5c", 4);
                        j += 4;
                } else {
                        if (j+1 >= size) {
                                j = 0;
                                break;
                        }
                        dest[j] = src[i];
                        j++;
                }
        }
        dest[j] = '\0';
        return j;
}

void util_remove_trailing_chars(char *path, char c) {
        size_t len;

        if (path == NULL)
                return;
        len = strlen(path);
        while (len > 0 && path[len-1] == c)
                path[--len] = '\0';
}

int util_replace_whitespace(const char *str, char *to, size_t len) {
        size_t i, j;

        /* strip trailing whitespace */
        len = strlen(str);
        while (len && isspace(str[len-1]))
                len--;

        /* strip leading whitespace */
        i = 0;
        while (isspace(str[i]) && (i < len))
                i++;

        j = 0;
        while (i < len) {
                /* substitute multiple whitespace with a single '_' */
                if (isspace(str[i])) {
                        while (isspace(str[i]))
                                i++;
                        to[j++] = '_';
                }
                to[j++] = str[i++];
        }
        to[j] = '\0';
        return 0;
}

/* allow chars in whitelist, plain ascii, hex-escaping everything else */
int util_replace_chars(char *str, const char *white) {
        size_t i = 0;
        int replaced = 0;

        while (str[i] != '\0') {
                int len;

                if (whitelisted_char_for_devnode(str[i], white)) {
                        i++;
                        continue;
                }

                /* accept hex encoding */
                if (str[i] == '\\' && str[i+1] == 'x') {
                        i += 2;
                        continue;
                }

                /* keep valid utf8 */
                len = utf8_encoded_valid_unichar(&str[i]);
                if (len > 1) {
                        i += len;
                        continue;
                }

                /* invalid chars are replaced with '_' */
                str[i] = '_';
                i++;
                replaced++;
        }
        return replaced;
}

/**
 * util_encode_string:
 * @str: input string to be encoded
 * @str_enc: output string to store the encoded input string
 * @len: maximum size of the output string, which may be
 *       four times as long as the input string
 *
 * Encode all potentially unsafe characters of a string to the
 * corresponding 2 char hex value prefixed by '\x'.
 *
 * Returns: 0 if the entire string was copied, non-zero otherwise.
 **/
int util_encode_string(const char *str, char *str_enc, size_t len) {
        size_t i, j;

        if (str == NULL || str_enc == NULL)
                return -1;

        for (i = 0, j = 0; str[i] != '\0'; i++) {
                int seqlen;

                seqlen = utf8_encoded_valid_unichar(&str[i]);
                if (seqlen > 1) {
                        if (j + seqlen >= len)
                                goto err;
                        memcpy(&str_enc[j], &str[i], seqlen);
                        j += seqlen;
                        i += (seqlen-1);
                } else if (str[i] == '\\' || !whitelisted_char_for_devnode(str[i], NULL)) {
                        if (j + 4 >= len)
                                goto err;
                        sprintf(&str_enc[j], "\\x%02x", (unsigned char) str[i]);
                        j += 4;
                } else {
                        if (j + 1 >= len)
                                goto err;
                        str_enc[j] = str[i];
                        j++;
                }
        }
        if (j >= len)
                goto err;
        str_enc[j] = '\0';
        return 0;
err:
        return -1;
}

unsigned int util_string_hash32(const char *str) {
        unsigned int hash = 0;

        /* jenkins one at a time hash */
        while (*str != '\0') {
                hash += (unsigned char) *str;
                hash += (hash << 10);
                hash ^= (hash >> 6);
                str++;
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
}

/* get a bunch of bit numbers out of the hash, and set the bits in our bit field */
uint64_t util_string_bloom64(const char *str) {
        uint64_t bits = 0;
        unsigned int hash = util_string_hash32(str);

        bits |= 1LLU << (hash & 63);
        bits |= 1LLU << ((hash >> 6) & 63);
        bits |= 1LLU << ((hash >> 12) & 63);
        bits |= 1LLU << ((hash >> 18) & 63);
        return bits;
}