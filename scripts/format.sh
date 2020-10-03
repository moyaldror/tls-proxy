#!/bin/bash

find . -regex '.*\.\(cpp\|hpp\|cu\|c\|h\)' -exec clang-format -style=file -i {} \;
find . -regex '.*\.\(cpp\|hpp\|cu\|c\|h\)' | xargs clang-tidy -header-filter=.* -format-style=file -p build/ -fix -fix-errors
