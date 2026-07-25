#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <spa/utils/result.h>
using std::cout;
using std::endl;

#include "LinuxAudioStream.h"

LinuxAudioStream::LinuxAudioStream() {
  static const pw_stream_events stream_events = {
      .version = PW_VERSION_STREAM_EVENTS,
      .process = on_process,
  };

  pw_init(nullptr, nullptr);

  loop = pw_thread_loop_new("muviz-capture", nullptr);
  if (!loop) {
    cout << "Could not create a PipeWire loop." << endl;
    exit(EXIT_FAILURE);
  }

  char latency[32];
  snprintf(latency, sizeof(latency), "%d/%d", max_buff_size, sample_rate);

  // stream.capture.sink makes PipeWire connect us to the default sink's
  // monitor.
  pw_properties *props = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
      PW_KEY_MEDIA_ROLE, "Music", PW_KEY_STREAM_CAPTURE_SINK, "true",
      PW_KEY_NODE_NAME, "muviz", PW_KEY_APP_NAME, "Music Visualizer",
      PW_KEY_NODE_LATENCY, latency, nullptr);

  pw_thread_loop_lock(loop);

  stream =
      pw_stream_new_simple(pw_thread_loop_get_loop(loop), "Music Visualizer",
                           props, &stream_events, this);
  if (!stream) {
    pw_thread_loop_unlock(loop);
    cout << "Could not create a PipeWire stream." << endl;
    exit(EXIT_FAILURE);
  }

  uint8_t pod_buffer[1024];
  spa_pod_builder builder =
      SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
  spa_audio_info_raw info = {};
  info.format = SPA_AUDIO_FORMAT_F32;
  info.rate = sample_rate;
  info.channels = channels;
  info.position[0] = SPA_AUDIO_CHANNEL_FL;
  info.position[1] = SPA_AUDIO_CHANNEL_FR;
  const spa_pod *params[1] = {
      spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info)};

  // No PW_STREAM_FLAG_RT_PROCESS: on_process takes a lock, so it must not run
  // on the realtime data thread.
  const int res = pw_stream_connect(
      stream, PW_DIRECTION_INPUT, PW_ID_ANY,
      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                   PW_STREAM_FLAG_MAP_BUFFERS),
      params, 1);
  pw_thread_loop_unlock(loop);

  if (res < 0) {
    cout << "Could not connect to a PipeWire monitor source: "
         << spa_strerror(res)
         << ". To list your PipeWire sources run 'pw-cli ls Node'." << endl;
    exit(EXIT_FAILURE);
  }

  if (pw_thread_loop_start(loop) < 0) {
    cout << "Could not start the PipeWire loop." << endl;
    exit(EXIT_FAILURE);
  }
}

LinuxAudioStream::~LinuxAudioStream() {
  if (loop)
    pw_thread_loop_stop(loop);
  if (stream)
    pw_stream_destroy(stream);
  if (loop)
    pw_thread_loop_destroy(loop);
  pw_deinit();
}

void LinuxAudioStream::on_process(void *userdata) {
  LinuxAudioStream *self = static_cast<LinuxAudioStream *>(userdata);

  pw_buffer *b = pw_stream_dequeue_buffer(self->stream);
  if (!b)
    return;

  const spa_data &d = b->buffer->datas[0];
  if (d.data && d.chunk->size) {
    const float *samples = reinterpret_cast<const float *>(
        static_cast<const uint8_t *>(d.data) + d.chunk->offset);
    self->store(samples, d.chunk->size / (sizeof(float) * channels));
  }

  pw_stream_queue_buffer(self->stream, b);
}

void LinuxAudioStream::store(const float *interleaved, int frames) {
  std::lock_guard<std::mutex> lock(mtx);

  for (int i = 0; i < frames; ++i) {
    ring_l[ring_write] = interleaved[i * channels + 0];
    ring_r[ring_write] = interleaved[i * channels + 1];
    ring_write = (ring_write + 1) % ring_size;
  }

  ring_avail += frames;
  if (ring_avail > ring_size) {
    // Reader fell behind, drop the oldest frames.
    ring_read = ring_write;
    ring_avail = ring_size;
  }

  cv.notify_one();
}

void LinuxAudioStream::get_next_pcm(float *buff_l, float *buff_r, int size) {
  if (max_buff_size < size)
    cout << "get_next_pcm called with size > max_buff_size" << endl;

  std::unique_lock<std::mutex> lock(mtx);
  // A suspended monitor delivers nothing, so don't wait for it forever.
  if (!cv.wait_for(lock, std::chrono::milliseconds(200),
                   [&] { return ring_avail >= size; })) {
    std::fill(buff_l, buff_l + size, 0.f);
    std::fill(buff_r, buff_r + size, 0.f);
    return;
  }

  for (int i = 0; i < size; ++i) {
    buff_l[i] = ring_l[ring_read];
    buff_r[i] = ring_r[ring_read];
    ring_read = (ring_read + 1) % ring_size;
  }
  ring_avail -= size;
}

int LinuxAudioStream::get_sample_rate() { return sample_rate; }

int LinuxAudioStream::get_max_buff_size() { return max_buff_size; }
