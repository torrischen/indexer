#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <bptree/bptree.hpp>
using bptree::bplus_tree;

int main(int argc, char *argv[]){
    bplus_tree tree("test.db", true);
    int insert_result = tree.insert("2", "111");
    std::cout << insert_result << std::endl;
    bptree::value_t value;
    int search_result = tree.search("2", &value);
    printf("result: %d\n", search_result);
    std::cout << value.v << std::endl;
}


// #include <stdio.h>
// #include <stdlib.h>
// #include <iostream>
// #include <tree_sitter/api.h>
// #include <tree_sitter/tree-sitter-python.h>
// #include <tree_sitter/tree-sitter-go.h>

// int main() {
//     TSParser *parser = ts_parser_new();
//     bool success = ts_parser_set_language(parser, tree_sitter_go());
//     if (!success) {
//         std::cout << "Failed to load language\n";
//         return 1;
//     }
// }