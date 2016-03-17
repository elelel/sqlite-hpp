#pragma once

namespace sqlite {
  class result_code_container {
  public:
    result_code_container() :
      result_code_(SQLITE_OK) {}

    result_code_container(const result_code_container& other) :
      result_code_(other.result_code_) {
    }

    result_code_container(result_code_container&& other) :
      result_code_(std::move(other.result_code_)){
    }
    
    const int result_code() const {
      return result_code_;
    }
  protected:
    int result_code_;

  };
}
