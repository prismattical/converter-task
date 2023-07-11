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

```bash
cmake -S . -B "build"
cmake --build build
```

Usage: converter input_url output_file

## Notes

If you are reading this it means that I couldn't make AV1 codec work. Currently there is x264 codec. When I use x264 it works fast and outputs small-sized file with ~2k bitrate. When I don't use any codec, it outputs raw data with ~700k bitrate and huge 500Mb file (originial file is 2Mb). But when I use AV1, it takes minutes for it to slowly build up big file and then it crashes without any result. It seems that I just miss some small codec/GStreamer option that can fix this issue.
