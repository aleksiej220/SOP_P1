#ifndef WATCHER_H
#define WATCHER_H

#include <sys/inotify.h>
#include <stdint.h>
#include "copying.h"
#include "controlPanel.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize file-system watcher; returns a shared control panel
controlPanel* initialize_watch(copy_info info);

// Low-level helpers (exposed for testing or extensions)
int add_watch(int fd, const char *path, uint32_t mask);
void add_watch_recursive(int fd, const char *dirpath, uint32_t mask);

// Event handling function
void handle_event(struct inotify_event* ev, const copy_info* info, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif // WATCHER_H
