# UIUC CS418 Rasterizer

* This is not a good rasterizer. I know enough to do better now and maybe one day I'll come back to it, but until then it will remain poorly implemented
* There are 2 copies of my poorly implemented rasterizer. The C files in the base folder and CMakeLists.txt were developed and heavily rely on CLion from JetBrains.
* The copy in the `make` folder should work on any system clang and have their own Makefile that can be run through the terminal agnostic of any Jetbrains requirements.
* Requires `libpng` be installed somewhere. Mine was installed at the system level on Apple Silicon so the Clion files are heavily tied to that but the `make` directory was tested on linux.