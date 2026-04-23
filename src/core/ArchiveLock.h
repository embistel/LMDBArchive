#pragma once
#include <QString>

class ArchiveLock {
public:
    ArchiveLock();
    ~ArchiveLock();

    // Try to acquire exclusive write lock on the file
    // Returns true if lock acquired successfully
    bool lockForWrite(const QString& filePath);

    // Try to acquire shared read lock
    bool lockForRead(const QString& filePath);

    // Release the lock
    void unlock();

    bool isLocked() const { return locked_; }
    QString lastError() const { return lastError_; }

private:
    bool locked_ = false;
    QString lastError_;

#ifdef _WIN32
    void* handle_ = nullptr; // HANDLE
#else
    int fd_ = -1;
#endif
};
