#pragma once

#include "query.hpp"

#include <tuple>

#include "value_access_policy.hpp"

#include "logging.hpp"

namespace sqlite {
  template <typename value_access_policy_t, typename... Rs>
  class buffered_insert_query_base;
  /*  template <typename value_access_policy_t, typename... Rs>
      class buffered_insert_query_iterator; */
  template <typename... Rs> 
  class buffered_insert_query;

  template <typename value_access_policy_t, typename... Rs>
  class buffered_insert_query_base : public query_base<value_access_policy_t> {
    friend class input_query_iterator<value_access_policy_t, Rs...>;
  public:
    typedef buffered_insert_query_base<value_access_policy_t, Rs...> type;
    typedef std::shared_ptr<type> type_ptr;
    typedef std::tuple<Rs...> value_type;

    using query_base<value_access_policy_t>::query_base;

    template <typename fields_container_t>
    buffered_insert_query_base(const database::type_ptr &db, const std::string& table_name,
                               fields_container_t fields) :
      query_base<value_access_policy_t>(db),
      max_sql_length_(db->sqlite_max_sql_length()),
      max_compound_select_(db->sqlite_max_compound_select()),
      max_variable_number_(db->sqlite_max_variable_number())
    {

      // TODO: Make policy-configurable
      const std::string verb = "INSERT";
      std::string fields_str;

      for (auto& f : fields) {
        if (fields_str.length() > 0) fields_str += ", ";
        fields_str += "`" + f + "`";
      }

      const size_t record_sz = std::tuple_size<value_type>::value;
      for (size_t i = 0; i < record_sz; ++i) {
        if (values_placeholders_str_.length() > 0) values_placeholders_str_ += ", ";
        values_placeholders_str_ += "?";
      }

      values_placeholders_str_ = "SELECT " + values_placeholders_str_;

      query_prefix_str_ = verb + " INTO `" + table_name + "` (" + fields_str + ") ";
    }

    ~buffered_insert_query_base() {
      SQLITE_HPP_LOG("Destructing");
      flush();
    }

    buffered_insert_query_base(const type& other) :
      query_base<value_access_policy_t>::query_base(other),
      max_sql_length_(other.max_sql_length_),
      max_compound_select_(other.max_compound_select_),
      max_variable_number_(other.max_variable_number_),
      query_prefix_str_(other.query_prefix_str_),
      values_placeholders_str_(other.values_placeholders_str_),
      buf_(other.buf_) {
    }

    buffered_insert_query_base(type&& other) :
      query_base<value_access_policy_t>::query_base(other),
      max_sql_length_(std::move(other.max_sql_length_)),
      max_compound_select_(std::move(other.max_compound_select_)),
      max_variable_number_(std::move(other.max_variable_number_)),
      query_prefix_str_(std::move(other.query_prefix_str_)),
      values_placeholders_str_(std::move(other.values_placeholders_str_)),
      buf_(other.buf_) {
    }

    void swap(type& other) {
      query_base<value_access_policy_t>::swap(other);
      std::swap(max_sql_length_, other.max_sql_length_);
      std::swap(max_compound_select_, other.max_compound_select_);
      std::swap(max_variable_number_, other.max_variable_number_);
      std::swap(query_prefix_str_, other.query_prefix_str_);
      std::swap(values_placeholders_str_, other.values_placeholders_str_);
      std::swap(buf_, other.buf_);
    }

    type& operator=(const type& other) {
      type tmp(other);
      swap(tmp);
      return *this;
    }

    std::back_insert_iterator<type> begin() {
      return std::back_insert_iterator<type>(this);
    }

    void push_back(const value_type& r) {
      const size_t record_sz = std::tuple_size<value_type>::value;
      if ((this->result_code_ == SQLITE_OK) || (this->result_code_ == SQLITE_DONE)) {
        SQLITE_HPP_LOG(std::string("buffered_insert_query::push_back Estimated query size + delta: buf_.size() = ") + std::to_string(buf_.size()) +
            " (max = " + std::to_string(max_compound_select_) +
            " ), query length = " + std::to_string((query_prefix_str_.length() + ((buf_.size() + 1) * (values_placeholders_str_.length() + record_separator_str_.length())))) +
            " (max = " + std::to_string(max_sql_length_) + "), " +
            " variable number = " + std::to_string((buf_.size() + 1) * record_sz) +
            " (max = " + std::to_string(max_variable_number_) + ")");
          
        if ((buf_.size() + 1 >= max_compound_select_) ||
            (query_prefix_str_.length() + ((buf_.size() + 1) * (values_placeholders_str_.length() + record_separator_str_.length()))
             >= max_sql_length_) ||
            (((buf_.size() + 1) * record_sz) >= max_variable_number_))
          {
          flush();
        }
        buf_.push_back(r);
      }
    }

    void push_back_variadic(Rs... values) {
      value_type r = std::make_tuple<value_type>(std::forward<Rs...>(values...));
      push_back(r);
    }

    void push_back(Rs... values) {
      push_back_variadic(std::forward<Rs...>(values...));
    }
    
    void flush() {
      if (this->result_code_ == SQLITE_DONE) this->result_code_ = SQLITE_OK;
      if (buf_.size() > 0) {
        std::string query_affix_str;
        for (size_t i = 0; i < buf_.size(); ++i) {
          if (query_affix_str.length() > 0) query_affix_str += record_separator_str_;
          query_affix_str += values_placeholders_str_;
        }
        this->query_str_ = query_prefix_str_ + query_affix_str;
        SQLITE_HPP_LOG(std::string("buffered_insert_query::flush Query string: ") + this->query_str_);
        this->prepare();
        SQLITE_HPP_LOG("buffered_insert_query::flush Prepare called.");
        if (this->result_code_ != SQLITE_OK) return;
        SQLITE_HPP_LOG("buffered_insert_query::flush Prepare result ok.");
        
        const size_t record_sz = std::tuple_size<value_type>::value;
        int idx = 1;
        for (value_type &r : buf_) {
          ::sqlite::query_base<value_access_policy_t>::bind_tuple(idx, r);
          if (this->result_code_ != SQLITE_OK) return;
          idx += record_sz;
        };
        SQLITE_HPP_LOG("buffered_insert_query::flush Bind tuples ok.");

        this->step();
        if (this->result_code_ == SQLITE_DONE) {
          buf_.clear();
        }
        SQLITE_HPP_LOG(std::string("buffered_insert_query::flush Step result = ") + std::to_string(this->result_code_));
      }
    }

  private:
    buffered_insert_query_base(const database::type_ptr& db, const std::string& query_str) = delete;

    int max_compound_select_;
    int max_sql_length_;
    int max_variable_number_;
    std::string query_prefix_str_;
    std::string values_placeholders_str_;
    const std::string record_separator_str_ = "\nUNION ALL ";
    std::vector<value_type> buf_;

  };

  template <typename... Rs>
  class buffered_insert_query : public buffered_insert_query_base<default_value_access_policy, Rs...> {
  public:
    using buffered_insert_query_base<default_value_access_policy, Rs...>::buffered_insert_query_base;
  };
}

