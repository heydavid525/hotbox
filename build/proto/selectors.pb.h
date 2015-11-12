// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/selectors.proto

#ifndef PROTOBUF_proto_2fselectors_2eproto__INCLUDED
#define PROTOBUF_proto_2fselectors_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace hotbox {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_proto_2fselectors_2eproto();
void protobuf_AssignDesc_proto_2fselectors_2eproto();
void protobuf_ShutdownFile_proto_2fselectors_2eproto();

class OutlierExclusionSelector;
class TimeRangeSelector;
class UniformSubsampleSelector;

// ===================================================================

class OutlierExclusionSelector : public ::google::protobuf::Message {
 public:
  OutlierExclusionSelector();
  virtual ~OutlierExclusionSelector();

  OutlierExclusionSelector(const OutlierExclusionSelector& from);

  inline OutlierExclusionSelector& operator=(const OutlierExclusionSelector& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const OutlierExclusionSelector& default_instance();

  void Swap(OutlierExclusionSelector* other);

  // implements Message ----------------------------------------------

  inline OutlierExclusionSelector* New() const { return New(NULL); }

  OutlierExclusionSelector* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const OutlierExclusionSelector& from);
  void MergeFrom(const OutlierExclusionSelector& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(OutlierExclusionSelector* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int32 quantile_lower_bound = 1;
  void clear_quantile_lower_bound();
  static const int kQuantileLowerBoundFieldNumber = 1;
  ::google::protobuf::int32 quantile_lower_bound() const;
  void set_quantile_lower_bound(::google::protobuf::int32 value);

  // optional int32 quantile_upper_bound = 2;
  void clear_quantile_upper_bound();
  static const int kQuantileUpperBoundFieldNumber = 2;
  ::google::protobuf::int32 quantile_upper_bound() const;
  void set_quantile_upper_bound(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:hotbox.OutlierExclusionSelector)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::int32 quantile_lower_bound_;
  ::google::protobuf::int32 quantile_upper_bound_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_proto_2fselectors_2eproto();
  friend void protobuf_AssignDesc_proto_2fselectors_2eproto();
  friend void protobuf_ShutdownFile_proto_2fselectors_2eproto();

  void InitAsDefaultInstance();
  static OutlierExclusionSelector* default_instance_;
};
// -------------------------------------------------------------------

class UniformSubsampleSelector : public ::google::protobuf::Message {
 public:
  UniformSubsampleSelector();
  virtual ~UniformSubsampleSelector();

  UniformSubsampleSelector(const UniformSubsampleSelector& from);

  inline UniformSubsampleSelector& operator=(const UniformSubsampleSelector& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const UniformSubsampleSelector& default_instance();

  void Swap(UniformSubsampleSelector* other);

  // implements Message ----------------------------------------------

  inline UniformSubsampleSelector* New() const { return New(NULL); }

  UniformSubsampleSelector* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const UniformSubsampleSelector& from);
  void MergeFrom(const UniformSubsampleSelector& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(UniformSubsampleSelector* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional double sample_rate = 1;
  void clear_sample_rate();
  static const int kSampleRateFieldNumber = 1;
  double sample_rate() const;
  void set_sample_rate(double value);

  // @@protoc_insertion_point(class_scope:hotbox.UniformSubsampleSelector)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  double sample_rate_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_proto_2fselectors_2eproto();
  friend void protobuf_AssignDesc_proto_2fselectors_2eproto();
  friend void protobuf_ShutdownFile_proto_2fselectors_2eproto();

  void InitAsDefaultInstance();
  static UniformSubsampleSelector* default_instance_;
};
// -------------------------------------------------------------------

class TimeRangeSelector : public ::google::protobuf::Message {
 public:
  TimeRangeSelector();
  virtual ~TimeRangeSelector();

  TimeRangeSelector(const TimeRangeSelector& from);

  inline TimeRangeSelector& operator=(const TimeRangeSelector& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const TimeRangeSelector& default_instance();

  void Swap(TimeRangeSelector* other);

  // implements Message ----------------------------------------------

  inline TimeRangeSelector* New() const { return New(NULL); }

  TimeRangeSelector* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const TimeRangeSelector& from);
  void MergeFrom(const TimeRangeSelector& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(TimeRangeSelector* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string timestamp_begin = 1;
  void clear_timestamp_begin();
  static const int kTimestampBeginFieldNumber = 1;
  const ::std::string& timestamp_begin() const;
  void set_timestamp_begin(const ::std::string& value);
  void set_timestamp_begin(const char* value);
  void set_timestamp_begin(const char* value, size_t size);
  ::std::string* mutable_timestamp_begin();
  ::std::string* release_timestamp_begin();
  void set_allocated_timestamp_begin(::std::string* timestamp_begin);

  // optional string timestamp_end = 2;
  void clear_timestamp_end();
  static const int kTimestampEndFieldNumber = 2;
  const ::std::string& timestamp_end() const;
  void set_timestamp_end(const ::std::string& value);
  void set_timestamp_end(const char* value);
  void set_timestamp_end(const char* value, size_t size);
  ::std::string* mutable_timestamp_end();
  ::std::string* release_timestamp_end();
  void set_allocated_timestamp_end(::std::string* timestamp_end);

  // @@protoc_insertion_point(class_scope:hotbox.TimeRangeSelector)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr timestamp_begin_;
  ::google::protobuf::internal::ArenaStringPtr timestamp_end_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_proto_2fselectors_2eproto();
  friend void protobuf_AssignDesc_proto_2fselectors_2eproto();
  friend void protobuf_ShutdownFile_proto_2fselectors_2eproto();

  void InitAsDefaultInstance();
  static TimeRangeSelector* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// OutlierExclusionSelector

// optional int32 quantile_lower_bound = 1;
inline void OutlierExclusionSelector::clear_quantile_lower_bound() {
  quantile_lower_bound_ = 0;
}
inline ::google::protobuf::int32 OutlierExclusionSelector::quantile_lower_bound() const {
  // @@protoc_insertion_point(field_get:hotbox.OutlierExclusionSelector.quantile_lower_bound)
  return quantile_lower_bound_;
}
inline void OutlierExclusionSelector::set_quantile_lower_bound(::google::protobuf::int32 value) {
  
  quantile_lower_bound_ = value;
  // @@protoc_insertion_point(field_set:hotbox.OutlierExclusionSelector.quantile_lower_bound)
}

// optional int32 quantile_upper_bound = 2;
inline void OutlierExclusionSelector::clear_quantile_upper_bound() {
  quantile_upper_bound_ = 0;
}
inline ::google::protobuf::int32 OutlierExclusionSelector::quantile_upper_bound() const {
  // @@protoc_insertion_point(field_get:hotbox.OutlierExclusionSelector.quantile_upper_bound)
  return quantile_upper_bound_;
}
inline void OutlierExclusionSelector::set_quantile_upper_bound(::google::protobuf::int32 value) {
  
  quantile_upper_bound_ = value;
  // @@protoc_insertion_point(field_set:hotbox.OutlierExclusionSelector.quantile_upper_bound)
}

// -------------------------------------------------------------------

// UniformSubsampleSelector

// optional double sample_rate = 1;
inline void UniformSubsampleSelector::clear_sample_rate() {
  sample_rate_ = 0;
}
inline double UniformSubsampleSelector::sample_rate() const {
  // @@protoc_insertion_point(field_get:hotbox.UniformSubsampleSelector.sample_rate)
  return sample_rate_;
}
inline void UniformSubsampleSelector::set_sample_rate(double value) {
  
  sample_rate_ = value;
  // @@protoc_insertion_point(field_set:hotbox.UniformSubsampleSelector.sample_rate)
}

// -------------------------------------------------------------------

// TimeRangeSelector

// optional string timestamp_begin = 1;
inline void TimeRangeSelector::clear_timestamp_begin() {
  timestamp_begin_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& TimeRangeSelector::timestamp_begin() const {
  // @@protoc_insertion_point(field_get:hotbox.TimeRangeSelector.timestamp_begin)
  return timestamp_begin_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TimeRangeSelector::set_timestamp_begin(const ::std::string& value) {
  
  timestamp_begin_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:hotbox.TimeRangeSelector.timestamp_begin)
}
inline void TimeRangeSelector::set_timestamp_begin(const char* value) {
  
  timestamp_begin_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:hotbox.TimeRangeSelector.timestamp_begin)
}
inline void TimeRangeSelector::set_timestamp_begin(const char* value, size_t size) {
  
  timestamp_begin_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:hotbox.TimeRangeSelector.timestamp_begin)
}
inline ::std::string* TimeRangeSelector::mutable_timestamp_begin() {
  
  // @@protoc_insertion_point(field_mutable:hotbox.TimeRangeSelector.timestamp_begin)
  return timestamp_begin_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* TimeRangeSelector::release_timestamp_begin() {
  
  return timestamp_begin_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TimeRangeSelector::set_allocated_timestamp_begin(::std::string* timestamp_begin) {
  if (timestamp_begin != NULL) {
    
  } else {
    
  }
  timestamp_begin_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), timestamp_begin);
  // @@protoc_insertion_point(field_set_allocated:hotbox.TimeRangeSelector.timestamp_begin)
}

// optional string timestamp_end = 2;
inline void TimeRangeSelector::clear_timestamp_end() {
  timestamp_end_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& TimeRangeSelector::timestamp_end() const {
  // @@protoc_insertion_point(field_get:hotbox.TimeRangeSelector.timestamp_end)
  return timestamp_end_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TimeRangeSelector::set_timestamp_end(const ::std::string& value) {
  
  timestamp_end_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:hotbox.TimeRangeSelector.timestamp_end)
}
inline void TimeRangeSelector::set_timestamp_end(const char* value) {
  
  timestamp_end_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:hotbox.TimeRangeSelector.timestamp_end)
}
inline void TimeRangeSelector::set_timestamp_end(const char* value, size_t size) {
  
  timestamp_end_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:hotbox.TimeRangeSelector.timestamp_end)
}
inline ::std::string* TimeRangeSelector::mutable_timestamp_end() {
  
  // @@protoc_insertion_point(field_mutable:hotbox.TimeRangeSelector.timestamp_end)
  return timestamp_end_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* TimeRangeSelector::release_timestamp_end() {
  
  return timestamp_end_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void TimeRangeSelector::set_allocated_timestamp_end(::std::string* timestamp_end) {
  if (timestamp_end != NULL) {
    
  } else {
    
  }
  timestamp_end_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), timestamp_end);
  // @@protoc_insertion_point(field_set_allocated:hotbox.TimeRangeSelector.timestamp_end)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace hotbox

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_proto_2fselectors_2eproto__INCLUDED