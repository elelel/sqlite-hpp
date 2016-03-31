#pragma once

// Compile-time integer list
// Origin: http://stackoverflow.com/a/8572595

namespace sqlite {
  template <size_t... n>
  struct ct_integer_list {
    template <size_t m>
    struct push_back {
      typedef ct_integer_list<n..., m> type;
    };
  };

  template <size_t max>
  struct ct_iota_1 {
    typedef typename ct_iota_1<max-1>::type::template push_back<max>::type type;
  };

  template <>
  struct ct_iota_1<0> {
    typedef ct_integer_list<> type;
    
  };
               
}
