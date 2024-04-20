#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include "env/env_hybrid.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/plugin/zenfs/fs/fs_zenfs.h"

using namespace std;

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::DestroyDB;
using ROCKSDB_NAMESPACE::HybridFSEnv;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteOptions;
using ROCKSDB_NAMESPACE::Slice;

const std::string PATH = "/tmp/test_rocksdb";

const long long limit = 1e5;
const int N = 2e5;
long long get_random() {
  long long rd = rand();
  long long rd2 = rand();
  return (rd << 31ll | rd2) % limit + 1;
  // return rand() % 1000000;
}
int main() {
  freopen("out.txt", "w", stdout);
  clock_t start, end;  //定义clock_t变量
  start = clock();     //开始时间
  DB* db;
  Options options;
  options.create_if_missing = true;
  options.env = new HybridFSEnv("nullb0");

  const int B = 1;
  const int KB = 1024;
  const int MB = 1024 * 1024;
  // options.max_bytes_for_level_base=10 * 1048;
  // options.target_file_size_base=2097152;
  // options.write_buffer_size = 4194304;
  options.max_bytes_for_level_base= 1 * KB;
  options.target_file_size_base = 1 * KB;
  options.write_buffer_size = 2 * KB;
  
  options.max_bytes_for_level_multiplier=5;

  options.max_background_compactions=1;
  options.max_background_flushes=1;
  options.max_background_jobs=1;

  options.soft_pending_compaction_bytes_limit = options.target_file_size_base;
  options.hard_pending_compaction_bytes_limit = options.target_file_size_base; //这里限制compaction的速率是因为rocksdb的flush速率特别快，不然来不及compaction

  options.num_levels=7;
  options.level0_stop_writes_trigger=12;
  options.level0_slowdown_writes_trigger=8;
  options.level0_file_num_compaction_trigger=4;
  options.max_write_buffer_number=1;
  options.compaction_style=rocksdb::kCompactionStyleLevel;
  options.compaction_pri=rocksdb::kRoundRobin;
  options.max_open_files=1000;
  options.target_file_size_multiplier=1;

  Status status = DB::Open(options, PATH, &db);
  assert(status.ok());
  for (int i = 0; i < N; i++) {
    std::string key = std::to_string(get_random());
    std::string value = std::to_string(get_random());
    status = db->Put(WriteOptions(), key, value);
    std::string get_value;
    if (status.ok()) {
      status = db->Get(ReadOptions(), key, &get_value);
      if (status.ok()) {
         //printf("get %s\n", get_value.c_str());
      } else {
        printf("get failed\n");
      }
      //printf("success %d\n", i);
    } else {
      printf("put failed\n");
    }
  }

  delete db;
  end = clock();  //结束时间
  cout << "time = " << double(end - start) / CLOCKS_PER_SEC << "s"
       << endl;  //输出时间
  return 0;
}
