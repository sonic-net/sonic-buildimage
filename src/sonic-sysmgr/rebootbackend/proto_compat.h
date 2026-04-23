#pragma once

#include <google/protobuf/util/json_util.h>

/** 
 * Source compatibility layer for protobuf versions.
 *
 * The Makefile-based version of SONiC uses Protobuf 3.21.12,
 * whereas the Bazel build tries to use the latest protobuf (as of time of writing, 34.0).
 * There are source incompatibilities between the two versions, and this file bridges that gap.
 * It always defaults to a version compatible with the Makefile-based build system,
 * only switching to Bazel if the `BAZEL` macro is defined.
 */
#ifdef BAZEL

/** Protobuf 3.22.0 introduced a dependency on Abseil,
 * which slowly changed their public API: https://protobuf.dev/news/v22/#abseil-dep
 * One of the changes was replacing `google::protobuf::util::Status` with `absl::Status`:
 * https://github.com/protocolbuffers/protobuf/commit/a3c8e2deb05186334b7ee8c1174f44802e38b43d
 *
 * We re-introduce the alias for compatibility.
 */
namespace google {
namespace protobuf {
namespace util {
  using Status = absl::Status; 
}
}
}

#endif // BAZEL
