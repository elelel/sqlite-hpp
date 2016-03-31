#pragma once

#include "ct_integer_list.hpp"

namespace sqlite {
  template <typename... x>
  struct tuple_tail_type;

  template <typename head_t>
  struct tuple_tail_type<std::tuple<head_t>> {
    typedef std::tuple<> type;
  };
    
  template <typename head_t, typename... tail_t>
  struct tuple_tail_type<std::tuple<head_t, tail_t...>> {
    typedef std::tuple<tail_t...> type;
  };

  template <size_t... indices, typename Tuple>
  auto tuple_subset(const Tuple& tpl, ct_integer_list<indices...>)
    -> decltype(std::make_tuple(std::get<indices>(tpl)...))
  {
    return std::make_tuple(std::get<indices>(tpl)...);
  }

  template <typename Head, typename... Tail>
  std::tuple<Tail...> tuple_tail(const std::tuple<Head, Tail...>& tpl)
  {
    return tuple_subset(tpl, typename ct_iota_1<sizeof...(Tail)>::type());
  } 
  
}
