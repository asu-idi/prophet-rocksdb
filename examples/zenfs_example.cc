// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

// Examples with Zenfs.

#include <iostream>
#include <string>

#include "env/env_hybrid.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/plugin/zenfs/fs/fs_zenfs.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::DestroyDB;
using ROCKSDB_NAMESPACE::HybridFSEnv;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteOptions;
using ROCKSDB_NAMESPACE::Slice;

std::string dbPath = "/hybrid_example";

void findAndListAllZenFS() {
  // List all ZenFS in current system
  std::map<std::string, std::string> list;
  Status s = ROCKSDB_NAMESPACE::ListZenFileSystems(list);
  assert(s.ok());  // check if okay

  // Print all available ZenFS if possible
  std::map<std::string, std::string>::iterator it;
  for (it = list.begin(); it != list.end(); it++) {
    std::cout << it->first          // string (key)
              << ':' << it->second  // string's value
              << std::endl;
  }
  assert(list.size() > 0);
}

int main() {
  // Find and list all ZenFS in current system
  findAndListAllZenFS();

  DB* hybrid = nullptr;
  Options hybridOptions;
  hybridOptions.env = new HybridFSEnv("nullb0");
  hybridOptions.create_if_missing = true;

  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  hybridOptions.IncreaseParallelism();
  hybridOptions.level0_slowdown_writes_trigger = 3;
  hybridOptions.level0_file_num_compaction_trigger = 4;
  hybridOptions.level0_stop_writes_trigger = 5;
  hybridOptions.max_bytes_for_level_base = 1000;
  hybridOptions.max_bytes_for_level_multiplier = 2;

  // Clean files from previous tests
  DestroyDB(dbPath, hybridOptions);

  // Open db
  Status s = DB::Open(hybridOptions, dbPath, &hybrid);
  assert(s.ok());

  // Delete any dirty files. ex. linked files...
  std::vector<std::string> dirty_files;
  hybridOptions.env->GetChildren(dbPath, &dirty_files);
  for(const std::string &name : dirty_files){
    hybridOptions.env->DeleteFile(dbPath + "/" + name);
  }

  // Put key-value
  s = hybrid->Put(WriteOptions(), "key1", "value");
  assert(s.ok());

  // Get value
  std::string value;
  s = hybrid->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

  // Delete value
  s = hybrid->Delete(WriteOptions(), "key1");
  assert(s.ok());

  // Verify the delete is completed
  s = hybrid->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  // Compaction test
  // if background compaction is not working, write will stall
  // because of options.level0_stop_writes_trigger
  for (int i = 0; i < 999999; ++i) {
    hybrid->Put(WriteOptions(), std::to_string(i),
                std::string(500, 'a' + (i % 26)));
  }

  // verify the values are still there
  for (int i = 0; i < 999999; ++i) {
    hybrid->Get(ReadOptions(), std::to_string(i), &value);
    assert(value == std::string(500, 'a' + (i % 26)));
  }

  // Flush all data from memory to drive
  s = hybrid->Flush(ROCKSDB_NAMESPACE::FlushOptions());
  assert(s.ok());

  // TEST LinkFile() and AreFilesSame()---------------------------
  std::vector<std::string> f_names;
  std::vector<std::string> sst_names;
  hybridOptions.env->GetChildren(dbPath, &f_names);
  for(const std::string &name : f_names){
    Slice rest(name);
    if(rest.ends_with(".sst")){
      sst_names.push_back(name);
    }
  }

  // sst_names should contains both posix and zenfs filenames where posix names in
  // front and zenfs names in back.
  assert(!sst_names.empty());

  std::string first_posix_sst_name = dbPath + "/" + sst_names[0];
  s = hybridOptions.env->LinkFile(first_posix_sst_name,
                                  first_posix_sst_name + "_another.sst");
  assert(s.ok());
  bool same = false;
  s = hybridOptions.env->AreFilesSame(
      first_posix_sst_name, first_posix_sst_name + "_another.sst", &same);
  assert(s.ok() && same);

  std::string first_zenfs_sst_name = dbPath + "/" + sst_names[sst_names.size() - 1];
  s = hybridOptions.env->LinkFile(first_zenfs_sst_name,
                                  first_zenfs_sst_name + "_another.sst");
  assert(s.ok());
  s = hybridOptions.env->AreFilesSame(
      first_zenfs_sst_name, first_zenfs_sst_name + "_another.sst", &same);
  assert(s.ok() && same);
  //-------------------------------------------------------------

  // Test transfer function since these 2 table files are in diff. mediums.
  s = hybridOptions.env->AreFilesSame(first_posix_sst_name,
                                      first_zenfs_sst_name, &same);
  assert(s.ok());

  // Close db
  s = hybrid->Close();
  assert(s.ok());

  // Remove and free ptr
  delete hybrid;

  return 0;
}