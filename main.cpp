#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <ffts/ffts.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using std::cout;
using std::endl;
using std::string;
using ClockT = std::chrono::steady_clock;
using std::ifstream;
using std::runtime_error;

#include "AudioProcess.h"
#include "LinuxAudioStream.h"
#include "MFileWatcher.h"
#include "Renderer.h"
#include "ShaderConfig.h"
#include "ShaderPrograms.h"
#include "Window.h"
#include "filesystem.h"

using AudioStreamT = LinuxAudioStream;
using AudioProcessT = AudioProcess<ClockT, AudioStreamT>;

const std::string version_string = "muviz 1.2.0";
const std::string default_visualizer = "spikestar";

int main(int argc, char *argv[]) {
  filesys::path shader_folder(SHADERDIR);
  filesys::path shader_config_folder(SHADERDIR);

  if ((argc > 1) && string(argv[1]) == "-l") {
    // list the available shader directories in shader_folder
    for (const auto &entry :
         std::filesystem::directory_iterator(shader_folder.string())) {
      if (entry.is_directory()) {
        const std::string s = entry.path();
        const auto slashpos = s.rfind("/");
        if (slashpos == -1) {
          cout << s << endl;
        } else {
          cout << s.substr(slashpos + 1, s.length()) << endl;
        }
      }
    }
    return 0;
  } else if ((argc > 1) && string(argv[1]) == "--help") {
    cout << "muviz [VISUALIZER]" << endl << endl;
    cout << "Flags:" << endl;
    cout << "  -l            list available visualizers" << endl;
    cout << "  --help        display this help" << endl;
    cout << "  --version     display the current version" << endl;
    return 0;
  } else if ((argc > 1) && string(argv[1]) == "--version") {
    cout << version_string << endl;
    return 0;
  } else if (argc > 1) {
    shader_folder = shader_folder / string(argv[1]);
    shader_config_folder = shader_config_folder / string(argv[1]);
  } else {
    shader_folder = shader_folder / default_visualizer;
    shader_config_folder = shader_config_folder / default_visualizer;
  }

  filesys::path shader_config_path = shader_config_folder / "shader.json";

  FileWatcher watcher(shader_folder);

  ShaderConfig *shader_config = nullptr;
  ShaderPrograms *shader_programs = nullptr;
  Renderer *renderer = nullptr;
  Window *window = nullptr;
  // TODO extract to get_valid_config(&, &, &, &)
  while (!(shader_config && shader_programs && window)) {
    try {
      shader_config = new ShaderConfig(shader_folder, shader_config_path);
      window = new Window(shader_config->mInitWinSize.width,
                          shader_config->mInitWinSize.height);
      renderer = new Renderer(*shader_config, *window);
      shader_programs =
          new ShaderPrograms(*shader_config, *renderer, *window, shader_folder);
      renderer->set_programs(shader_programs);
    } catch (runtime_error &msg) {
      cout << msg.what() << endl;

      // something failed so reset state
      delete shader_config;
      delete shader_programs;
      delete window;
      delete renderer;
      shader_config = nullptr;
      shader_programs = nullptr;
      window = nullptr;
      renderer = nullptr;

      while (!watcher.files_changed()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  }
  cout << "Successfully compiled shaders." << endl;

  // AudioStreamT audio_stream(); // Most Vexing Parse
  AudioStreamT audio_stream;
  AudioProcessT audio_process{audio_stream, shader_config->mAudio_ops};
  std::thread audio_thread =
      std::thread(&AudioProcess<ClockT, AudioStreamT>::start, &audio_process);
  if (shader_config->mAudio_enabled)
    audio_process.start_audio_system();

  auto update_shader = [&]() {
    cout << "Updating shaders." << endl;
    try {
      ShaderConfig new_shader_config(shader_folder, shader_config_path);
      Renderer new_renderer(new_shader_config, *window);
      ShaderPrograms new_shader_programs(new_shader_config, new_renderer,
                                         *window, shader_folder);
      *shader_config = new_shader_config;
      *shader_programs = std::move(new_shader_programs);
      *renderer = std::move(new_renderer);
      renderer->set_programs(shader_programs);
    } catch (runtime_error &msg) {
      cout << msg.what() << endl;
      cout << "Failed to update shaders." << endl << endl;
      return;
    }
    if (shader_config->mAudio_enabled) {
      audio_process.start_audio_system();
      audio_process.set_audio_options(shader_config->mAudio_ops);
    } else {
      audio_process.pause_audio_system();
    }
    cout << "Successfully updated shaders." << endl << endl;
  };

  while (window->is_alive()) {
    if (watcher.files_changed())
      update_shader();
    auto now = ClockT::now();
    renderer->update(audio_process.get_audio_data());
    renderer->render();
    window->swap_buffers();
    window->poll_events();
    std::this_thread::sleep_for(std::chrono::microseconds(16666) -
                                (ClockT::now() - now));
  }

  audio_process.exit_audio_system();
  audio_thread.join();

  return 0;
}
