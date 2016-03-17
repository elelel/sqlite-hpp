#pragma once

#include "query.hpp"

#include <sqlite3.h>

#include <tuple>

#include "logging.hpp"

namespace sqlite {
  template <typename value_access_policy_t, typename... Rs>
  class input_query_base;
  template <typename value_access_policy_t, typename... Rs>
  class input_query_iterator;
  template <typename... Rs>
  class input_query;

  template <typename value_access_policy_t, typename... Rs>
  class input_query_base : public query_base<value_access_policy_t> {
    friend class input_query_iterator<value_access_policy_t, Rs...>;
  public:
    typedef input_query_base<value_access_policy_t, Rs...> type;
    typedef std::shared_ptr<type> type_ptr;

    using query_base<value_access_policy_t>::query_base;

    input_query_iterator<value_access_policy_t, Rs...> begin() {
      this->step();
      return input_query_iterator<value_access_policy_t, Rs...>(type_ptr(this, [] (type *) {}));
    }

    input_query_iterator<value_access_policy_t, Rs...> end() {
      return input_query_iterator<value_access_policy_t, Rs...>(type_ptr(this, [] (type *) {}), true);
    }
  };

  template <typename value_access_policy_t, typename... Rs>
  class input_query_iterator :
    public std::iterator<std::input_iterator_tag, input_query<value_access_policy_t, Rs...>>,
    public result_code_container {
  public:
    typedef input_query_iterator<value_access_policy_t, Rs...> type;
    typedef std::shared_ptr<type> type_ptr;
    typedef input_query_base<value_access_policy_t, Rs...> query_type;

    input_query_iterator(const typename query_type::type_ptr& q) : q_(q), end_(false) {
    }

    input_query_iterator(const typename query_type::type_ptr& q, bool end) : q_(q), end_(end) {
    }

    input_query_iterator(const input_query_iterator& other) : q_(other.q_) {
    }

    void swap(input_query_iterator &other) {
      typename query_type::type_ptr tmp_q(other.q_);
    }
    
    type operator=(const input_query_iterator &other) {
      return type(other);
    }

    type& operator++() {
      q_->step();
      if (q_->result_code() != SQLITE_ROW) end_ = true;
    }

    void operator++(int) {
      q_->step();
      if (q_->result_code() != SQLITE_ROW) end_ = true;
    }

    bool operator==(const type& other) const {
      if (end_) {
        return (q_ == other.q_) && (end_ == other.end_);
      } else {
        return (q_ == other.q_) && (pos_ == other.pos_) &&
          (end_ == other.end_) && (q_->result_code() == other.q_->result_code());
      }
    }

    bool operator!=(const type& other) const {
      return !(*this == other);
    }

    std::tuple<Rs...> operator*() {
      return get_values<Rs...>(0);
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
    
    template <typename... Zs>
      std::tuple<Rs...> get_values() {
      return get_values<Rs...>(0);
    }

  };

  template <typename... Rs>
  class input_query : public input_query_base<default_value_access_policy, Rs...> {
    using input_query_base<default_value_access_policy, Rs...>::input_query_base;
  };
}
