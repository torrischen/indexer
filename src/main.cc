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
#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-python.h>
#include <tree_sitter/tree-sitter-go.h>
#include <tree_builder/tree_builder.hpp>

const char *file_read(void *payload, uint32_t byte_index, TSPoint position, uint32_t *bytes_read) {
    FilePayload *fp = (FilePayload *)payload;
    
    if (fseek(fp->file, byte_index, SEEK_SET) != 0) {
        *bytes_read = 0;
        return NULL;
    }
    
    *bytes_read = fread(fp->buffer, 1, fp->buffer_size, fp->file);
    return *bytes_read > 0 ? fp->buffer : NULL;
}

int main() {
    // TreeBuilder builder = TreeBuilder(GOLANG);
    TSParser *parser = ts_parser_new();
    bool success = ts_parser_set_language(parser, tree_sitter_go());
    if (!success) {
        std::cout << "Failed to load language\n";
        return 1;
    }
    
    FILE *file = fopen("source.go", "rb");
    if (!file) {
        std::cerr << "Could not open file\n";
        return 1;
    }
    
    FilePayload payload = {
        .file = file,
        .buffer = (char *)malloc(4096),
        .buffer_size = 4096
    };
    
    TSInput input = {
        .payload = &payload,
        .read = file_read,
        .encoding = TSInputEncodingUTF8,
        .decode = NULL
    };
    
    TSTree *tree = ts_parser_parse(parser, NULL, input);
    
    if (!tree) {
        std::cout << "Failed to parse\n";
        return 1;
    }
    
    // 获取根节点
    TSNode root_node = ts_tree_root_node(tree);
    
    // 打印语法树
    char *tree_string = ts_node_string(root_node);
    std::cout << "Syntax tree:\n" << tree_string << std::endl;
    free(tree_string);
    
    // 清理
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    
    return 0;
}