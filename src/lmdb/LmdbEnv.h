#pragma once
#include <QString>
#include <lmdb.h>
#include <memory>

class LmdbEnv {
public:
    LmdbEnv();
    ~LmdbEnv();

    // Open LMDB environment at given path
    bool open(const QString& path, bool readOnly = false,
              size_t mapSize = 4ULL * 1024 * 1024 * 1024, // 4GB default
              int maxDbs = 16);

    void close();

    MDB_env* env() const { return env_; }
    bool isOpen() const { return env_ != nullptr; }

    // Open a named database, returns dbi handle
    bool openDbi(const char* name, MDB_dbi& dbi, bool create = true);

    // Transaction helpers
    MDB_txn* beginTxn(bool readOnly = false);
    int commitTxn(MDB_txn* txn);
    void abortTxn(MDB_txn* txn);

    QString lastError() const { return lastError_; }

    // Resize map size (requires close + reopen)
    bool resize(size_t newMapSize);

private:
    MDB_env* env_ = nullptr;
    size_t mapSize_ = 0;
    QString path_;
    QString lastError_;
    bool readOnly_ = false;
};
