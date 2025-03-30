cmake_minimum_required(VERSION 3.10)

project(indexer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/include
)

link_directories(
    # ${CMAKE_CURRENT_SOURCE_DIR}/lib
    /usr/local/lib
)

file(GLOB_RECURSE BPTREE_SOURCE
    src/bptree/*.cc
)

file(GLOB_RECURSE TREE_BUILDER_SOURCE
    src/tree_builder/*.cc
)

# file(GLOB_RECURSE INDEXER_SOURCE
#     src/*.cc
# )

add_library(bptree SHARED
    ${BPTREE_SOURCE}
)

add_library(tree_builder SHARED
    ${TREE_BUILDER_SOURCE}
)

# add_library(indexer SHARED
#     ${INDEXER_SOURCE}
# )

target_link_libraries(tree_builder
    tree-sitter
    tree-sitter-python
    tree-sitter-go
    tree-sitter-javascript
    tree-sitter-typescript
    tree-sitter-tsx
    tree-sitter-java
)

add_executable(${PROJECT_NAME} main.cc)

target_link_libraries(${PROJECT_NAME}
    bptree
    tree_builder
    tree-sitter
    tree-sitter-python
    tree-sitter-go
    tree-sitter-javascript
    tree-sitter-typescript
    tree-sitter-tsx
    tree-sitter-java
)