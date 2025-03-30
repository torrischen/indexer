#ifndef TREE_BUILDER_H
#define TREE_BUILDER_H

#include <iostream>
#include <string.h>
#include <tree_sitter/api.h>
#include <vector>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

enum Language {
    TREE_BUILDER_LANGUAGE_GOLANG,
    TREE_BUILDER_LANGUAGE_JAVA,
    TREE_BUILDER_LANGUAGE_PYTHON,
    TREE_BUILDER_LANGUAGE_JAVASCRIPT,
    TREE_BUILDER_LANGUAGE_TYPESCRIPT
};

typedef struct {
    FILE *file;
    char *buffer;
    size_t buffer_size;
}FilePayload;

const char *file_read_helper(void *payload, uint32_t byte_index,
                            TSPoint position, uint32_t *bytes_read) {

    FilePayload *fp = (FilePayload *)payload;

    if (fseek(fp->file, byte_index, SEEK_SET) != 0) {
    *bytes_read = 0;
    return NULL;
    }

    *bytes_read = fread(fp->buffer, 1, fp->buffer_size, fp->file);
    return *bytes_read > 0 ? fp->buffer : NULL;
}

class TreeBuilder {
public:
    TreeBuilder(const Language lang);
    ~TreeBuilder();

    FILE* open_file(const char* path) {
        // Use platform-specific file opening with wide chars on Windows for UTF-8 support
#ifdef _WIN32
        // Convert UTF-8 path to wide chars for Windows
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
        wchar_t* wpath = new wchar_t[size_needed];
        MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, size_needed);
        
        FILE* file = _wfopen(wpath, L"rb");
        delete[] wpath;
#else
        FILE* file = fopen(path, "rb");
#endif
        if (!file) {
            return nullptr;
        }
        return file;
    }
    
    FilePayload load_file_to_payload(FILE* file);
    TSInput construct_parser_input(FilePayload* payload);
    TSTree *build_tree(TSInput input);
    void delete_tree(TSTree *tree);
    TSNode get_root_node(TSTree *tree);
    std::vector<TSPoint> query(TSTree *tree, const std::string &query_str); 

    void print(TSTree *tree);

private:
    // private fields
    TSParser *parser;
    const TSLanguage *language;
};

#endif // TREE_BUILDER_H