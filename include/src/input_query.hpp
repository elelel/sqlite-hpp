#pragma once

#include <sqlite3.h>

#include <tuple>

#include "logging.hpp"
#include "query.hpp"

namespace sqlite {
  template <typename record_tuple_t,
            typename value_access_policy_t>
  class input_query_base;
  template <typename record_tuple_t,
            typename value_access_policy_t>
  class input_query_iterator;
  template <typename... Rs>
  class input_query;

  template <typename record_tuple_t,
            typename value_access_policy_t>
  class input_query_base : public query_base<value_access_policy_t> {
    friend class input_query_iterator<record_tuple_t, value_access_policy_t>;
  public:
    typedef input_query_base<record_tuple_t,
                             value_access_policy_t> type;
    typedef std::shared_ptr<type> type_ptr;
    typedef input_query_iterator<record_tuple_t, value_access_policy_t> iterator;

    using query_base<value_access_policy_t>::query_base;

    iterator begin() {
      this->step();
      return iterator(type_ptr(this, [] (type *) {}), false);
    }

    iterator end() {
      return iterator(type_ptr(this, [] (type *) {}), true);
    }
  };

  template <typename record_tuple_t, typename value_access_policy_t>
  class input_query_iterator :
    public std::iterator<std::input_iterator_tag, input_query<record_tuple_t, value_access_policy_t>>,
    public result_code_container {
  public:
    typedef input_query_iterator<record_tuple_t, value_access_policy_t> type;
    typedef std::shared_ptr<type> type_ptr;
    typedef input_query_base<record_tuple_t, value_access_policy_t> query_type;
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
    }

    void operator++(int) {
      q_->step();
      ++pos_;
      if (q_->result_code() != SQLITE_ROW) end_ = true;
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
      
    template <typename A>
    std::tuple<A> get_values(const int i) {
      A value;
      q_->get(i, value);
      if (q_->result_code_ != SQLITE_OK) {
        result_code_ = q_->result_code_;
      }
      return std::tuple<A>(value);
    }
    
    template <typename A, typename B, typename... Zs>
    std::tuple<A, B, Zs...> get_values(const int i) {
      A value;
      q_->get(i, value);
      if (q_->result_code_ != SQLITE_OK) {
        result_code_ = q_->result_code_;
      }
      return std::tuple_cat(std::tuple<A>(value), get_values<B, Zs...>(i + 1));
    }
    
  };

  template <typename... Rs>
  class input_query : public input_query_base<std::tuple<Rs...>, default_value_access_policy> {
    using input_query_base<std::tuple<Rs...>, default_value_access_policy>::input_query_base;
  };
}
