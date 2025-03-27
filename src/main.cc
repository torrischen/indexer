#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <bptree/bptree.hpp>

int main(int argc, char *argv[]){
    BPlusTree tree("test.db");
    tree.Put("1", "1");
    tree.Put("2", "44");
    tree.Put("3", "3");
    tree.Dump();

    // tree.Delete("2");
    std::string value;
    tree.Get("2", value);
    std::cout << value << std::endl;
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