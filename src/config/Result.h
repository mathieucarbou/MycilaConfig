// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

namespace Mycila {
  namespace config {
    enum class Status {
      PERSISTED,
      DEFAULTED,
      REMOVED,
      ERR_DISABLED,
      ERR_UNKNOWN_KEY,
      ERR_INVALID_TYPE,
      ERR_INVALID_VALUE,
      ERR_FAIL_ON_WRITE,
      ERR_FAIL_ON_REMOVE,
    };

    /**
     * Result of a set/unset operation
     */
    class Result {
      public:
        constexpr Result(Status status) noexcept : _status(status) {} // NOLINT
        // operation successful
        constexpr operator bool() const { return _status == Status::PERSISTED || _status == Status::DEFAULTED || _status == Status::REMOVED; }
        constexpr operator Status() const { return _status; }
        constexpr bool operator==(const Status& other) const { return _status == other; }
        // storage updated (value actually changed)
        constexpr bool isStorageUpdated() const { return _status == Status::PERSISTED || _status == Status::REMOVED; }

      private:
        Status _status;
    };
  } // namespace config
} // namespace Mycila
