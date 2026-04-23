#include "lmdb/LmdbEnv.h"
#include <QDir>
#include <spdlog/spdlog.h>

LmdbEnv::LmdbEnv() = default;

LmdbEnv::~LmdbEnv() {
    close();
}

bool LmdbEnv::open(const QString& path, bool readOnly, size_t mapSize, int maxDbs) {
    if (env_) close();

    QDir dir(path);
    if (!readOnly) {
        if (!dir.mkpath(QLatin1String("."))) {
            lastError_ = QStringLiteral("Cannot create directory: ") + path;
            return false;
        }
    }

    int rc = mdb_env_create(&env_);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_env_create failed: ") + QString::fromLatin1(mdb_strerror(rc));
        env_ = nullptr;
        return false;
    }

    rc = mdb_env_set_mapsize(env_, mapSize);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_env_set_mapsize failed: ") + QString::fromLatin1(mdb_strerror(rc));
        mdb_env_close(env_);
        env_ = nullptr;
        return false;
    }

    rc = mdb_env_set_maxdbs(env_, maxDbs);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_env_set_maxdbs failed: ") + QString::fromLatin1(mdb_strerror(rc));
        mdb_env_close(env_);
        env_ = nullptr;
        return false;
    }

    unsigned int flags = readOnly ? MDB_RDONLY : 0;
    rc = mdb_env_open(env_, path.toUtf8().constData(), flags, 0664);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_env_open failed: ") + QString::fromLatin1(mdb_strerror(rc));
        mdb_env_close(env_);
        env_ = nullptr;
        return false;
    }

    path_ = path;
    mapSize_ = mapSize;
    readOnly_ = readOnly;
    spdlog::info("LMDB environment opened: {} (readonly={})", path.toStdString(), readOnly);
    return true;
}

void LmdbEnv::close() {
    if (env_) {
        mdb_env_close(env_);
        env_ = nullptr;
        spdlog::info("LMDB environment closed: {}", path_.toStdString());
    }
}

bool LmdbEnv::openDbi(const char* name, MDB_dbi& dbi, bool create) {
    if (!env_) {
        lastError_ = QStringLiteral("Environment not open");
        return false;
    }

    MDB_txn* txn = beginTxn(false);
    if (!txn) return false;

    unsigned int flags = create ? MDB_CREATE : 0;
    int rc = mdb_dbi_open(txn, name, flags, &dbi);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_dbi_open failed for '%1': %2")
            .arg(QString::fromLatin1(name), QString::fromLatin1(mdb_strerror(rc)));
        abortTxn(txn);
        return false;
    }

    commitTxn(txn);
    return true;
}

MDB_txn* LmdbEnv::beginTxn(bool readOnly) {
    if (!env_) return nullptr;
    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(env_, nullptr, readOnly ? MDB_RDONLY : 0, &txn);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_txn_begin failed: ") + QString::fromLatin1(mdb_strerror(rc));
        return nullptr;
    }
    return txn;
}

int LmdbEnv::commitTxn(MDB_txn* txn) {
    if (!txn) return EINVAL;
    int rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        lastError_ = QStringLiteral("mdb_txn_commit failed: ") + QString::fromLatin1(mdb_strerror(rc));
    }
    return rc;
}

void LmdbEnv::abortTxn(MDB_txn* txn) {
    if (txn) mdb_txn_abort(txn);
}

bool LmdbEnv::resize(size_t newMapSize) {
    QString savedPath = path_;
    bool wasReadOnly = readOnly_;
    close();
    return open(savedPath, wasReadOnly, newMapSize);
}
