# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# _MBCS stands for MultiByte Character Sets
add_compile_definitions(_MBCS)

add_library(dht STATIC dht.cxx)