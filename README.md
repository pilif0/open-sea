# Open Sea

This project is my effort to practice C++ while also improving my skills with OpenGL.
As the purpose is to practice C++, there will be a lot of iterative improvements and often substantial changes of how the system works in order to make it better.
The aim is to develop a 3D game engine from the ground up, focusing on elegant design and performance.
To achieve this, I will explore various approaches (e.g. data oriented design) and technologies (e.g. OpenCL).

## Built With

- OpenGL,
- [GLFW](https://github.com/glfw/glfw/),
- [glad](https://github.com/Dav1dde/glad),
- [GLM](https://github.com/g-truc/glm/),
- [dear imgui](https://github.com/ocornut/imgui/),
- [Boost](https://www.boost.org/),
- [CMake](https://cmake.org/),
- [Doxygen](http://www.stack.nl/~dimitri/doxygen/),
- [CLang](https://clang.llvm.org/)

## Getting Started

### Requirements

- OpenGL >= 4.0,
- Boost >= 1.66.0,

and any libraries required to [build GLFW](https://github.com/glfw/glfw#compiling-glfw).

### CMake Options

- `open_sea_BUILD_EXAMPLES` &mdash; build example programs (default: ON),
- `open_sea_BUILD_DOC` &mdash; build documentation (default: ON),
- `open_sea_DEBUG_LOG` &mdash; debug logging (default: OFF),
- `open_sea_BOOST` &mdash; Boost directory (default: /opt/boost)

## Contributing

Please read the [Contributing Guide](CONTRIBUTING.md) for details on the contribution process.

## Versioning

After release (when the system is at least basically feature complete), the project will use [Semantic Versioning](https://semver.org/).

## Authors

- [Filip Smola](https://smola.me) - Initial work

## Licence

This project is licensed under the MIT licence - see [LICENCE.md](LICENCE.md) for details.
