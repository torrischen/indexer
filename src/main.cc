// #include <assert.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <algorithm>
// #include <iostream>
// #include <bptree/bptree.hpp>

// int main(int argc, char *argv[]) {
//     BPlusTree tree("test.db");
//     tree.upsert("1", "1");
//     tree.upsert("2", "44");
//     tree.upsert("3", "3");
//     tree.dump();

//     // tree.remove("2");
//     std::string value;
//     tree.get("2", value);
//     std::cout << value << std::endl;
// }


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <tree_builder/tree_builder.hpp>

int main() {
    TreeBuilder builder = TreeBuilder(TREE_BUILDER_LANGUAGE_TYPESCRIPT);
    FILE *file = builder.open_file("./source.ts");
    FilePayload payload = builder.load_file_to_payload(file);
    TSInput input = builder.construct_parser_input(&payload);
    TSTree *tree = builder.build_tree(input);
    builder.print(tree);

    builder.delete_tree(tree);
}