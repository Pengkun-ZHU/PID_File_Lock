#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

// Attempts to acquire a named lock resource (e.g., "/tmp/mylog.lock").
// Returns:
//   0  - Success, resource locked by this process.
//   1  - Resource is locked by another process (pid returned via *pid).
//  <0  - Other errors, with error code set in *err_code.
int acquire_resource(const char *lock_path, int *err_code, pid_t *pid) {
    char current_pid_str[32];
    char proc_path[6 + 32] = "/proc/";
    struct stat stat_buf;

    *pid = 0;
    snprintf(current_pid_str, sizeof(current_pid_str), "%ld", (long)getpid());

    for (int attempt = 0; attempt < 10; ++attempt) {
        // Try to create a symlink for the lock.
        if (symlink(lock_path, current_pid_str) == 0) {
            // Lock acquired.
            return 0;
        }
        if (errno != EEXIST) {
            // Failed for some reason other than the link existing.
            *err_code = errno;
            return -1;
        }

        // Read the link to find out who holds the lock.
        ssize_t link_len = readlink(lock_path, proc_path + 6, sizeof(proc_path) - 6 - 1);
        if (link_len < 0) {
            if (errno == ENOENT)
                continue;  // Lock file disappeared, try again.
            *err_code = errno;
            return -2;
        }
        proc_path[6 + link_len] = '\0';

        // Extract the PID of the process holding the lock.
        *pid = (pid_t)strtol(proc_path + 6, NULL, 10);

        // Check if the process is still running.
        if (stat(proc_path, &stat_buf) == 0) {
            // Process exists, lock is held by another process.
            return 1;
        }

        // Process is dead, try to remove the stale lock.
        if (unlink(lock_path) != 0 && errno != ENOENT) {
            *err_code = errno;
            return -3;
        }
    }
    // Too many failed attempts to acquire the lock.
    return -4;
}

// Releases the named lock resource.
// Returns 0 on success, -1 on failure.
int release_resource(const char *lock_path) {
    return unlink(lock_path);
}
