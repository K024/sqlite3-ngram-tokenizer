
# Ngram tokenizer for sqlite3 fts5

This tokenizer works behind another tokenizer like unicode61 (default). 

Example: 
```js
const sqlite = require('better-sqlite3');
const tokenizer = require('sqlite3-ngram-tokenizer');

const db = sqlite(':memory:');
db.loadExtension(tokenizer.pluginPath); // pluginPath does not contain extension

db.exec(`
CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'ngram');
`);

```

It tokenizes latin words by 2-gram with seperate first letter: `"letter" => [l le et tt te er]`, so it will match `"let"` but not `"etter"`.

For non-latin words, it tokenizes them by 1-gram. It performs like `String.includes()` on non-latin words. 

You can specify other tokenizers like 
```sql
-- porter tokenizer also works behind another tokenizer
CREATE VIRTUAL TABLE t1 USING fts5(x, tokenize = 'ngram porter unicode61 remove_diacritics 1');
```

**NOTICE**

`highlight()` may not work as expected behind a porter tokenizer. 
