// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <stdint.h>

#include <optional>

#include "./Str.h"

namespace Mycila {
  namespace config {
    class Storage {
      public:
        Storage() = default;
        virtual ~Storage() = default;

        virtual bool begin(const char* name) { return false; }
        virtual void end() {}

        virtual bool hasKey(const char* key) const { return false; }

        // returns true if the key was removed, or did not exist, false on failure
        virtual bool remove(const char* key) { return false; }
        virtual bool removeAll() { return false; }

        virtual bool storeBool(const char* key, bool value) { return false; }
        virtual bool storeFloat(const char* key, float value) { return false; }
        virtual bool storeDouble(const char* key, double value) { return false; }
        virtual bool storeI8(const char* key, int8_t value) { return false; }
        virtual bool storeU8(const char* key, uint8_t value) { return false; }
        virtual bool storeI16(const char* key, int16_t value) { return false; }
        virtual bool storeU16(const char* key, uint16_t value) { return false; }
        virtual bool storeI32(const char* key, int32_t value) { return false; }
        virtual bool storeU32(const char* key, uint32_t value) { return false; }
        virtual bool storeI64(const char* key, int64_t value) { return false; }
        virtual bool storeU64(const char* key, uint64_t value) { return false; }
        virtual bool storeString(const char* key, const char* value) { return false; }

        virtual std::optional<bool> loadBool(const char* key) const { return std::nullopt; }
        virtual std::optional<float> loadFloat(const char* key) const { return std::nullopt; }
        virtual std::optional<double> loadDouble(const char* key) const { return std::nullopt; }
        virtual std::optional<int8_t> loadI8(const char* key) const { return std::nullopt; }
        virtual std::optional<uint8_t> loadU8(const char* key) const { return std::nullopt; }
        virtual std::optional<int16_t> loadI16(const char* key) const { return std::nullopt; }
        virtual std::optional<uint16_t> loadU16(const char* key) const { return std::nullopt; }
        virtual std::optional<int32_t> loadI32(const char* key) const { return std::nullopt; }
        virtual std::optional<uint32_t> loadU32(const char* key) const { return std::nullopt; }
        virtual std::optional<int64_t> loadI64(const char* key) const { return std::nullopt; }
        virtual std::optional<uint64_t> loadU64(const char* key) const { return std::nullopt; }
        virtual std::optional<Str> loadString(const char* key) const { return std::nullopt; }
    };
  } // namespace config
} // namespace Mycila
