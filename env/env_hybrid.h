#pragma once

#include <pthread.h>

#include <vector>

#include "composite_env_wrapper.h"
#include "util/threadpool_imp.h"

namespace ROCKSDB_NAMESPACE {
class HybridFSEnv : public CompositeEnv {
 public:
  static const char* kClassName() { return "HybridFSEnv"; }
  const char* Name() const override { return kClassName(); }
  const char* NickName() const override { return kClassName(); }

  // Constructs the default Env, a singleton
  HybridFSEnv(const std::string& bdevname);
  ~HybridFSEnv();

  void Schedule(void (*function)(void*), void* arg, Env::Priority pri,
                void* tag, void (*unschedFunction)(void* arg)) override;

  void StartThread(void (*function)(void* arg), void* arg) override;

  Status GetHostName(char* name, uint64_t len) override;

  // Allow increasing the number of worker threads.
  void SetBackgroundThreads(int num, Env::Priority pri) override;
  int GetBackgroundThreads(Env::Priority pri) override;

  void IncBackgroundThreadsIfNeeded(int num, Env::Priority pri) override;

 private:
  std::vector<ThreadPoolImpl>& thread_pools_;
  std::vector<ThreadPoolImpl> thread_pools_storage_;
  std::vector<pthread_t> threads_to_join_storage_;
  std::vector<pthread_t>& threads_to_join_;
  pthread_mutex_t& mu_;
  pthread_mutex_t mu_storage_;
};
}  // namespace ROCKSDB_NAMESPACE