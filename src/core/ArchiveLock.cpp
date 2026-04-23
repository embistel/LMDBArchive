#include "core/ArchiveLock.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

ArchiveLock::ArchiveLock() = default;

ArchiveLock::~ArchiveLock() {
    unlock();
}

#ifdef _WIN32

bool ArchiveLock::lockForWrite(const QString& filePath) {
    if (locked_) unlock();

    // Open with sharing allowed so LMDB can also open the file
    // We use a lock file (filePath + ".lock") as the synchronization primitive,
    // not the .lmdb file itself, since LMDB needs to mmap it directly.
    QString lockPath = filePath + QStringLiteral(".lock");

    HANDLE h = CreateFileW(
        reinterpret_cast<LPCWSTR>(lockPath.utf16()),
        GENERIC_READ | GENERIC_WRITE,
        0, // exclusive - no sharing of the lock file
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) {
        lastError_ = QStringLiteral("Archive is locked by another process: ") + filePath;
        return false;
    }

    handle_ = h;
    locked_ = true;
    return true;
}

bool ArchiveLock::lockForRead(const QString& filePath) {
    if (locked_) unlock();

    // For read-only, check if a write lock exists (someone holds the .lock file)
    // We don't need to create our own lock file for reads in single-writer model
    QString lockPath = filePath + QStringLiteral(".lock");

    // Try to open the lock file exclusively - if it exists and is locked, fail
    HANDLE h = CreateFileW(
        reinterpret_cast<LPCWSTR>(lockPath.utf16()),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (h != INVALID_HANDLE_VALUE) {
        // Lock file exists - someone might be writing
        // Check if we can get a shared lock
        if (!LockFile(h, 0, 0, 1, 0)) {
            CloseHandle(h);
            lastError_ = QStringLiteral("Archive is being written by another process: ") + filePath;
            return false;
        }
        UnlockFile(h, 0, 0, 1, 0);
        CloseHandle(h);
    }

    locked_ = true;
    return true;
}

void ArchiveLock::unlock() {
    if (handle_) {
        // FILE_FLAG_DELETE_ON_CLOSE ensures lock file is cleaned up
        CloseHandle(handle_);
        handle_ = nullptr;
    }
    locked_ = false;
}

#else // Linux

bool ArchiveLock::lockForWrite(const QString& filePath) {
    if (locked_) unlock();

    QString lockPath = filePath + QStringLiteral(".lock");

    int fd = open(lockPath.toUtf8().constData(), O_RDWR | O_CREAT, 0664);
    if (fd < 0) {
        lastError_ = QStringLiteral("Cannot create lock file: ") + lockPath;
        return false;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        lastError_ = QStringLiteral("Archive is locked by another process: ") + filePath;
        return false;
    }

    fd_ = fd;
    locked_ = true;
    return true;
}

bool ArchiveLock::lockForRead(const QString& filePath) {
    if (locked_) unlock();

    QString lockPath = filePath + QStringLiteral(".lock");

    int fd = open(lockPath.toUtf8().constData(), O_RDONLY);
    if (fd < 0) {
        // No lock file = no writer, safe to read
        locked_ = true;
        return true;
    }

    // Try shared lock - will block if someone has exclusive lock
    if (flock(fd, LOCK_SH | LOCK_NB) != 0) {
        close(fd);
        lastError_ = QStringLiteral("Archive is being written by another process: ") + filePath;
        return false;
    }

    // Release shared lock immediately - we just checked no writer is active
    flock(fd, LOCK_UN);
    close(fd);
    locked_ = true;
    return true;
}

void ArchiveLock::unlock() {
    if (fd_ >= 0) {
        // Remove lock file on unlock
        flock(fd_, LOCK_UN);
        // Get path from fd is complex; lock file gets cleaned up on close on Windows
        // On Linux the lock file persists but is unlocked
        close(fd_);
        fd_ = -1;
    }
    locked_ = false;
}

#endif
