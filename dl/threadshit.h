#ifdef __APPLE__
#include <pthread.h>

typedef struct {
    pthread_cond_t cv;
    pthread_mutex_t mp;
    int b;
} ev_t;
static void ev_init(ev_t *ev) {
    pthread_mutex_init(&ev->mp, NULL);
    pthread_cond_init(&ev->cv, NULL);
    ev->b = 0;
}
static void ev_wait(ev_t *ev) {
    pthread_mutex_lock(&ev->mp);
    while(!ev->b) {
        pthread_cond_wait(&ev->cv, &ev->mp);
    }
    pthread_mutex_unlock(&ev->mp);
}
static void ev_signal(ev_t *ev) {
    ev->b = 1;
    pthread_mutex_lock(&ev->mp);
    pthread_cond_signal(&ev->cv);
    pthread_mutex_unlock(&ev->mp);
}

int threadcount = 0;
ev_t ev_threadcount;
pthread_mutex_t mp_threadcount;

static void *a_thread(void *func) {
    ((void (*)()) func)();
    pthread_mutex_lock(&mp_threadcount);
    threadcount--;
    if(threadcount == 1) { // 1 is the waiting thread
        ev_signal(&ev_threadcount);        
    }
    pthread_mutex_unlock(&mp_threadcount);
    return NULL;
}
static void create_a_thread(void (*func)()) {
    if(threadcount == 0) {
        // not initted yet
        pthread_mutex_init(&mp_threadcount, NULL);
        ev_init(&ev_threadcount);
        threadcount = 0;
    }
    pthread_mutex_lock(&mp_threadcount);
    threadcount++;
    pthread_mutex_unlock(&mp_threadcount);
    pthread_t thread;
    pthread_create(&thread, NULL, a_thread, func);
}
#else
#include <windows.h>

typedef HANDLE ev_t;

static void ev_init(ev_t *ev) {
    *ev = CreateEvent(NULL, TRUE, FALSE, NULL);
}
static void ev_wait(ev_t *ev) {
    WaitForSingleObject(*ev, INFINITE);
}
static void ev_signal(ev_t *ev) {
    SetEvent(*ev);
}

int threadcount = 0;
ev_t ev_threadcount;
CRITICAL_SECTION mp_threadcount;


static DWORD a_thread(void *func) {
    ((void (*)()) func)();
    EnterCriticalSection(&mp_threadcount);
    threadcount--;
    if(threadcount == 1) {
        ev_signal(&ev_threadcount);        
    }
    LeaveCriticalSection(&mp_threadcount);
    return 0;
}

static void create_a_thread(void (*func)()) {
    if(threadcount == 0) {
        // not initted yet
        InitializeCriticalSection(&mp_threadcount);
        ev_init(&ev_threadcount);
        threadcount = 0;
    }
    EnterCriticalSection(&mp_threadcount);
    threadcount++;
    LeaveCriticalSection(&mp_threadcount);
    CreateThread(NULL, 0, a_thread, func, 0, NULL);     
}
#endif
