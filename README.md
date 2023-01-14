# bq-r ![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/sarahkittyy/blockquest-remake?label=VERSION&style=plastic)

[Download the latest release here :)](https://github.com/sarahkittyy/blockquest-remake/releases/latest)

[Roadmap](https://trello.com/b/i9xWNDe9/blockquest-remake)

This is a remake of [BlockQuest](http://www.blockquest.net/).

![Run](gifs/run.gif)
![Jump](gifs/jump.gif)
![Dash](gifs/dash.gif)
![Climb](gifs/climb.gif)
![Wallkick](gifs/wallkick.gif)

All assets in `assets/` are ripped straight from the original BlockQuest SWF available for download on the [blockquest site](http://www.blockquest.net). Daigo don't sue me :p

## Dependencies

- C++20
- OpenSSL

## Building

```bash
$ git clone https://github.com/sarahkittyy/blockquest-remake
$ cd blockquest-remake
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Running

The `assets/` folder must be in the current directory.

```bash
$ cd blockquest-remake
$ ./build/bq-r
```
