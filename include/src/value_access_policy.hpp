#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sqlite {

  template <typename derived_t>
  struct value_access_policy {
  };

  struct default_value_access_policy : value_access_policy<default_value_access_policy> {
    template <typename value_type_t>
    struct local_type {
    };
  };
  
    template <>
    struct default_value_access_policy::local_type<std::vector<uint8_t>> {
      const int sqlite_type = SQLITE_BLOB;
      typedef std::vector<uint8_t> value_type;

      static value_type null_value() {
        return value_type();
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        ::sqlite3_column_blob(stmt, i);
        const int sz = ::sqlite3_column_bytes(stmt, i);
        value_type value(sz);
        const char* b = reinterpret_cast<const char*>(::sqlite3_column_blob(stmt, i));
        std::copy(b, b + sz, &value[0]);
        return value;
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type& value) {
        return sqlite3_bind_blob(stmt, i, &value[0], value.size(), SQLITE_TRANSIENT);
      }
    };
  
    template <>
    struct default_value_access_policy::local_type<std::string> {
      const int sqlite_type = SQLITE_TEXT;
      typedef std::string value_type;
    
      static value_type null_value() {
        return value_type();
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        ::sqlite3_column_text(stmt, i);
        const int sz = ::sqlite3_column_bytes(stmt, i);
        value_type value;
        value.resize(sz);
        const unsigned char *c = ::sqlite3_column_text(stmt, i);
        std::copy(c, c + value.size(), &value[0]);
        return value;
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type& value) {
        return sqlite3_bind_text(stmt, i, value.c_str(), -1, SQLITE_TRANSIENT);
      }
    };

    template <>
    struct default_value_access_policy::local_type<int64_t> {
      const int sqlite_type = SQLITE_INTEGER;
      typedef int64_t value_type;

      static value_type null_value() {
        return value_type(0);
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        return value_type(::sqlite3_column_int64(stmt, i));
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type value) {
        return sqlite3_bind_int64(stmt, i, value);
      }
    
    };

    template <>
    struct default_value_access_policy::local_type<int32_t> {
      const int sqlite_type = SQLITE_INTEGER;
      typedef int32_t value_type;

      static value_type null_value() {
        return value_type(0);
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        return value_type(::sqlite3_column_int(stmt, i));
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type value) {
        return sqlite3_bind_int(stmt, i, value);
      }

    };
  
    template <>
    struct default_value_access_policy::local_type<double> {
      const int sqlite_type = SQLITE_FLOAT;
      typedef double value_type;

      static value_type null_value() {
        return value_type(0);
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        return value_type(::sqlite3_column_double(stmt, i));
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type value) {
        return sqlite3_bind_double(stmt, i, value);
      }
    };

    template <>
    struct default_value_access_policy::local_type<float> {
      const int sqlite_type = SQLITE_FLOAT;
      typedef float value_type;

      static value_type null_value() {
        return value_type(0);
      }

      static value_type get_column_from_stmt(sqlite3_stmt* stmt, int i) {
        return value_type(::sqlite3_column_double(stmt, i));
      }

      static int bind(sqlite3_stmt* stmt, int i, const value_type value) {
        return sqlite3_bind_double(stmt, i, double(value));
      }
    };
  
}
