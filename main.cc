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
#include <tree_sitter/tree-sitter-tsx.h>

void print_captures(TSQueryCursor *cursor) {
    TSQueryMatch match;
    uint32_t index;
    while (ts_query_cursor_next_capture(cursor, &match, &index)) {
        std::cout << "got the index: " << index << std::endl;

        TSPoint point = ts_node_start_point(match.captures->node);
        std::cout << "col: " << point.column << "  " << "row: " << point.row << std::endl;
    }
}

int main() {
    TreeBuilder builder = TreeBuilder(TREE_BUILDER_LANGUAGE_TYPESCRIPT);
    FILE *file = builder.open_file("./source.tsx");
    FilePayload payload = builder.load_file_to_payload(file);
    TSInput input = builder.construct_parser_input(&payload);
    TSTree *tree = builder.build_tree(input);

    builder.print(tree);

    std::string src = R"((interface_declaration) @iface.dec)";
    std::vector<TSPoint> result = builder.query(tree, src);

    std::cout << result[0].column << std::endl;

    // uint32_t error_offset = 0;
    // TSQueryError error_type = TSQueryErrorNone;
    // TSQuery *query = ts_query_new(tree_sitter_tsx(), src.c_str(), strlen(src.c_str()), &error_offset, &error_type);
    // TSQueryCursor *cursor = ts_query_cursor_new();
    // ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));
    // print_captures(cursor);
}