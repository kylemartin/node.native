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
		explicit Buffer(const std::vector<char>& data);
		explicit Buffer(const std::string& str);
		explicit Buffer(const std::string& str, const detail::encoding_type& encoding);
		explicit Buffer(const char* data, std::size_t length);
		explicit Buffer(std::nullptr_t);

		virtual ~Buffer();

		char& operator[](std::size_t idx);
		const char& operator[](std::size_t idx) const;

		Buffer& operator =(const Buffer& c);
		Buffer& operator =(Buffer&& c);

		char* base();
		const char* base() const;

		std::size_t size() const;

		Buffer slice(std::size_t start, std::size_t end);

		void append(const Buffer& c);

		std::string str() const;

	private:
		std::vector<char> data_;
		int encoding_;
	};

	std::ostream& operator<<(std::ostream& os, const Buffer& buf);

	}  // namespace detail
} // namespace native

#endif
