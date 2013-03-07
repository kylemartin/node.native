#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#include <iostream>

#include "base.h"

namespace native
{
	namespace detail {

	// TODO: implement Buffer class
	class Buffer
	{
	public:
		Buffer();
		Buffer(const Buffer& c);
		Buffer(Buffer&& c);
		Buffer(const std::string& str);
		Buffer(const std::vector<char>& data);
		Buffer(const std::string& str, const detail::encoding_type& encoding);
		Buffer(const char* data, std::size_t length);
		Buffer(std::nullptr_t);

		virtual ~Buffer();

		char& operator[](std::size_t idx);
		const char& operator[](std::size_t idx) const;

		Buffer& operator =(const Buffer& c);
		Buffer& operator =(Buffer&& c);

		char* base();
		const char* base() const;

		std::size_t size() const;

		Buffer slice(std::size_t start, std::size_t end);

		Buffer& append(const Buffer& c);

		std::string str() const;

	private:
		std::vector<char> data_;
		int encoding_;
	};

	std::ostream& operator<<(std::ostream& os, const Buffer& buf);

	}  // namespace detail
} // namespace native

#endif
