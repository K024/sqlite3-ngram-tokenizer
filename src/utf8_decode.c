/* utf8_decode.c */

/* 2016-04-05 */

/*
Copyright (c) 2005 JSON.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "utf8_decode.h"

/*
    Very Strict UTF-8 Decoder

    UTF-8 is a multibyte character encoding of Unicode. A character can be
    represented by 1-4 bytes. The bit pattern of the first byte indicates the
    number of continuation bytes.

    Most UTF-8 decoders tend to be lenient, attempting to recover as much
    information as possible, even from badly encoded input. This UTF-8
    decoder is not lenient. It will reject input which does not include
    proper continuation bytes. It will reject aliases (or suboptimal
    codings). It will reject surrogates. (Surrogate encoding should only be
    used with UTF-16.)

    Code     Contination Minimum Maximum
    0xxxxxxx           0       0     127
    10xxxxxx       error
    110xxxxx           1     128    2047
    1110xxxx           2    2048   65535 excluding 55296 - 57343
    11110xxx           3   65536 1114111
    11111xxx       error
*/


/*
    Get the next byte. It returns UTF8_END if there are no more bytes.
*/
static int get(utf8_decode_ctx *ctx) {
    int c;
    if (ctx->the_index >= ctx->the_length) {
        return UTF8_END;
    }
    c = ctx->the_input[ctx->the_index] & 0xFF;
    ctx->the_index += 1;
    return c;
}


/*
    Get the 6-bit payload of the next continuation byte.
    Return UTF8_ERROR if it is not a contination byte.
*/
static int cont(utf8_decode_ctx *ctx) {
    int c = get(ctx);
    return ((c & 0xC0) == 0x80)
        ? (c & 0x3F)
        : UTF8_ERROR;
}


/*
    Initialize the UTF-8 decoder. The decoder is not reentrant,
*/
void utf8_decode_init(utf8_decode_ctx *ctx, const char p[], int length) {
    ctx->the_index = 0;
    ctx->the_input = p;
    ctx->the_length = length;
    ctx->the_char = 0;
    ctx->the_byte = 0;
}


/*
    Get the current byte offset. This is generally used in error reporting.
*/
int utf8_decode_at_byte(utf8_decode_ctx *ctx) {
    return ctx->the_byte;
}


int utf8_decode_index(utf8_decode_ctx *ctx) {
    return ctx->the_index;
}

/*
    Get the current character offset. This is generally used in error reporting.
    The character offset matches the byte offset if the text is strictly ASCII.
*/
int utf8_decode_at_character(utf8_decode_ctx *ctx) {
    return (ctx->the_char > 0)
        ? ctx->the_char - 1
        : 0;
}


/*
    Extract the next character.
    Returns: the character (between 0 and 1114111)
         or  UTF8_END   (the end)
         or  UTF8_ERROR (error)
*/
int utf8_decode_next(utf8_decode_ctx *ctx) {
    int c;  /* the first byte of the character */
    int c1; /* the first continuation character */
    int c2; /* the second continuation character */
    int c3; /* the third continuation character */
    int r;  /* the result */

    if (ctx->the_index >= ctx->the_length) {
        return ctx->the_index == ctx->the_length ? UTF8_END : UTF8_ERROR;
    }
    ctx->the_byte = ctx->the_index;
    ctx->the_char += 1;
    c = get(ctx);
/*
    Zero continuation (0 to 127)
*/
    if ((c & 0x80) == 0) {
        return c;
    }
/*
    One continuation (128 to 2047)
*/
    if ((c & 0xE0) == 0xC0) {
        c1 = cont(ctx);
        if (c1 >= 0) {
            r = ((c & 0x1F) << 6) | c1;
            if (r >= 128) {
                return r;
            }
        }

/*
    Two continuations (2048 to 55295 and 57344 to 65535)
*/
    } else if ((c & 0xF0) == 0xE0) {
        c1 = cont(ctx);
        c2 = cont(ctx);
        if ((c1 | c2) >= 0) {
            r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
            if (r >= 2048 && (r < 55296 || r > 57343)) {
                return r;
            }
        }

/*
    Three continuations (65536 to 1114111)
*/
    } else if ((c & 0xF8) == 0xF0) {
        c1 = cont(ctx);
        c2 = cont(ctx);
        c3 = cont(ctx);
        if ((c1 | c2 | c3) >= 0) {
            r = ((c & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
            if (r >= 65536 && r <= 1114111) {
                return r;
            }
        }
    }
    return UTF8_ERROR;
}