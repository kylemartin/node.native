
#include "native/detail/node.h"

namespace native
{
    namespace detail
    {
            node& node::instance()
            {
                // TODO: (C++11) Is it really true that this is thread-safe and single instance is guaranteed across the modules?
                static node instance_;
                return instance_;
            }

            int node::start(std::function<void()> logic)
            {
                assert(logic);

                // initialize
                init();

                // run main logic
                logic();

                // start event loop ;D
                return uv_run(uv_default_loop());
            }

            void node::add_tick_callback(std::function<void()> callback)
            {
                tick_callbacks_.push_back(callback);
                enable_tick();
            }

            void node::init()
            {
                uv_uptime(&start_time_);

                uv_prepare_init(uv_default_loop(), &prepare_);
                prepare_.data = this;
                uv_prepare_start(&prepare_, [](uv_prepare_t* handle, int status) {
                    auto self = reinterpret_cast<node*>(handle->data);
                    assert(self && &self->prepare_ == handle);
                    assert(status == 0);

                    self->tick();
                });
                uv_unref(reinterpret_cast<uv_handle_t*>(&prepare_));

                uv_check_init(uv_default_loop(), &check_);
                check_.data = this;
                uv_check_start(&check_, [](uv_check_t* handle, int status) {
                    auto self = reinterpret_cast<node*>(handle->data);
                    assert(self && &self->check_ == handle);
                    assert(status == 0);

                    self->tick();
                });
                uv_unref(reinterpret_cast<uv_handle_t*>(&check_));

                uv_idle_init(uv_default_loop(), &idle_);
                uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));
                idle_.data = this;
            }

            void node::tick()
            {
                if(!need_tick_) return;
                need_tick_ = false;

                if(uv_is_active(reinterpret_cast<uv_handle_t*>(&idle_)))
                {
                    uv_idle_stop(&idle_);
                    uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));
                }

                auto fail_it = tick_callbacks_.end();
                for(auto it=tick_callbacks_.begin(); it!=tick_callbacks_.end(); ++it)
                {
                    try { (*it)(); }
                    catch(...)
                    {
                        // TODO: handle exception in a tick callback
                        // ...

                        // stop here and the remaining callbacks will be executed in the next tick.
                        fail_it = ++it;
                        break;
                    }
                }
                tick_callbacks_.erase(tick_callbacks_.begin(), fail_it);
            }

            void node::enable_tick()
            {
                need_tick_ = true;
                if(!uv_is_active(reinterpret_cast<uv_handle_t*>(&idle_)))
                {
                    uv_idle_start(&idle_, [](uv_idle_t* handle, int status) {
                        auto self = reinterpret_cast<node*>(handle->data);
                        assert(self);
                        assert(self && &self->idle_ == handle);
                        assert(status == 0);

                        self->tick();
                    });
                    uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));
                }
            }

    }
}
