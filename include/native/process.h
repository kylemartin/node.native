#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "base.h"
#include "detail.h"
#include "events.h"

namespace native
{
    class process : public EventEmitter
    {
    public:
        static void nextTick(std::function<void()> callback);

        /**
         *  Starts the main loop with the callback.
         *
         *  @param callback     Callback object that is executed by node.native.
         *
         *  @return             This function always returns 0.
         */
        static int run(std::function<void()> callback);

    private:
        process();

        process(const process&) = delete;
        void operator =(const process&) = delete;

        virtual ~process() {}

    public:
        static process& instance();

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

        void prepareStdio();


/* Signal Events **************************************************************/
    private:
        std::map<int, std::shared_ptr<detail::signal_watcher>> watchers_;
    public:
        template <int SIGNAL>
        struct signal_event : public util::callback_def<int> {
          enum { value = SIGNAL };
        };

        using EventEmitter::listener_t;

        template<int N>
        static void on_signal(int signum) {
          instance().emit<signal_event<N>>(N);
        }

        /**
         *  Adds a listener for the event.
         *
         *  @param callback     Callback object.
         *
         *  @return             A unique value for the listener. This value is used as a parameter for removeListener() function.
         *                      Even if you add the same callback object, this return value differ for each calls.
         *                      If EventEmitter fails to add the listener, it returns nullptr.
         */
        template<int N>
        listener_t addSignal_(const typename signal_event<N>::callback_type& callback, bool once)
        {
            auto s = events_.find(typeid(signal_event<N>).hash_code());
            if(s == events_.end())
            { // Could not find event_emitter for signal_event
              auto res = events_.insert(std::make_pair(typeid(signal_event<N>).hash_code(), detail::event_emitter()));
              assert(res.second);
              s = res.first;
              // create signal watcher if necessary
              if (watchers_.count(N) <= 0) {
                auto res = watchers_.insert(
                    std::make_pair(N, std::shared_ptr<detail::signal_watcher>(
                        new detail::signal_watcher()
                )));
                assert(res.second);
                std::shared_ptr<detail::signal_watcher> watcher = res.first->second;
                watcher->unref();
                watcher->callback(on_signal<N>);
                watcher->start(N);
              }
            }
            return once
                ? s->second.add<typename signal_event<N>::callback_type>(callback, true)
                : s->second.add<typename signal_event<N>::callback_type>(callback);
        }

        template<int N>
        listener_t addSignal(const typename signal_event<N>::callback_type& callback) {
          return addSignal_<signal_event<N>>(callback, false);
        }
        /**
         *  Adds a listener for the event.
         *
         *  @param callback     Callback object.
         *
         *  @return             A unique value for the listener. This value is used as a parameter for removeListener() function.
         *                      Even if you add the same callback object, this return value differ for each calls.
         *                      If EventEmitter fails to add the listener, it returns nullptr.
         */
        template<int N>
        listener_t onSignal(const typename signal_event<N>::callback_type& callback)
        {
            return addSignal_<N>(callback, false);
        }

        /**
         *  Adds a listener for the event. This listener is deleted after it is invoked first time.
         *
         *  @param callback     Callback object.
         *
         *  @return             A unique value for the listener. This value is used as a parameter for removeListener() function.
         *                      Even if you add the same callback object, this return value differ for each calls.
         *                      If EventEmitter fails to add the listener, it returns nullptr.
         */
        template<int N>
        listener_t onceSignal(const typename signal_event<N>::callback_type& callback)
        {
          return addSignal_<N>(callback, true);
        }

        /**
         *  Removes a listener for the event.
         *
         *  @param listener     A unique value for the listener. You must use the return value of addListener(), on(), or once() function.
         *
         *  @retval true       The listener was removed from EventEmitter.
         *  @retval false      The listener was not found in EventEmitter, or the event was not registered.
         */
        template<int N>
        bool removeListener(listener_t listener)
        {
            auto s = events_.find(typeid(signal_event<N>).hash_code());
            if(s != events_.end())
            {
                return s->second.remove(listener);
                if (s->second.count() <= 0) {
                  // remove signal watcher when no more event listeners
                  watchers_.erase(N);
                }
            }

            return false;
        }

        /**
         *  Removes all listeners for the event.
         *
         *  @retval true       All the listeners for the event were removed.
         *  @retval false      The event was not registered.
         */
        template<int N>
        bool removeAllListeners()
        {
            auto s = events_.find(typeid(signal_event<N>).hash_code());
            if(s != events_.end())
            {
                s->second.clear();
                return true;
                // remove signal watcher
                watchers_.erase(N);
            }

            return false;
        }

    };
}

#endif
