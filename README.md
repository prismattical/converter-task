# Homework task

## Problem

Create a command line utility that will accept a media file at the input:
<https://download.samplelib.com/mp4/sample-5s.mp4>
Which should take the video stream and convert it to the AV1 codec, convert the audio to the Opus codec, mux them together and save them in an MKV container.
For implementation, you can use the FFMpeg or GStreamer libraries of your choice.
Need use CMake build system.
Write README.txt file with command line interface.

Preferred operating system: Linux

Coding conventions <https://gcc.gnu.org/codingconventions.html>

## Compilation and usage

Dependencies: CMake, gstreamer library and plugins for it

```bash
cmake -S . -B "build"
cmake --build build
```

Usage: converter input_url output_file
