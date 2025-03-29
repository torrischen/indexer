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

void print_captures(TSQuery *query, TSQueryCursor *cursor) {
    TSQueryMatch match;
    uint32_t index;
    ts_query_cursor_next_capture(cursor, &match, &index);
    while (ts_query_cursor_next_capture(cursor, &match, &index)) {
        std::cout << "Match at " << match.pattern_index << ":\n";
        
        for (uint32_t i = 0; i < match.capture_count; ++i) {
            TSQueryCapture capture = match.captures[i];
            const char *name = ts_query_capture_name_for_id(query, capture.index, nullptr);
            TSNode node = capture.node;
            
            uint32_t start = ts_node_start_byte(node);
            uint32_t end = ts_node_end_byte(node);
            
            std::cout << "  Capture @" << name << ": " << ts_node_type(node)
                      << " (" << start << "-" << end << "): ";
        }
    }
}

int main() {
    TreeBuilder builder = TreeBuilder(TREE_BUILDER_LANGUAGE_TYPESCRIPT);
    FILE *file = builder.open_file("./source.tsx");
    FilePayload payload = builder.load_file_to_payload(file);
    TSInput input = builder.construct_parser_input(&payload);
    TSTree *tree = builder.build_tree(input);

    builder.print(tree);

    std::string src = "(call_expression)";
    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    TSQuery *query = ts_query_new(tree_sitter_tsx(), src.c_str(), strlen(src.c_str()), &error_offset, &error_type);
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));
    print_captures(query, cursor);
}