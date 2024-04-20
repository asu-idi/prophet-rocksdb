//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#include "env_hybrid.h"

#include <errno.h>
#include <unistd.h>

#include "env/io_posix.h"
#include "monitoring/thread_status_updater.h"
#include "port/lang.h"
#include "rocksdb/env.h"
#include "rocksdb/fs_hybrid.h"
#include "util/string_util.h"

namespace ROCKSDB_NAMESPACE {

struct StartThreadState {
  void (*user_function)(void*);
  void* arg;
};

static void* StartThreadWrapper(void* arg) {
  StartThreadState* state = reinterpret_cast<StartThreadState*>(arg);
  state->user_function(state->arg);
  delete state;
  return nullptr;
}

HybridFSEnv::HybridFSEnv(const std::string& bdevname)
    : CompositeEnv(PosixZenFSHybridFileSystem::Default(bdevname),
                   SystemClock::Default()),
      thread_pools_(thread_pools_storage_),
      thread_pools_storage_(Env::Priority::TOTAL),
      threads_to_join_(threads_to_join_storage_),
      mu_(mu_storage_) {
  ThreadPoolImpl::PthreadCall("mutex_init", pthread_mutex_init(&mu_, nullptr));
  for (int pool_id = 0; pool_id < Env::Priority::TOTAL; ++pool_id) {
    thread_pools_[pool_id].SetThreadPriority(
        static_cast<Env::Priority>(pool_id));
    // This allows later initializing the thread-local-env of each thread.
    thread_pools_[pool_id].SetHostEnv(this);
  }
  thread_status_updater_ = new ThreadStatusUpdater();
}

HybridFSEnv::~HybridFSEnv() {
  // All threads must be joined before the deletion of
  // thread_status_updater_.
  delete thread_status_updater_;
}

void HybridFSEnv::Schedule(void (*function)(void* arg1), void* arg,
                           Env::Priority pri, void* tag,
                           void (*unschedFunction)(void* arg)) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].Schedule(function, arg, tag, unschedFunction);
}

void HybridFSEnv::StartThread(void (*function)(void* arg), void* arg) {
  pthread_t t;
  StartThreadState* state = new StartThreadState;
  state->user_function = function;
  state->arg = arg;
  ThreadPoolImpl::PthreadCall(
      "start thread", pthread_create(&t, nullptr, &StartThreadWrapper, state));
  ThreadPoolImpl::PthreadCall("lock", pthread_mutex_lock(&mu_));
  threads_to_join_.push_back(t);
  ThreadPoolImpl::PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

Status HybridFSEnv::GetHostName(char* name, uint64_t len) {
  int ret = gethostname(name, static_cast<size_t>(len));
  if (ret < 0) {
    if (errno == EFAULT || errno == EINVAL) {
      return Status::InvalidArgument(errnoStr(errno).c_str());
    } else {
      return IOError("GetHostName", name, errno);
    }
  }
  return Status::OK();
}

// Allow increasing the number of worker threads.
void HybridFSEnv::SetBackgroundThreads(int num, Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].SetBackgroundThreads(num);
}

int HybridFSEnv::GetBackgroundThreads(Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  return thread_pools_[pri].GetBackgroundThreads();
}

// Allow increasing the number of worker threads.
void HybridFSEnv::IncBackgroundThreadsIfNeeded(int num, Env::Priority pri) {
  assert(pri >= Env::Priority::BOTTOM && pri <= Env::Priority::HIGH);
  thread_pools_[pri].IncBackgroundThreadsIfNeeded(num);
}

}  // namespace ROCKSDB_NAMESPACE