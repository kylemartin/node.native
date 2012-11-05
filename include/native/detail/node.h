#ifndef __DETAIL_NODE_H__
#define __DETAIL_NODE_H__

#include "base.h"

namespace native
{
    namespace detail
    {
        class node
        {
        private:
            node()
                : start_time_()
                , check_()
                , prepare_()
                , idle_()
                , need_tick_(false)
                , tick_callbacks_()
            {
            }

            node(const node&) = delete;
            void operator =(const node&) = delete;

            virtual ~node()
            {}

        public:
            static node& instance();

            int start(std::function<void()> logic);

            void add_tick_callback(std::function<void()> callback);

        private:
            void init();

            void tick();

            void enable_tick();

        private:
            double start_time_;

            uv_check_t check_;
            uv_prepare_t prepare_;
            uv_idle_t idle_;
            bool need_tick_;
            std::list<std::function<void()>> tick_callbacks_;
        };
    }
}

#endif

