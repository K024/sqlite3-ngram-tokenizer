/* utf8_decode.h */

#define UTF8_END   -1
#define UTF8_ERROR -2

typedef struct utf8_decode_ctx utf8_decode_ctx;
struct utf8_decode_ctx {
    int the_index;
    int the_length;
    int the_char;
    int the_byte;
    const char *the_input;
};

extern int  utf8_decode_at_byte(utf8_decode_ctx *ctx);
extern int  utf8_decode_index(utf8_decode_ctx *ctx);
extern int  utf8_decode_at_character(utf8_decode_ctx *ctx);
extern void utf8_decode_init(utf8_decode_ctx *ctx, const char p[], int length);
extern int  utf8_decode_next(utf8_decode_ctx *ctx);
