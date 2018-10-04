clang-tidy -p bin/compile_commands.json -config="" -header-filter="include/open-sea/*" src/*.cpp examples/*/*.cpp > clang-tidy.log
