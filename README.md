# arblang

This repository contains the functionality necessary to build
an end-to-end compiler for the `arblang` DSL. 

The arblang DSL specifications can be found under `doc/specifications`, 
but not all features described in the DSL are supported by this initial 
version of the arblang compiler. 

The documentation for the compiler functionality is contained in the 
source code.

To test the compiler: 
```
$ mkdir build && cd build
$ cmake ..
$ make -j compiler
$ ./bin/compiler -o output_name -N namespace /path/to/arblang/source
```
This will generate 2 files: `output_name.hpp` and `output_name_cpu.cpp`, 
written against arbor's mechanism ABI, which can be compiled into a catalogue
to be used by the arbor CPU simulation using `arbor-build-catalogue`. 

To run the unit tests:
```
$ make -j unit
$ ./bin/unit
```