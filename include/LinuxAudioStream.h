#pragma once

#include <condition_variable>
#include <mutex>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include "AudioStream.h"

class LinuxAudioStream : public AudioStream {
public:
  LinuxAudioStream();
  ~LinuxAudioStream();
  void get_next_pcm(float *buff_l, float *buff_r, int size);
  int get_sample_rate();
  int get_max_buff_size();

private:
  static void on_process(void *userdata);
  void store(const float *interleaved, int frames);

  static const int sample_rate = 48000;
  static const int max_buff_size = 512;
  static const int channels = 2;
  // Holds ~0.17s of audio, enough to absorb scheduling jitter.
  static const int ring_size = 8192;

  std::mutex mtx;
  std::condition_variable cv;
  float ring_l[ring_size] = {};
  float ring_r[ring_size] = {};
  int ring_read = 0;
  int ring_write = 0;
  int ring_avail = 0;

  pw_thread_loop *loop = nullptr;
  pw_stream *stream = nullptr;
};
