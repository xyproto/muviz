# Music Visualizer

Visualize music and audio. Tested on Arch Linux under both X and Wayland.

The default visualizer in `muviz` is based on both one of the visualizers from `music_visualizer` and also a [shader that I wrote](https://www.shadertoy.com/view/3slSRN).

Screenshot of the default music visualizer (it's very light because it's meant to be used with a projector, like an effect, and not viewed directly on a monitor):

![Screenshot](img/muviz_screenshot.jpg)

## Fork information

This is a fork of [bradleybauer/music_visualizer](https://github.com/bradleybauer/music_visualizer) (GPL3 licensed).

It also contains files from:

* A fork of [bradleybauer/SimpleFileWatcher](https://github.com/bradleybauer/SimpleFileWatcher)
* Which is a fork of [shadowndacorner/SimpleFileWatcher](https://github.com/shadowndacorner/SimpleFileWatcher)
* Which is a fork of [apetrone/simplefilewatcher](https://github.com/apetrone/simplefilewatcher)

SimpleFileWatcher is MIT licensed.

## Command line options

Use `--help` or `--version` to display help or the current version. Use `-l` to list the available music visualizer. Supply one of the names as the first argument to `muviz` to use them.

## Build instructions

`muviz` can be built with [slay](https://github.com/xyproto/slay). Simply run:

    slay

## Installation instructions

Set `$pkgdir` to the root of where you want muviz to be installed.

    DESTDIR="$pkgdir" PREFIX=/usr slay install

## Package instructions

    slay pkg

## Dependencies

    ffts glfw libpipewire rapidjson

## Installation on Arch Linux

Just install `muviz` from AUR using your favorite AUR helper.

## General information

* Version: 1.2.0
* License: MIT and GPL3. See the [`LICENCE`](LICENSE) file for more information.
