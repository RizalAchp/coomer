# Coomer
Zoomer utility program inspired by https://github.com/tsoding/boomer but using C

## Dependencies

### Debian

```console
$ sudo apt-get install libgl1-mesa-dev libx11-dev libxext-dev libxrandr-dev
```

### Arch

```console
$ sudo pacman -S mesa libx11 libxext libxrandr
```

## Quick Start

```console
$ // using gnu compiler
$ gcc -o cb ./cb.c
$ // using clang
$ clang -o cb ./cb.c
$
$ ./cb config --release --display && ./cb build
$ ./build/release/coomer/coomer                 // just start the output executable 
```

or install it, it will installed the executable to path `$HOME/.local/bin`

```console
$ ./cb install
```

remove the `$`

## Controls

| Control                                   | Description                                                   |
|-------------------------------------------|---------------------------------------------------------------|
| <kbd>0</kbd>                              | Reset the application state (position, scale, velocity, etc). |
| <kbd>q</kbd> or <kbd>ESC</kbd>            | Quit the application.                                         |
| <kbd>r</kbd>                              | Reload configuration.                                         |
| <kbd>f</kbd>                              | Toggle flashlight effect.                                     |
| Drag with left mouse button               | Move the image around.                                        |
| Scroll wheel or <kbd>=</kbd>/<kbd>-</kbd> | Zoom in/out.                                                  |
| <kbd>Ctrl</kbd> + Scroll wheel            | Change the radious of the flaslight.                          |

## Configuration

Configuration file is located at `$HOME/.config/coomer/config.cfg` and has roughly the following format:

```cfg
<param-1> = <value-1>
<param-2> = <value-2>
# comment
<param-3> = <value-3>
```
You can generate a new config at `$HOME/.config/coomer/config.cfg` with `$ coomer --new-config`.

Supported parameters:

| Name           | Description                                        |
|----------------|----------------------------------------------------|
| min_scale      | The smallest it can get when zooming out           |
| scroll_speed   | How quickly you can zoom in/out by scrolling       |
| drag_friction  | How quickly the movement slows down after dragging |
| scale_friction | How quickly the zoom slows down after scrolling    |
