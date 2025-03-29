#include <tree_builder/tree_builder.hpp>
#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-go.h>
#include <tree_sitter/tree-sitter-java.h>
#include <tree_sitter/tree-sitter-python.h>
#include <tree_sitter/tree-sitter-javascript.h>
#include <tree_sitter/tree-sitter-tsx.h>

TreeBuilder::TreeBuilder(const Language lang) {
    TSParser *_parser = ts_parser_new();
    const TSLanguage *_language;

    switch (lang) {
        case (TREE_BUILDER_LANGUAGE_GOLANG):
            _language = tree_sitter_go();
            break;
        case (TREE_BUILDER_LANGUAGE_JAVA):
            _language = tree_sitter_java();
            break;
        case (TREE_BUILDER_LANGUAGE_PYTHON):
            _language = tree_sitter_python();
            break;
        case (TREE_BUILDER_LANGUAGE_JAVASCRIPT):
            _language = tree_sitter_javascript();
            break;
        case (TREE_BUILDER_LANGUAGE_TYPESCRIPT):
            _language = tree_sitter_tsx();
            break;
        default:
            _language = tree_sitter_java();
    }

    bool __set_lang_success = ts_parser_set_language(_parser, _language);
    if (!__set_lang_success) {
        throw std::runtime_error("fail to set parser's language");
    }

    language = _language;
    parser = _parser;
}

TreeBuilder::~TreeBuilder() {
    ts_parser_delete(parser);
    ts_language_delete(language);
}

FilePayload TreeBuilder::load_file_to_payload(FILE *file) {
    FilePayload payload = {
        .file = file,
        .buffer = (char *)malloc(4096),
        .buffer_size = 4096
    };

    return payload;
}

TSInput TreeBuilder::construct_parser_input(FilePayload *payload) {
    TSInput input = {
        .payload = payload,
        .read = file_read_helper,
        .encoding = TSInputEncodingUTF8,
        .decode = NULL
    };
    return input;
}

TSTree *TreeBuilder::build_tree(TSInput input) {
    return ts_parser_parse(parser, NULL, input);
}

void TreeBuilder::delete_tree(TSTree *tree) {
    ts_tree_delete(tree);
}

TSNode TreeBuilder::get_root_node(TSTree *tree) {
    return ts_tree_root_node(tree);
}

void TreeBuilder::print(TSTree *tree) {
    TSNode node = get_root_node(tree);
    char *tree_string = ts_node_string(node);
    puts(tree_string);
    free(tree_string);
}