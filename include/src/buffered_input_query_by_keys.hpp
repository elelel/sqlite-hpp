#pragma once

#include <algorithm>
#include <locale>
#include <memory>
#include <set>
#include <tuple>

#include "logging.hpp"
#include "query.hpp"

namespace sqlite {
  namespace buffered {
    template <typename buffered_input_query_t,
              typename record_tuple_t,
              typename value_access_policy_t>
    class input_query_iterator ;
    
    template <typename record_tuple_t,
              typename key_tuple_t,
              typename value_access_policy_t>
    class input_query_by_keys_base;

    template <typename record_tuple_t,
              typename key_tuple_t,
              typename value_access_policy_t>
    class input_query_by_keys_base : public query_base<value_access_policy_t> {
      friend class input_query_iterator<
        input_query_by_keys_base<record_tuple_t, key_tuple_t, value_access_policy_t>,
        record_tuple_t, value_access_policy_t>;

    public:
      typedef input_query_by_keys_base<record_tuple_t,
                                           key_tuple_t,
                                           value_access_policy_t> type;
      typedef std::shared_ptr<type> type_ptr;
      typedef key_tuple_t key_tuple_type;
      typedef record_tuple_t record_tuple_type;
      typedef input_query_iterator<type, record_tuple_type, value_access_policy_t> iterator;

      template <typename key_fields_container_t>
      input_query_by_keys_base(const database::type_ptr& db,
                                   const std::string& query_prefix_str,
                                   const key_fields_container_t& key_fields,
                                   const std::string& query_postfix_str = "",
                                   const int key_parameters_offset = 0) :
        query_base<value_access_policy_t>(db),
        query_prefix_str_(query_prefix_str),
        query_postfix_str_(query_postfix_str),
        key_parameters_offset_(key_parameters_offset),
        max_sql_length_(db->sqlite_max_sql_length()),
        max_compound_select_(db->sqlite_max_compound_select()),
        max_variable_number_(db->sqlite_max_variable_number())
      {
        for (const auto &f : key_fields) {
          if (values_placeholders_str_.length() > 0) values_placeholders_str_ += field_separator_str_;
          values_placeholders_str_ += "`" + f + "` = ?";
        }
        values_placeholders_str_ = "(" + values_placeholders_str_ + ")";
      }

      input_query_by_keys_base(const type& other) :
        query_base<value_access_policy_t>(other),
        query_prefix_str_(other.query_prefix_str_),
        query_postfix_str_(other.query_postfix_str_),
        key_parameters_offset_(other.key_parameters_offset_),
        max_sql_length_(other.max_sql_length_),
        max_compound_select_(other.max_compound_select_),
        max_variable_number_(other.max_variable_number_) {
      }

      input_query_by_keys_base(type&& other) :
        query_base<value_access_policy_t>(std::move(other)),
        query_prefix_str_(std::move(other.query_prefix_str_)),
        query_postfix_str_(std::move(other.query_postfix_str_)),
        key_parameters_offset_(std::move(other.key_parameters_offset_)),
        max_sql_length_(std::move(other.max_sql_length_)),
        max_compound_select_(std::move(other.max_compound_select_)),
        max_variable_number_(std::move(other.max_variable_number_)) {
      }

      void swap(type& other) {
        query_base<value_access_policy_t>::swap(other);
        std::swap(query_prefix_str_, other.query_prefix_str_);
        std::swap(query_postfix_str_, other.query_postfix_str_);
        std::swap(key_parameters_offset_, other.key_parameters_offset_);
        std::swap(max_sql_length_, other.max_sql_length_);
        std::swap(max_compound_select_, other.max_compound_select_);
        std::swap(max_variable_number_, other.max_variable_number_);
      }

      type& operator=(const type& other) {
        type tmp(other);
        swap(tmp);
        return *this;
      }

      iterator begin() {
        pull();
        step();
        if (this->result_code_ == SQLITE_ROW) {
          return iterator(type_ptr(this, [] (type *) {}), false);
        } else {
          return iterator(type_ptr(this, [] (type *) {}), true);
        }
      }

      iterator end() {
        return iterator(type_ptr(this, [] (type *) {}), true);
      }
                              
      void add_key(const key_tuple_type& key) {
        keys_buf_.insert(key);
      }

      void step() {
        query_base<value_access_policy_t>::step();
        if ((this->result_code_ == SQLITE_DONE) &&
            (keys_buf_.size() > 0)) {
          pull();
          query_base<value_access_policy_t>::step();
        }
      }

