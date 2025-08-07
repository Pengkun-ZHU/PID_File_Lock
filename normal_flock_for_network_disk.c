#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// Demonstrates acquiring a write lock on "my.log" using fcntl and struct flock.
int main(void) {
    // Open the log file for reading and writing; create if it doesn't exist.
    int fd = open("my.log", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open file");
        return 1;
    }

    // Set up the flock structure for an exclusive (write) lock.
    struct flock lock_info;
    lock_info.l_type   = F_WRLCK;   // Request a write lock (exclusive).
    lock_info.l_whence = SEEK_SET;  // Relative to the beginning of the file.
    lock_info.l_start  = 0;         // Start of the file.
    lock_info.l_len    = 0;         // Lock the entire file.

    // Attempt to acquire the lock non-blockingly.
    if (fcntl(fd, F_SETLK, &lock_info) == -1) {
        perror("Another process holds the lock. Cannot acquire.");
        close(fd);
        return 1;
    }

    fprintf(stderr, "Lock acquired successfully.\n");

    // ... (do work while holding the lock) ...

    // Unlock and clean up.
    lock_info.l_type = F_UNLCK;     // Release the lock.
    fcntl(fd, F_SETLK, &lock_info);

    close(fd);
    return 0;
}
