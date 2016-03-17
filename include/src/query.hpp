#pragma once

#include <sqlite3.h>

#include <memory>

#include "value_access_policy.hpp"

namespace sqlite {
  template <typename value_access_t = default_value_access_policy>
  class query_base;

  class query;
  
  template <typename value_access_t>
  class query_base : public result_code_container {
  public:
    typedef query_base type;
    typedef std::shared_ptr<type> type_ptr;
    
    query_base(const database::type_ptr& db) : db_(db) {}
    
    query_base(const database::type_ptr& db, const std::string& query_str) :
      result_code_container(),
      db_(db),
      query_str_(query_str) {
      prepare();
    }

    query_base(const type& other) :
      result_code_container(other),
      query_str_(other.query_str_),
      stmt_(other.stmt_),
      db_(other.db_) {
    }

    query_base(type&& other) :
      result_code_container(other),
      query_str_(std::move(other.query_str_)),
      stmt_(std::move(other.stmt_)),
      db_(std::move(other.db_)) {
    }

    void swap(type& other) {
      std::swap(result_code_, other.result_code_);
      std::swap(query_str_, other.query_str_);
      std::swap(stmt_, other.stmt_);
      std::swap(db_, other.db_);
    }

    type& operator=(const type& other) {
      type tmp(other);
      swap(tmp);
      return *this;
    }

    const std::shared_ptr<sqlite3_stmt>& statement() const {
      return stmt_;
    }

    template <typename T>
    void bind(const int i, const T& value) {
      typedef typename value_access_t::template local_type<T> value_policy;
      result_code_ = value_policy::bind(stmt_.get(), i, value);
    }
    
    template <std::size_t I = 0, typename... Tp>
    typename std::enable_if<I == sizeof...(Tp), void>::type bind_tuple(const int i, const std::tuple<Tp...>& t) {
    }

    template <std::size_t I = 0, typename... Tp>
    typename std::enable_if <I < sizeof...(Tp), void>::type bind_tuple(const int i, const std::tuple<Tp...>& t) {
      bind(i, std::get<I>(t));
      bind_tuple<I + 1, Tp...>(i+ 1, t);
    }

    template <typename T>
    T get(const int i) {
      typedef typename value_access_t::template local_type<T> value_policy;
      const int ct = ::sqlite3_column_type(stmt_.get(), i);
      if (ct == SQLITE_NULL) {
        return value_policy::null_value();
      } else {
        return value_policy::get_column_from_stmt(stmt_.get(), i);
      }
    }
    
    template <typename T>
    void get(const int i, T& value) {
      value = get<T>(i);
    }

    void step() {
      result_code_ = sqlite3_step(stmt_.get());
    }

    void prepare(const std::string& query_str) {
      query_str_ = query_str;
      prepare();
    }
  protected:
    database::type_ptr db_;
    std::string query_str_;
    std::shared_ptr<sqlite3_stmt> stmt_;    

    void prepare() {
      ::sqlite3_stmt* stmt;
      if (db_->db().get() != nullptr) {
        result_code_ = sqlite3_prepare(db_->db().get(), query_str_.c_str(),
                                       query_str_.length() + 1, &stmt, nullptr);
        if (result_code_ == SQLITE_OK) {
          stmt_ = std::shared_ptr<::sqlite3_stmt>(stmt, [] (::sqlite3_stmt* p) {
              sqlite3_finalize(p);
            });
        }
      } else {
        result_code_ = SQLITE_ERROR;
      }
    }
  };

  class query : public query_base<default_value_access_policy> {
    using query_base<default_value_access_policy>::query_base;
  };
}