      void pull() {
        if (keys_buf_.size() == 0) {
          this->result_code_ = SQLITE_DONE;
          return;
        }
        this->query_str_ = "";
        int records_to_add = 0;
        const size_t record_sz = std::tuple_size<key_tuple_type>::value;
        while (records_to_add <= keys_buf_.size()) {
          const size_t estimated_query_len =
            query_prefix_str_.length() +
            (records_to_add + 1) * ((values_placeholders_str_.length() + rec_separator_str_.length())) +
            query_postfix_str_.length();
          const size_t estimated_var_count = key_parameters_offset_ + ((records_to_add + 1) * record_sz);
          SQLITE_HPP_LOG(std::string("input_query_by_keys_base::pull Current keys_buf_.size() = ") + std::to_string(keys_buf_.size()) +
                         ", key parameters offset = " + std::to_string(key_parameters_offset_) +
                         ". Estimated: query length = " + std::to_string(estimated_query_len) + 
                         " (max = " + std::to_string(max_sql_length_) + "), " +
                         " variable number = " + std::to_string(estimated_var_count) +
                         " (max = " + std::to_string(max_variable_number_) + ")");
          if ((estimated_query_len >= max_sql_length_) ||
              (estimated_var_count >= max_variable_number_)) {
            SQLITE_HPP_LOG("input_query_by_keys_base::pull Limits reached");
            break;
          } else {
            if (this->query_str_.length() > 0) this->query_str_ += rec_separator_str_;
            this->query_str_ += values_placeholders_str_;
          }
          ++records_to_add;
        }
        this->query_str_ = query_prefix_str_ + this->query_str_;
        SQLITE_HPP_LOG(std::string("input_query_by_keys_vase:pull query ") + this->query_str_);
          
        this->prepare();
        if (this->result_code_ != SQLITE_OK) return;
        SQLITE_HPP_LOG(std::string("input_query_by_keys_vase:pull prepare ok"));
        int idx = 1 + key_parameters_offset_;
        while ((keys_buf_.size() > 0) && (records_to_add > 0)) {
          ::sqlite::query_base<value_access_policy_t>::bind_tuple(idx, *keys_buf_.begin());
          if (this->result_code_ != SQLITE_OK) return;
          keys_buf_.erase(keys_buf_.begin());
          idx += record_sz;
        }
        SQLITE_HPP_LOG(std::string("input_query_by_keys_vase:pull bind tuples ok"));
      }
    
    private:
      input_query_by_keys_base(const database::type_ptr& db, const std::string& query_str) = delete;

      std::string query_prefix_str_;
      std::string query_postfix_str_;
      std::string values_placeholders_str_;
      std::string field_separator_str_ = " AND ";
      std::string rec_separator_str_ = " OR ";
      int key_parameters_offset_;
      int max_compound_select_;
      int max_sql_length_;
      int max_variable_number_;
      std::set<key_tuple_type> keys_buf_;
      
    };

    template <typename buffered_input_query_t,
              typename record_tuple_t,
              typename value_access_policy_t>
    class input_query_iterator :
      public std::iterator<std::input_iterator_tag, input_query<record_tuple_t, value_access_policy_t>>,
      public result_code_container {
    public:
      typedef input_query_iterator<buffered_input_query_t,
                                            record_tuple_t, value_access_policy_t> type;
      typedef std::shared_ptr<type> type_ptr;
      typedef buffered_input_query_t query_type;
      typedef record_tuple_t record_tuple_type;
      typedef record_tuple_t value_type;

      input_query_iterator(const typename query_type::type_ptr& q) :
        result_code_container(), 
        q_(q),
        end_(false),
        pos_(0) {
      }

      input_query_iterator(const typename query_type::type_ptr& q, bool end) :
        result_code_container(),
        q_(q),
        end_(end),
        pos_(0) {
      }

      input_query_iterator(const type& other) :
        result_code_container(other),
        q_(other.q_),
        end_(other.end_),
        pos_(other.pos_) {
      }

      input_query_iterator(type&& other) :
        result_code_container(other),
        q_(std::move(other.q_)),
        end_(std::move(other.end_)),
        pos_(std::move(other.pos_)) {
      }

      void swap(input_query_iterator &other) {
        typename query_type::type_ptr tmp_q(other.q_);
      }
    
      type operator=(const input_query_iterator &other) {
        return type(other);
      }

      type& operator++() {
        q_->step();
        ++pos_;
        if (q_->result_code() != SQLITE_ROW) end_ = true;
        return *this;
      }

      bool operator==(const type& other) const {
        if (end_) {
          return (q_ == other.q_) && (end_ == other.end_);
        } else {
          return (q_ == other.q_) && (end_ == other.end_) &&
            (pos_ == other.pos_) && (q_->result_code() == other.q_->result_code());
        }
      }

      bool operator!=(const type& other) const {
        return !(*this == other);
      }

      record_tuple_type operator*() {
        record_tuple_type* dummy_ptr(nullptr);
        return q_->get_tuple(dummy_ptr);
      }

    private:
      typename query_type::type_ptr q_;
      bool end_;
      size_t pos_;
          
    };

  }
}
