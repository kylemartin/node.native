/*
 * stream.cc
 *
 *  Created on: Nov 25, 2012
 *      Author: kmartin
 */

#include "native/stream.h"

namespace native {
using detail::Buffer;

Stream::Stream(detail::stream* stream, bool readable, bool writable) :
    EventEmitter(), stream_(stream), readable_(readable), writable_(writable), is_stdio_(
        false), pipe_() {
  if (readable_)
    registerEvent<event::data>();
  if (readable_)
    registerEvent<event::end>();
  registerEvent<event::error>();
  registerEvent<event::close>();
  if (writable_)
    registerEvent<event::drain>();
  if (writable_)
    registerEvent<event::pipe>();
}

Stream::~Stream() {
  DBG("~Stream()");
}

bool Stream::readable() const {
  return readable_;
}
bool Stream::writable() const {
  return writable_;
}

// TODO: implement Stream::setEncoding() function.
 void Stream::setEncoding(const std::string& encoding) {
}

void Stream::pause() {
}
void Stream::resume() {
}

Stream* Stream::pipe(Stream* destination, const util::dict& options) {
  pipe_ = std::shared_ptr<pipe_context>(
      new pipe_context(this, destination, options));
  assert(pipe_);
  return destination;
}

Stream* Stream::pipe(Stream* destination) {
  return pipe(destination, util::dict());
}

void Stream::writable(bool b) {
  writable_ = b;
}
void Stream::readable(bool b) {
  readable_ = b;
}

Stream::pipe_context::pipe_context(Stream* source, Stream* destination,
    const util::dict& options) :
    source_(source), destination_(destination), did_on_end_(false), pipe_count_(
        0) {
  assert(source_ && destination_);

  // whenever data is available from source, copy it destination
  source_on_data_ = source_->on<event::data>([&](const Buffer& chunk) {
    if(destination_->writable())
    {
      // if the destination is full: pause source
      if(!destination_->write(chunk)) source_->pause();
    }
  });

  // when the destination becomes writable again, resume reading
  dest_on_drain_ = destination_->on<event::drain>([&]() {
    if(source_->readable()) source_->resume();
  });

  // assign "end", "close" event callback
  if (!destination_->is_stdio_ && !options.compare("end", "false")) {
    pipe_count_++;

    source_on_end_ = source_->on<event::end>([&]() {on_end();});
    source_on_close_ = source_->on<event::close>([&]() {on_close();});
  }

  source_on_error_ = source_->on<event::error>(
      [&](const Exception& exception) {on_error(exception);});
  dest_on_error_ = destination_->on<event::error>(
      [&](const Exception& exception) {on_error(exception);});

  source_on_end_clenaup_ = source_->on<event::end>([&]() {cleanup();});
  source_on_close_clenaup_ = source_->on<event::close>([&]() {cleanup();});

  dest_on_end_clenaup_ = destination_->on<event::end>([&]() {cleanup();});
  dest_on_close_clenaup_ = destination_->on<event::close>([&]() {cleanup();});

  destination_->emit<event::pipe>(source_);
}

void Stream::pipe_context::on_error(Exception exception) {
  cleanup();
}

void Stream::pipe_context::on_close() {
  if (did_on_end_)
    return;
  did_on_end_ = true;

  pipe_count_--;

  cleanup();

  if (pipe_count_ == 0)
    destination_->destroy();
}

void Stream::pipe_context::on_end() {
  if (did_on_end_)
    return;
  did_on_end_ = true;

  pipe_count_--;

  cleanup();

  if (pipe_count_ == 0)
    destination_->end();
}

void Stream::pipe_context::cleanup() {
  source_->removeListener<event::data>(source_on_data_);
  destination_->removeListener<event::drain>(dest_on_drain_);

  source_->removeListener<event::end>(source_on_end_);
  source_->removeListener<event::close>(source_on_close_);

  source_->removeListener<event::error>(source_on_error_);
  destination_->removeListener<event::error>(dest_on_error_);

  source_->removeListener<event::end>(source_on_end_clenaup_);
  source_->removeListener<event::close>(source_on_close_clenaup_);

  destination_->removeListener<event::end>(dest_on_end_clenaup_);
  destination_->removeListener<event::close>(dest_on_close_clenaup_);
}

} // namespace native
