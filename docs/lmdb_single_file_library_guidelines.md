# LMDB Single File Archive - Library Access Guidelines

## File Format

LMDB Archive produces a **single `.lmdb` file** that is a standard LMDB data file opened with `MDB_NOSUBDIR | MDB_NOLOCK`.

This means:
- The `.lmdb` file is directly usable by any standard LMDB library
- No directory structure or sidecar files are required
- The file is portable - copy one file to move the entire archive

## How to Open from External Code

### C Example

```c
#include <lmdb.h>

MDB_env* env;
MDB_txn* txn;
MDB_dbi entries_dbi;

// Create environment
mdb_env_create(&env);
mdb_env_set_maxdbs(env, 16);
mdb_env_set_mapsize(env, 4ULL * 1024 * 1024 * 1024); // 4GB

// Open with MDB_NOSUBDIR - path is the .lmdb file itself
int rc = mdb_env_open(env, "archive.lmdb", MDB_NOSUBDIR | MDB_NOLOCK | MDB_RDONLY, 0664);
if (rc != MDB_SUCCESS) {
    // handle error
}

// Read entries
mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
mdb_dbi_open(txn, "entries", 0, &entries_dbi);

MDB_val key, val;
MDB_cursor* cursor;
mdb_cursor_open(txn, entries_dbi, &cursor);

while (mdb_cursor_get(cursor, &key, &val, MDB_NEXT) == MDB_SUCCESS) {
    // key.mv_data = file path string
    // val.mv_data = file content bytes
    printf("File: %.*s (%zu bytes)\n", (int)key.mv_size, (char*)key.mv_data, val.mv_size);
}

mdb_cursor_close(cursor);
mdb_txn_abort(txn);
mdb_env_close(env);
```

### Python Example (using lmdb package)

```python
import lmdb

# Open the .lmdb file directly
env = lmdb.open("archive.lmdb", max_dbs=16, readonly=True, subdir=False)

with env.begin() as txn:
    entries_db = env.open_db(b"entries", txn=txn)
    cursor = txn.cursor(entries_db)
    for key, value in cursor:
        print(f"File: {key.decode()} ({len(value)} bytes)")
```

## Internal Schema

The `.lmdb` file contains three named databases:

| DBI Name | Key | Value |
|----------|-----|-------|
| `entries` | File path (e.g., `SubFolder/File.jpg`) | Raw file bytes |
| `meta` | File path | JSON metadata (size, dates, hash, etc.) |
| `manifest` | `_meta` | JSON archive-level metadata |

## Lock Policy

The default mode uses `MDB_NOLOCK`:
- **No sidecar lock file** is created
- The `.lmdb` file is fully self-contained
- Application-level locking is managed by LMDB Archive itself

If you need concurrent access from external processes:
1. Ensure only one writer at a time
2. Use OS-level file locking (e.g., `flock` on Linux, `LockFile` on Windows)
3. Or open in read-only mode (`MDB_RDONLY`)

## Concurrency Rules

| Scenario | Safe? | Notes |
|----------|-------|-------|
| Single writer | Yes | Default mode |
| Multiple readers | Yes | Open with `MDB_RDONLY` |
| Reader + Writer simultaneously | Caution | Not safe with `MDB_NOLOCK` |
| Multiple writers | No | Must use external synchronization |

## Portability

- Copy the `.lmdb` file to any system with LMDB support
- Works on both Windows and Linux
- No conversion or repacking needed
- File is byte-order independent (LMDB handles this)
