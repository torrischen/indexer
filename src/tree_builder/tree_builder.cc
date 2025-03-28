#include <tree_builder/tree_builder.hpp>
#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-go.h>
#include <tree_sitter/tree-sitter-java.h>
#include <tree_sitter/tree-sitter-python.h>
#include <tree_sitter/tree-sitter-javascript.h>
#include <tree_sitter/tree-sitter-typescript.h>

TreeBuilder::TreeBuilder(const Language lang) {
    TSParser *_parser = ts_parser_new();
    const TSLanguage *_language;

    switch (lang) {
        case (GOLANG):
            _language = tree_sitter_go();
            break;
        case (JAVA):
            _language = tree_sitter_java();
            break;
        case (PYTHON):
            _language = tree_sitter_python();
            break;
        case (JAVASCRIPT):
            _language = tree_sitter_javascript();
            break;
        case (TYPESCRIPT):
            _language = tree_sitter_typescript();
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

TreeBuilder::~TreeBuilder() {}

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
        .payload = &payload,
        .read = file_read_helper,
        .encoding = TSInputEncodingUTF8,
        .decode = NULL
    };

    return input;
}