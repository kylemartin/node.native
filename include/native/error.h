#ifndef __ERROR_H__
#define __ERROR_H__

#include "base.h"
#include "detail.h"

namespace native
{
    class Exception : public std::exception
    {
    public:
        Exception(const std::string& message=std::string())
            : message_(message)
        {}

        Exception(detail::resval rv)
            : message_(rv.str())
        {}

        Exception(detail::resval rv, const std::string& message)
            : message_(message + "\n" + rv.str())
        {}

        Exception(std::nullptr_t)
            : message_()
        {}

        virtual ~Exception() noexcept
        {}

        virtual const char* what() const noexcept { return message_.c_str(); }

        const std::string& message() const { return message_; }

    private:
        std::string message_;
    };
}

#endif
