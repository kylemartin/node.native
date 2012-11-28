#include <iostream>

#include "native/base.h"
#include "native/detail/buffers.h"

namespace native
{
  namespace detail {

  Buffer::Buffer()
    : data_()
    , encoding_(detail::et_utf8)
  {}

  Buffer::Buffer(const Buffer& c)
    : data_(c.data_)
    , encoding_(c.encoding_)
  {}

  Buffer::Buffer(const std::vector<char>& data)
    : data_(data)
    , encoding_(detail::et_utf8)
  {}

  Buffer::Buffer(const std::string& str)
    : data_(str.begin(), str.end())
    , encoding_(detail::et_ascii)
  {}

  Buffer::Buffer(const std::string& str, const detail::encoding_type& encoding)
    : data_(str.begin(), str.end())
    , encoding_(encoding)
  {
    //TODO: initialize Buffer according to argument encoding
  }

  Buffer::Buffer(const char* data, std::size_t length)
    : data_(data, data + length)
    , encoding_(detail::et_utf8)
  {}

  Buffer::Buffer(Buffer&& c)
    : data_(std::move(c.data_))
    , encoding_(c.encoding_)
  {}

  Buffer::Buffer(std::nullptr_t)
    : data_()
    , encoding_(detail::et_none)
  {}

  Buffer::~Buffer()
  {}

  char& Buffer::operator[](std::size_t idx) { return data_[idx]; }
  const char& Buffer::operator[](std::size_t idx) const { return data_[idx]; }

  Buffer& Buffer::operator =(const Buffer& c)
  {
    data_ = c.data_;
    encoding_ = c.encoding_;
    return *this;
  }

  Buffer& Buffer::operator =(Buffer&& c)
  {
    data_ = std::move(c.data_);
    encoding_ = c.encoding_;
    return *this;
  }

  char* Buffer::base() { return &data_[0]; }
  const char* Buffer::base() const { return &data_[0]; }

  std::size_t Buffer::size() const { return data_.size(); }

  Buffer Buffer::slice(std::size_t start, std::size_t end)
  {
    return Buffer(std::vector<char>(data_.begin()+start, data_.begin()+end));
  }

  void Buffer::append(const Buffer& buf) {
    data_.insert(data_.end(), buf.data_.begin(), buf.data_.end());
  }

  std::string Buffer::str() const { return std::string(base(), size()); }

  std::ostream& operator<<(std::ostream& os, const Buffer& buf)
  {
      os << buf.str();
      return os;
  }

  }  // namespace detail
} // namespace native
