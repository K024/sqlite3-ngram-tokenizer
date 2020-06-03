#include <stdio.h>
#include <string.h>
/* Add your header comment here */
#include <sqlite3ext.h> /* Do not use <sqlite3.h>! */
SQLITE_EXTENSION_INIT1

/* Insert your extension code here */

typedef int
xTokenCb(void *pCtx,         /* Copy of 2nd argument to xTokenize() */
         int tflags,         /* Mask of FTS5_TOKEN_* flags */
         const char *pToken, /* Pointer to buffer containing token */
         int nToken,         /* Size of token in bytes */
         int iStart,         /* Byte offset of token within input text */
         int iEnd            /* Byte offset of end of token within input text */
);

fts5_api *fts5_api_from_db(sqlite3 *db);

int ngramCreate(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut);
void ngramDelete(Fts5Tokenizer *);
int ngramTokenize(Fts5Tokenizer *, void *pCtx,
                  int flags, /* Mask of FTS5_TOKENIZE_* flags */
                  const char *pText, int nText, xTokenCb *xToken);
xTokenCb NgramCb;

fts5_tokenizer ngram_tokenizer = {ngramCreate, ngramDelete, ngramTokenize};

/*
** Place init function right after this
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
    /* TODO: Change the entry point name so that "extension" is replaced by
    ** text derived from the shared library filename as follows:  Copy every
    ** ASCII alphabetic character from the filename after the last "/" through
    ** the next following ".", converting each character to lowercase, and
    ** discarding the first three characters if they are "lib".
    */
    int sqlite3_ngramtokenizer_init(sqlite3 *db, char **pzErrMsg,
                                    const sqlite3_api_routines *pApi) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    /* Insert here calls to
    **     sqlite3_create_function_v2(),
    **     sqlite3_create_collation_v2(),
    **     sqlite3_create_module_v2(), and/or
    **     sqlite3_vfs_register()
    ** to register the new features that your extension adds.
    */

    fts5_api *api = fts5_api_from_db(db);
    if (api == 0) {
        rc = SQLITE_ERROR;
    } else {
        rc = api->xCreateTokenizer(api, "ngram", api, &ngram_tokenizer, 0);
    }

    return rc;
}

/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
fts5_api *fts5_api_from_db(sqlite3 *db) {
    fts5_api *pRet = 0;
    sqlite3_stmt *pStmt = 0;

    if (SQLITE_OK == sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0)) {
        sqlite3_bind_pointer(pStmt, 1, (void *)&pRet, "fts5_api_ptr", 0);
        sqlite3_step(pStmt);
    }
    sqlite3_finalize(pStmt);
    return pRet;
}

typedef struct NgramTokenizer NgramTokenizer;
struct NgramTokenizer {
    fts5_tokenizer tokenizer;  /* Parent tokenizer module */
    Fts5Tokenizer *pTokenizer; /* Parent tokenizer instance */
};

typedef struct NgramContext NgramContext;
struct NgramContext {
    void *pCtx;
    int (*xToken)(void *, int, const char *, int, int, int);
};

int ngramCreate(void *pCtx, const char **azArg, int nArg,
                Fts5Tokenizer **ppOut) {
    fts5_api *pApi = (fts5_api *)pCtx;
    int rc = SQLITE_OK;
    NgramTokenizer *pRet;
    void *pUserdata = 0;
    const char *zBase = "unicode61";

    if (nArg > 0) {
        zBase = azArg[0];
    }

    pRet = (NgramTokenizer *)sqlite3_malloc(sizeof(NgramTokenizer));
    if (pRet) {
        memset(pRet, 0, sizeof(NgramTokenizer));
        rc = pApi->xFindTokenizer(pApi, zBase, &pUserdata, &pRet->tokenizer);
    } else {
        rc = SQLITE_NOMEM;
    }
    if (rc == SQLITE_OK) {
        int nArg2 = (nArg > 0 ? nArg - 1 : 0);
        const char **azArg2 = (nArg2 ? &azArg[1] : 0);
        rc = pRet->tokenizer.xCreate(pUserdata, azArg2, nArg2,
                                     &pRet->pTokenizer);
    }

    if (rc != SQLITE_OK) {
        ngramDelete((Fts5Tokenizer *)pRet);
        pRet = 0;
    }
    *ppOut = (Fts5Tokenizer *)pRet;
    return rc;
}

