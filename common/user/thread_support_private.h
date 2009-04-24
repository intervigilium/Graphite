#ifndef THREAD_SUPPORT_PRIVATE_H
#define THREAD_SUPPORT_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

void CarbonGetThreadToSpawn(ThreadSpawnRequest *req);
// void CarbonThreadStart(ThreadSpawnRequest *req);
// void CarbonThreadExit();
void *CarbonSpawnManagedThread(void *p);
void *CarbonThreadSpawner(void *p);
int CarbonSpawnThreadSpawner();
void CarbonGetThreadSpawnReq (ThreadSpawnRequest *req);

int CarbonPthreadCreate(pthread_t *tid, int *attr, thread_func_t func, void *arg);
int CarbonPthreadJoin(pthread_t tid, void **pparg);

#ifdef __cplusplus
}
#endif

#endif
