//  Copyright (c) Meta Platforms, Inc. and affiliates.
//
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/compaction/compaction_outputs.h"

#include "db/builder.h"
#include "rocksdb/plugin/zenfs/fs/zbd_zenfs.h"
#include <iostream>

namespace ROCKSDB_NAMESPACE {
extern void get_predict(int level, const FileMetaData &file, Version *v, const Compaction* compaction_, int &predict_, int &predict_type_, int &tmp_rank);
extern void get_overlap(const FileMetaData &file, int target_level, Version *v, std::vector<std::string> &overlap_list);
extern void set_deleted_time(int fnumber, int clock);
extern int get_clock();
extern std::string get_fname(uint64_t id);
void CompactionOutputs::NewBuilder(const TableBuilderOptions& tboptions) {
  builder_.reset(NewTableBuilder(tboptions, file_writer_.get()));
}

Status CompactionOutputs::Finish(const Status& intput_status,
                                 const SeqnoToTimeMapping& seqno_time_mapping) {
  FileMetaData* meta = GetMetaData();
  int predict;
  int predict_type;
  int rank;
  const int output_level = GetCompaction()->output_level();
  printf("CompactionOutputs::Finish number=%ld get_clock=%d output_level=%d start_level=%d num_input_level=%ld\n", meta->fnumber, get_clock(), output_level, GetCompaction()->start_level(), GetCompaction()->num_input_levels());
  get_predict(output_level, *meta, GetCompaction()->column_family_data()->current(), GetCompaction(), predict, predict_type, rank);
  std::vector<std::string> overlap_list;
  if(output_level + 1 <= 6) {
    get_overlap(*meta, output_level + 1, GetCompaction()->column_family_data()->current(), overlap_list);
  }
  set_deleted_time(meta->fnumber, predict + get_clock());

  //fs_->SetFileLifetime(get_fname(meta->fd.GetNumber()), predict + get_clock(), get_clock(), 0, (predict < 50) ? 1: output_level, overlap_list);
  if(ENABLE_SHORT_WITH_TYPE0 != -1) {
    fs_->SetFileLifetime(get_fname(meta->fd.GetNumber()), predict + get_clock(), get_clock(), 0, (predict < ENABLE_SHORT_WITH_TYPE0) ? 1 : output_level, overlap_list);
  } else {
    fs_->SetFileLifetime(get_fname(meta->fd.GetNumber()), predict + get_clock(), get_clock(), 0, output_level, overlap_list);
  }
  if(!update_input_file_lifetime) {
    for(size_t i = 0; i < GetCompaction()->num_input_levels(); i++) {
      //printf("vector[%ld] element:\n", i);
      for(size_t j = 0; j < GetCompaction()->num_input_files(i); j++) {
        FileMetaData *tmp = GetCompaction()->input(i, j);
        fs_->SetFileLifetime(get_fname(tmp->fd.GetNumber()), get_clock(), get_clock(), 1, output_level, std::vector<std::string> {});
      }
    }
    update_input_file_lifetime = 1;
  }

  

  std::cout << "Finish:"
            << meta->fd.GetNumber() 
            << '[' << meta->smallest.user_key().ToString() <<  ','
            << meta->largest.user_key().ToString() << ']' 
            << "lifetime=" << predict
            << '\n';

  assert(meta != nullptr);
  Status s = intput_status;
  if (s.ok()) {
    std::string seqno_time_mapping_str;
    seqno_time_mapping.Encode(seqno_time_mapping_str, meta->fd.smallest_seqno,
                              meta->fd.largest_seqno, meta->file_creation_time);
    builder_->SetSeqnoTimeTableProperties(seqno_time_mapping_str,
                                          meta->oldest_ancester_time);
    s = builder_->Finish();

  } else {
    builder_->Abandon();
  }
  Status io_s = builder_->io_status();
  if (s.ok()) {
    s = io_s;
  } else {
    io_s.PermitUncheckedError();
  }
  const uint64_t current_bytes = builder_->FileSize();
  if (s.ok()) {
    meta->fd.file_size = current_bytes;
    meta->marked_for_compaction = builder_->NeedCompact();
  }
  current_output().finished = true;
  stats_.bytes_written += current_bytes;
  stats_.num_output_files = outputs_.size();

  return s;
}

IOStatus CompactionOutputs::WriterSyncClose(const Status& input_status,
                                            SystemClock* clock,
                                            Statistics* statistics,
                                            bool use_fsync) {
  IOStatus io_s;
  if (input_status.ok()) {
    StopWatch sw(clock, statistics, COMPACTION_OUTFILE_SYNC_MICROS);
    io_s = file_writer_->Sync(use_fsync);
  }
  if (input_status.ok() && io_s.ok()) {
    io_s = file_writer_->Close();
  }

  if (input_status.ok() && io_s.ok()) {
    FileMetaData* meta = GetMetaData();
    meta->file_checksum = file_writer_->GetFileChecksum();
    meta->file_checksum_func_name = file_writer_->GetFileChecksumFuncName();
  }

  file_writer_.reset();

  return io_s;
}

//重点观察对象
Status CompactionOutputs::AddToOutput(
    const CompactionIterator& c_iter,
    const CompactionFileOpenFunc& open_file_func,
    const CompactionFileCloseFunc& close_file_func) {
  Status s;
  const Slice& key = c_iter.key();

  if (!pending_close_ && c_iter.Valid() && partitioner_ && HasBuilder() &&
      partitioner_->ShouldPartition(
          PartitionerRequest(last_key_for_partitioner_, c_iter.user_key(),
                             current_output_file_size_)) == kRequired) {
    pending_close_ = true;
  }

  if (pending_close_) {
    s = close_file_func(*this, c_iter.InputStatus(), key);
    pending_close_ = false;
  }
  if (!s.ok()) {
    return s;
  }

  // Open output file if necessary
  if (!HasBuilder()) {
    s = open_file_func(*this); //在这里调用OpenCompactionOutputFile
  }
  if (!s.ok()) {
    return s;
  }

  Output& curr = current_output();
  assert(builder_ != nullptr);
  const Slice& value = c_iter.value();
  s = curr.validator.Add(key, value);
  if (!s.ok()) {
    return s;
  }
  builder_->Add(key, value);

  stats_.num_output_records++;
  current_output_file_size_ = builder_->EstimatedFileSize();

  if (blob_garbage_meter_) {
    s = blob_garbage_meter_->ProcessOutFlow(key, value);
  }

  if (!s.ok()) {
    return s;
  }

  const ParsedInternalKey& ikey = c_iter.ikey();
  s = current_output().meta.UpdateBoundaries(key, value, ikey.sequence,
                                             ikey.type);

  // Close output file if it is big enough. Two possibilities determine it's
  // time to close it: (1) the current key should be this file's last key, (2)
  // the next key should not be in this file.
  //
  // TODO(aekmekji): determine if file should be closed earlier than this
  // during subcompactions (i.e. if output size, estimated by input size, is
  // going to be 1.2MB and max_output_file_size = 1MB, prefer to have 0.6MB
  // and 0.6MB instead of 1MB and 0.2MB)
  if (compaction_->output_level() != 0 &&
      current_output_file_size_ >= compaction_->max_output_file_size()) {
    pending_close_ = true;
  }

  if (partitioner_) {
    last_key_for_partitioner_.assign(c_iter.user_key().data_,
                                     c_iter.user_key().size_);
  }

  return s;
}

Status CompactionOutputs::AddRangeDels(
    const Slice* comp_start, const Slice* comp_end,
    CompactionIterationStats& range_del_out_stats, bool bottommost_level,
    const InternalKeyComparator& icmp, SequenceNumber earliest_snapshot,
    const Slice& next_table_min_key) {
  assert(HasRangeDel());
  FileMetaData& meta = current_output().meta;
  const Comparator* ucmp = icmp.user_comparator();

  Slice lower_bound_guard, upper_bound_guard;
  std::string smallest_user_key;
  const Slice *lower_bound, *upper_bound;
  bool lower_bound_from_sub_compact = false;

  size_t output_size = outputs_.size();
  if (output_size == 1) {
    // For the first output table, include range tombstones before the min
    // key but after the subcompaction boundary.
    lower_bound = comp_start;
    lower_bound_from_sub_compact = true;
  } else if (meta.smallest.size() > 0) {
    // For subsequent output tables, only include range tombstones from min
    // key onwards since the previous file was extended to contain range
    // tombstones falling before min key.
    smallest_user_key = meta.smallest.user_key().ToString(false /*hex*/);
    lower_bound_guard = Slice(smallest_user_key);
    lower_bound = &lower_bound_guard;
  } else {
    lower_bound = nullptr;
  }
  if (!next_table_min_key.empty()) {
    // This may be the last file in the subcompaction in some cases, so we
    // need to compare the end key of subcompaction with the next file start
    // key. When the end key is chosen by the subcompaction, we know that
    // it must be the biggest key in output file. Therefore, it is safe to
    // use the smaller key as the upper bound of the output file, to ensure
    // that there is no overlapping between different output files.
    upper_bound_guard = ExtractUserKey(next_table_min_key);
    if (comp_end != nullptr &&
        ucmp->Compare(upper_bound_guard, *comp_end) >= 0) {
      upper_bound = comp_end;
    } else {
      upper_bound = &upper_bound_guard;
    }
  } else {
    // This is the last file in the subcompaction, so extend until the
    // subcompaction ends.
    upper_bound = comp_end;
  }
  bool has_overlapping_endpoints;
  if (upper_bound != nullptr && meta.largest.size() > 0) {
    has_overlapping_endpoints =
        ucmp->Compare(meta.largest.user_key(), *upper_bound) == 0;
  } else {
    has_overlapping_endpoints = false;
  }

  // The end key of the subcompaction must be bigger or equal to the upper
  // bound. If the end of subcompaction is null or the upper bound is null,
  // it means that this file is the last file in the compaction. So there
  // will be no overlapping between this file and others.
  assert(comp_end == nullptr || upper_bound == nullptr ||
         ucmp->Compare(*upper_bound, *comp_end) <= 0);
  auto it = range_del_agg_->NewIterator(lower_bound, upper_bound,
                                        has_overlapping_endpoints);
  // Position the range tombstone output iterator. There may be tombstone
  // fragments that are entirely out of range, so make sure that we do not
  // include those.
  if (lower_bound != nullptr) {
    it->Seek(*lower_bound);
  } else {
    it->SeekToFirst();
  }
  for (; it->Valid(); it->Next()) {
    auto tombstone = it->Tombstone();
    if (upper_bound != nullptr) {
      int cmp = ucmp->Compare(*upper_bound, tombstone.start_key_);
      if ((has_overlapping_endpoints && cmp < 0) ||
          (!has_overlapping_endpoints && cmp <= 0)) {
        // Tombstones starting after upper_bound only need to be included in
        // the next table. If the current SST ends before upper_bound, i.e.,
        // `has_overlapping_endpoints == false`, we can also skip over range
        // tombstones that start exactly at upper_bound. Such range
        // tombstones will be included in the next file and are not relevant
        // to the point keys or endpoints of the current file.
        break;
      }
    }

    if (bottommost_level && tombstone.seq_ <= earliest_snapshot) {
      // TODO(andrewkr): tombstones that span multiple output files are
      // counted for each compaction output file, so lots of double
      // counting.
      range_del_out_stats.num_range_del_drop_obsolete++;
      range_del_out_stats.num_record_drop_obsolete++;
      continue;
    }

    auto kv = tombstone.Serialize();
    assert(lower_bound == nullptr ||
           ucmp->Compare(*lower_bound, kv.second) < 0);
    // Range tombstone is not supported by output validator yet.
    builder_->Add(kv.first.Encode(), kv.second);
    InternalKey smallest_candidate = std::move(kv.first);
    if (lower_bound != nullptr &&
        ucmp->Compare(smallest_candidate.user_key(), *lower_bound) <= 0) {
      // Pretend the smallest key has the same user key as lower_bound
      // (the max key in the previous table or subcompaction) in order for
      // files to appear key-space partitioned.
      //
      // When lower_bound is chosen by a subcompaction, we know that
      // subcompactions over smaller keys cannot contain any keys at
      // lower_bound. We also know that smaller subcompactions exist,
      // because otherwise the subcompaction woud be unbounded on the left.
      // As a result, we know that no other files on the output level will
      // contain actual keys at lower_bound (an output file may have a
      // largest key of lower_bound@kMaxSequenceNumber, but this only
      // indicates a large range tombstone was truncated). Therefore, it is
      // safe to use the tombstone's sequence number, to ensure that keys at
      // lower_bound at lower levels are covered by truncated tombstones.
      //
      // If lower_bound was chosen by the smallest data key in the file,
      // choose lowest seqnum so this file's smallest internal key comes
      // after the previous file's largest. The fake seqnum is OK because
      // the read path's file-picking code only considers user key.
      smallest_candidate = InternalKey(
          *lower_bound, lower_bound_from_sub_compact ? tombstone.seq_ : 0,
          kTypeRangeDeletion);
    }
    InternalKey largest_candidate = tombstone.SerializeEndKey();
    if (upper_bound != nullptr &&
        ucmp->Compare(*upper_bound, largest_candidate.user_key()) <= 0) {
      // Pretend the largest key has the same user key as upper_bound (the
      // min key in the following table or subcompaction) in order for files
      // to appear key-space partitioned.
      //
      // Choose highest seqnum so this file's largest internal key comes
      // before the next file's/subcompaction's smallest. The fake seqnum is
      // OK because the read path's file-picking code only considers the
      // user key portion.
      //
      // Note Seek() also creates InternalKey with (user_key,
      // kMaxSequenceNumber), but with kTypeDeletion (0x7) instead of
      // kTypeRangeDeletion (0xF), so the range tombstone comes before the
      // Seek() key in InternalKey's ordering. So Seek() will look in the
      // next file for the user key.
      largest_candidate =
          InternalKey(*upper_bound, kMaxSequenceNumber, kTypeRangeDeletion);
    }
#ifndef NDEBUG
    SequenceNumber smallest_ikey_seqnum = kMaxSequenceNumber;
    if (meta.smallest.size() > 0) {
      smallest_ikey_seqnum = GetInternalKeySeqno(meta.smallest.Encode());
    }
#endif
    meta.UpdateBoundariesForRange(smallest_candidate, largest_candidate,
                                  tombstone.seq_, icmp);
    // The smallest key in a file is used for range tombstone truncation, so
    // it cannot have a seqnum of 0 (unless the smallest data key in a file
    // has a seqnum of 0). Otherwise, the truncated tombstone may expose
    // deleted keys at lower levels.
    assert(smallest_ikey_seqnum == 0 ||
           ExtractInternalKeyFooter(meta.smallest.Encode()) !=
               PackSequenceAndType(0, kTypeRangeDeletion));
  }
  return Status::OK();
}
}  // namespace ROCKSDB_NAMESPACE
