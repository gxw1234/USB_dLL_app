#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#ifndef _WIN32
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic Windows type aliases
typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef void* HANDLE;
typedef void* LPVOID;
typedef unsigned long DWORD;
#ifndef WINAPI
#define WINAPI
#endif

// Critical section -> pthread mutex
typedef pthread_mutex_t CRITICAL_SECTION;

static inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_init(cs, NULL);
}
static inline void DeleteCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_destroy(cs);
}
static inline void EnterCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_lock(cs);
}
static inline void LeaveCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_unlock(cs);
}

// Sleep milliseconds
static inline void Sleep(unsigned int ms) {
    usleep(ms * 1000);
}

// Thread wrapper

typedef struct {
    pthread_t thread;
} THREAD_WRAPPER;

static inline HANDLE CreateThread(void* lpThreadAttributes, size_t dwStackSize,
                                  DWORD (*lpStartAddress)(LPVOID),
                                  LPVOID lpParameter, DWORD dwCreationFlags,
                                  DWORD* lpThreadId) {
    (void)lpThreadAttributes; (void)dwStackSize; (void)dwCreationFlags;
    (void)lpThreadId;
    THREAD_WRAPPER* h = (THREAD_WRAPPER*)malloc(sizeof(THREAD_WRAPPER));
    if (!h) return NULL;
    int rc = pthread_create(&h->thread, NULL, (void*(*)(void*))lpStartAddress, lpParameter);
    if (rc != 0) { free(h); return NULL; }
    return (HANDLE)h;
}

#define INFINITE 0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258

static inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    THREAD_WRAPPER* h = (THREAD_WRAPPER*)hHandle;
#if defined(_GNU_SOURCE)
    if (dwMilliseconds != INFINITE) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += dwMilliseconds / 1000;
        ts.tv_nsec += (dwMilliseconds % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
        int rc = pthread_timedjoin_np(h->thread, NULL, &ts);
        return rc == 0 ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
    }
#endif
    pthread_join(h->thread, NULL);
    return WAIT_OBJECT_0;
}

static inline BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode) {
    (void)dwExitCode;
#if defined(__linux__)
    THREAD_WRAPPER* h = (THREAD_WRAPPER*)hThread;
    pthread_cancel(h->thread);
    return TRUE;
#else
    return FALSE;
#endif
}

static inline void CloseHandle(HANDLE hObject) {
    THREAD_WRAPPER* h = (THREAD_WRAPPER*)hObject;
    if (h) { free(h); }
}

#ifdef __cplusplus
}
#endif

#endif // !_WIN32

#endif // PLATFORM_COMPAT_H