void ngramDelete(Fts5Tokenizer *pTok) {
    if (pTok) {
        NgramTokenizer *p = (NgramTokenizer *)pTok;
        if (p->pTokenizer) {
            p->tokenizer.xDelete(p->pTokenizer);
        }
        sqlite3_free(p);
    }
}

int ngramTokenize(Fts5Tokenizer *pTok, void *pCtx,
                  int flags, /* Mask of FTS5_TOKENIZE_* flags */
                  const char *pText, int nText, xTokenCb *xToken) {

    NgramTokenizer *p = (NgramTokenizer *)pTok;
    NgramContext sCtx;
    sCtx.xToken = xToken;
    sCtx.pCtx = pCtx;
    return p->tokenizer.xTokenize(p->pTokenizer, (void *)&sCtx, flags, pText,
                                  nText, NgramCb);
}

static const unsigned char sqlite3Utf8Trans1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};

#define READ_UTF8(zIn, c)                                                      \
    c = *(zIn++);                                                              \
    if (c >= 0xc0) {                                                           \
        c = sqlite3Utf8Trans1[c - 0xc0];                                       \
        while (zIn != 0 && (*zIn & 0xc0) == 0x80) {                            \
            c = (c << 6) + (0x3f & *(zIn++));                                  \
        }                                                                      \
        if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 ||                          \
            (c & 0xFFFFFFFE) == 0xFFFE) {                                      \
            c = 0xFFFD;                                                        \
        }                                                                      \
    }

int NgramCb(void *pCtx, int tflags, const char *pToken, int nToken, int iStart,
            int iEnd) {
    NgramContext *p = (NgramContext *)pCtx;
    int rc = SQLITE_OK, c, size, offset;
    const unsigned char *ps = pToken, *pe = pToken, *end = pToken + nToken;

    READ_UTF8(pe, c);
    pe = pToken;
    if (c >= 0x2000) { /* 1-gram */
        while (pe < end) {
            READ_UTF8(pe, c);
            if (pe > end) {
                rc = SQLITE_ERROR;
                fprintf(stderr, "1-gram: READ_UTF8 overflows\n");
                fprintf(stderr, "token: %.*s\n", nToken, pToken);
                break;
            }
            size = pe - ps;
            offset = ps - pToken;
            p->xToken(p->pCtx, tflags, ps, size, iStart + offset,
                      iStart + offset + size);

            ps = pe;
        }
    } else { /* 2-gram */
        // read one char first
        READ_UTF8(pe, c);
        if (pe > end) {
            rc = SQLITE_ERROR;
            fprintf(stderr, "2-gram: first READ_UTF8 overflows\n");
            fprintf(stderr, "token: %.*s\n", nToken, pToken);
            return rc;
        }
        size = pe - ps;
        offset = ps - pToken;
        p->xToken(p->pCtx, tflags, ps, size, iStart + offset,
                  iStart + offset + size);

        const char *ps2 = ps;
        ps = pe;
        while (pe < end) {
            READ_UTF8(pe, c);
            if (pe > end) {
                rc = SQLITE_ERROR;
                fprintf(stderr, "2-gram: READ_UTF8 overflows\n");
                fprintf(stderr, "token: %.*s\n", nToken, pToken);
                break;
            }
            size = pe - ps2;
            offset = ps2 - pToken;
            p->xToken(p->pCtx, tflags, ps2, size, iStart + offset,
                      iStart + offset + size);

            ps2 = ps;
            ps = pe;
        }
    }

    return rc;
}