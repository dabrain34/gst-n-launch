# gst-n-launch

## Install meson and ninja

Meson 0.48 or newer is required.
On Linux and macOS you can get meson through your package manager or using:

```
$ pip3 install --user meson
```

This will install meson into ~/.local/bin which may or may not be included
automatically in your PATH by default.

You should get ninja using your package manager or download the official
release and put the ninja
binary in your PATH.

## Build 

```
# meson build/ && ninja -C build/
```

## Usage:

Create two complete branches in the same pipeline:

```
#./gst-n-launch -b "videotestsrc ! autovideosink" -b "videotestsrc num-buffers=100 ! videoconvert ! autovidosink"
```
