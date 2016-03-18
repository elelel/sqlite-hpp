#pragma once

#include <memory>
#include <string>

#include <sqlite3.h>

#include "logging.hpp"
#include "result_code_container.hpp"

namespace sqlite {

  class database : public result_code_container {
  public:
    typedef database type;
    typedef std::shared_ptr<database> type_ptr;
    
    database() {
      SQLITE_HPP_LOG("database::database() constructor");
    }

    database(const std::string& filename) : database() {
      SQLITE_HPP_LOG("database::database(filename) constructor");
      open(filename);
    }

    database(const database& other) :
      result_code_container(other),
      filename_(other.filename_),
      db_(other.db_) {
      SQLITE_HPP_LOG("database::database copy constructor");
    }

    database(database&& other) :
      result_code_container(other),
      filename_(std::move(other.filename_)),
      db_(std::move(other.db_)) {
      SQLITE_HPP_LOG("database::database move constructor");
    }

    void swap(database& other) {
      SQLITE_HPP_LOG("database::swap");
      std::swap(filename_, other.filename_);
      std::swap(db_, other.db_);
      std::swap(result_code_, other.result_code_);
    }

    database& operator=(const database& other) {
      SQLITE_HPP_LOG("database::operator=");
      database tmp(other);
      swap(tmp);
      return *this;
    }
    
    const int open(const std::string& filename) {
      ::sqlite3 *db;
      result_code_ = sqlite3_open(filename.c_str(), &db);
      if (result_code_ == SQLITE_OK) {
        filename_ = filename;
        db_ = std::shared_ptr<::sqlite3>(db, [this] (sqlite3* p) {
            SQLITE_HPP_LOG("database::db_ Database closed");
            this->result_code_ = sqlite3_close(p); });
      } else {
        SQLITE_HPP_LOG(std::string("sqlite::database::open failed to open ") + filename);
      }
      return result_code_;
    }

    const void close() {
      db_ = nullptr;
    }

    const std::shared_ptr<::sqlite3>& db() {
      return db_;
    }

    const int sqlite_max_length() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_LENGTH, -1);
    }

    const int sqlite_max_sql_length() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_SQL_LENGTH, -1);
    }

    const int sqlite_max_column() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_COLUMN, -1);
    }

    const int sqlite_max_expr_depth() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_EXPR_DEPTH, -1);
    }

    const int sqlite_max_compound_select() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_COMPOUND_SELECT, -1);
    }

    const int sqlite_max_vdbe_op() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_VDBE_OP, -1);
    }

    const int sqlite_max_function_arg() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_FUNCTION_ARG, -1);
    }

    const int sqlite_max_attached() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_ATTACHED, -1);
    }

    const int sqlite_max_like_pattern_length() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_LIKE_PATTERN_LENGTH, -1);
    }

    const int sqlite_max_variable_number() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_VARIABLE_NUMBER, -1);
    }

    const int sqlite_max_trigger_depth() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_TRIGGER_DEPTH, -1);
    }

    const int sqlite_max_worker_threads() {
      return sqlite3_limit(db_.get(), SQLITE_LIMIT_WORKER_THREADS, -1);
    }

    void sqlite_max_length(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_LENGTH, new_limit);
    }

    void sqlite_max_sql_length(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_SQL_LENGTH, new_limit);
    }

    void sqlite_max_column(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_COLUMN, new_limit);
    }

    void sqlite_max_expr_depth(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_EXPR_DEPTH, new_limit);
    }

    void sqlite_max_compound_select(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_COMPOUND_SELECT, new_limit);
    }

    void sqlite_max_vdbe_op(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_VDBE_OP, new_limit);
    }

    void sqlite_max_function_arg(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_FUNCTION_ARG, new_limit);
    }

    void sqlite_max_attached(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_ATTACHED, new_limit);
    }

    void sqlite_max_like_pattern_length(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_LIKE_PATTERN_LENGTH, new_limit);
    }

    void sqlite_max_variable_number(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_VARIABLE_NUMBER, new_limit);
    }

    void sqlite_max_trigger_depth(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_TRIGGER_DEPTH, new_limit);
    }

    void sqlite_max_worker_threads(const int new_limit) {
      sqlite3_limit(db_.get(), SQLITE_LIMIT_WORKER_THREADS, new_limit);
    }
    
  private:
    std::string filename_;
    std::shared_ptr<::sqlite3> db_;
  };
}
