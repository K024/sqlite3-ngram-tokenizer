#include "utf8_decode.h"
#include <string.h>

#include <sqlite3ext.h> /* Do not use <sqlite3.h>! */
SQLITE_EXTENSION_INIT1

typedef int
xTokenCb(void *pCtx,         /* Copy of 2nd argument to xTokenize() */
         int tflags,         /* Mask of FTS5_TOKEN_* flags */
         const char *pToken, /* Pointer to buffer containing token */
         int nToken,         /* Size of token in bytes */
         int iStart,         /* Byte offset of token within input text */
         int iEnd            /* Byte offset of end of token within input text */
);

int ngramCreate(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut);
void ngramDelete(Fts5Tokenizer *);
int ngramTokenize(Fts5Tokenizer *, void *pCtx,
                  int flags, /* Mask of FTS5_TOKENIZE_* flags */
                  const char *pText, int nText, xTokenCb *xToken);

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

fts5_tokenizer ngram_tokenizer = {ngramCreate, ngramDelete, ngramTokenize};

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

#ifdef _WIN32
__declspec(dllexport)
#endif
    int sqlite3_ngramtokenizer_init(sqlite3 *db, char **pzErrMsg,
                                    const sqlite3_api_routines *pApi) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);

    fts5_api *api = fts5_api_from_db(db);
    if (api == 0) {
        rc = SQLITE_ERROR;
    } else {
        rc = api->xCreateTokenizer(api, "ngram", api, &ngram_tokenizer, 0);
    }

    return rc;
}

int NgramCb(void *pCtx, int tflags, const char *pToken, int nToken, int iStart,
            int iEnd) {
    NgramContext *p = (NgramContext *)pCtx;
    int c, cl, start = 0, mid = 0, end;

    utf8_decode_init(pToken, nToken);

    c = utf8_decode_next();
    end = utf8_decode_index();
    if (c == UTF8_END)
        return SQLITE_OK;
    if (c == UTF8_ERROR)
        return SQLITE_ERROR;
    p->xToken(p->pCtx, tflags, pToken + start, end - start, iStart + start,
              iStart + end);

    for (;;) {
        start = mid;
        mid = end;
        cl = c;
        c = utf8_decode_next();
        end = utf8_decode_index();
        if (c == UTF8_END)
            return SQLITE_OK;
        if (c == UTF8_ERROR)
            return SQLITE_ERROR;
        if ((unsigned int)c < 0x2000u &&
            (unsigned int)cl < 0x2000u) { // 2-gram for latin

            p->xToken(p->pCtx, tflags, pToken + start, end - start,
                      iStart + start, iStart + end);
        } else { // 1-gram for others

            p->xToken(p->pCtx, tflags, pToken + mid, end - mid,
                      iStart + mid, iStart + end);
        }
    }

    return SQLITE_ERROR;
}

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
