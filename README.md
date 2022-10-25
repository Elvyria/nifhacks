# NifHacks
![License](https://img.shields.io/github/license/Elvyria/nifhacks?color=green)

## Features:
* Export **nif** shapes to ***obj**
* Transfer *vertex/normal/texture* coordinates from **obj** to **nif**

## Usage
```
$ nifhacks --help
Janky tool that (sometimes) get things done for your nif-needs

Usage: nifhacks [OPTIONS] INPUT OUTPUT

Positionals:
  INPUT     *.nif *.obj       
  OUTPUT    *.obj *.nif       

Options:
  -h,--help                   Print this help message and exit
  -s,--skin                   Apply skin transforms to shape
  -i,--in-place               Modify file in place
  -v,--version                Display program version information and exit
```

To export skinned shape (highpoly head)
```bash
$ nifhacks -s head.nif head.obj
```

To import changes back to **nif**. (Your **.nif** file will not be overwritten by default, expect changes in **.nif.nif** file)
```bash
$ nifhacks -s eyes.obj head.nif
```

## Building
Aside from c++ build tools you'll need [CMake](https://cmake.org) and [Conan](https://conan.io)
```bash
$ git clone --depth 1 https://github.com/Elvyria/nifhacks
$ cd nifhacks
$ mkdir build && cd build
$ conan install .. -s build_type=Release --build=missing
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . --config Release
```
