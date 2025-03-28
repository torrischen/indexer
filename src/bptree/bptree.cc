#include "bptree/bptree.hpp"

#include <cassert>
#include <cstring>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

const off_t kMetaOffset = 0;
const int kOrder = 32;
static_assert(kOrder >= 3,
                            "The order of B+Tree should be greater than or equal to 3.");
const int kMaxKeySize = 32;
const int kMaxValueSize = 256;
const int kMaxCacheSize = 1024 *    1024 * 5;
typedef char Key[kMaxKeySize];
typedef char Value[kMaxValueSize];

void Exit(const char* msg) {
#ifdef _WIN32
    fprintf(stderr, "%s: Error code %lu\n", msg, GetLastError());
#else
    perror(msg);
#endif
    exit(EXIT_FAILURE);
}

struct BPlusTree::Meta {
    off_t offset;     // ofset of self
    off_t root;         // offset of root
    off_t block;        // offset of next new node
    size_t height;    // height of B+Tree
    size_t size;        // key size
};

struct BPlusTree::Index {
    Index() : offset(0) { std::memset(key, 0, sizeof(key)); }

    off_t offset;
    Key key;

    void UpdateIndex(off_t of, const char* k) {
        offset = of;
        strncpy(key, k, kMaxKeySize);
    }
    void UpdateKey(const char* k) { strncpy(key, k, kMaxKeySize); }
};

struct BPlusTree::Record {
    Key key;
    Value value;

    void UpdateKV(const char* k, const char* v) {
        strncpy(key, k, kMaxKeySize);
        strncpy(value, v, kMaxValueSize);
    }
    void UpdateKey(const char* k) { strncpy(key, k, kMaxKeySize); }
    void UpdateValue(const char* v) { strncpy(value, v, kMaxValueSize); }
};

struct BPlusTree::Node {
    Node() : parent(0), left(0), right(0), count(0) {}
    Node(off_t parent_, off_t leaf_, off_t right_, size_t count_)
            : parent(parent_), left(leaf_), right(right_), count(count_) {}
    ~Node() = default;

    off_t offset;    // offset of self
    off_t parent;    // offset of parent
    off_t left;        // offset of left node(may be sibling)
    off_t right;     // offset of right node(may be sibling)
    size_t count;    // count of keys
};

struct BPlusTree::IndexNode : BPlusTree::Node {
    IndexNode() = default;
    ~IndexNode() = default;

    const char* FirstKey() const {
        assert(count > 0);
        return indexes[0].key;
    }

    const char* LastKey() const {
        assert(count > 0);
        return indexes[count - 1].key;
    }

    const char* Key(int index) const {
        assert(count > 0);
        assert(index >= 0);
        assert(index <= kOrder);
        return indexes[index].key;
    }

    void UpdateKey(int index, const char* k) {
        assert(index >= 0);
        assert(index <= kOrder);
        indexes[index].UpdateKey(k);
    }

    void UpdateOffset(int index, off_t offset) {
        assert(index >= 0);
        assert(index <= kOrder);
        indexes[index].offset = offset;
    }

    void UpdateIndex(int index, const char* k, off_t offset) {
        assert(index >= 0);
        assert(index <= kOrder);
        UpdateKey(index, k);
        UpdateOffset(index, offset);
    }

    void DeleteKeyAtIndex(int index) {
        assert(index >= 0);
        assert(index <= kOrder);
        std::memmove(&indexes[index], &indexes[index + 1],
                                 sizeof(indexes[0]) * (count-- - index));
    }

    void InsertKeyAtIndex(int index, const char* k) {
        assert(index >= 0);
        assert(index <= kOrder);
        std::memmove(&indexes[index + 1], &indexes[index],
                                 sizeof(indexes[0]) * (++count - index));
        UpdateKey(index, k);
    }

    void InsertIndexAtIndex(int index, const char* k, off_t offset) {
        assert(index >= 0);
        assert(index <= kOrder);
        std::memmove(&indexes[index + 1], &indexes[index],
                                 sizeof(indexes[0]) * (++count - index));
        UpdateIndex(index, k, offset);
    }

    void MergeLeftSibling(IndexNode* sibling) {
        std::memmove(&indexes[sibling->count + 1], &indexes[0],
                                 sizeof(indexes[0]) * (count + 1));
        std::memcpy(&indexes[0], &sibling->indexes[0],
                                sizeof(indexes[0]) * (sibling->count + 1));
        count += (sibling->count + 1);
    }

    void MergeRightSibling(IndexNode* sibling) {
        std::memcpy(&indexes[count], &sibling->indexes[0],
                                sizeof(indexes[0]) * (sibling->count + 1));
        count += sibling->count;
    }

    Index indexes[kOrder + 1];
};

struct BPlusTree::LeafNode : BPlusTree::Node {
    LeafNode() = default;
    ~LeafNode() = default;

    const char* FirstKey() const {
        assert(count > 0);
        return records[0].key;
    }

    const char* LastKey() const {
        assert(count > 0);
        return records[count - 1].key;
    }

    const char* Key(int index) const {
        assert(count > 0);
        assert(index >= 0);
        return records[index].key;
    }

    const char* FirstValue() const {
        assert(count > 0);
        return records[0].value;
    }

    const char* LastValue() const {
        assert(count > 0);
        return records[count - 1].value;
    }

    const char* Value(int index) const {
        assert(count > 0);
        return records[index].value;
    }

    void UpdateValue(int index, const char* v) {
        assert(index >= 0);
        records[index].UpdateValue(v);
    }

    void UpdateKey(int index, const char* k) {
        assert(index >= 0);
        records[index].UpdateKey(k);
    }

    void UpdateKV(int index, const char* k, const char* v) {
        assert(index >= 0);
        records[index].UpdateKV(k, v);
    }

    void InsertKVAtIndex(int index, const char* k, const char* v) {
        assert(index >= 0);
        assert(index < kOrder);
        std::memmove(&records[index + 1], &records[index],
                                 sizeof(records[0]) * (count++ - index));
        UpdateKV(index, k, v);
    }

    void DeleteKVAtIndex(int index) {
        assert(index >= 0);
        assert(index < kOrder);
        std::memmove(&records[index], &records[index + 1],
                                 sizeof(records[0]) * (--count - index));
    }

    void MergeLeftSibling(LeafNode* sibling) {
        std::memmove(&records[sibling->count], &records[0],
                                 sizeof(records[0]) * count);
        std::memcpy(&records[0], &sibling->records[0],
                                sizeof(records[0]) * sibling->count);
        count += sibling->count;
    }

    void MergeRightSibling(LeafNode* sibling) {
        std::memcpy(&records[count], &sibling->records[0],
                                sizeof(records[0]) * sibling->count);
        count += sibling->count;
    }

    BPlusTree::Record records[kOrder];
};

class BPlusTree::BlockCache {
    struct Node;

 public:
    BlockCache() : head_(new Node()), size_(0) {
        head_->next = head_;
        head_->prev = head_;
    }

    ~BlockCache() {
        for (auto it = offset2node_.begin(); it != offset2node_.end(); it++) {
            Node* node = it->second;
#ifdef _WIN32
            UnmapViewOfFile(node->block);
            CloseHandle(node->hMapFile);
#else
            off_t page_offset = node->offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
            char* start = reinterpret_cast<char*>(node->block);
            void* addr = static_cast<void*>(&start[page_offset - node->offset]);
            if (munmap(addr, node->size + node->offset - page_offset) != 0) {
                Exit("munmap");
            }
#endif
            delete node;
        }
        delete head_;
    }

    void DeleteNode(Node* node) {
        if (node->next == node->prev && nullptr == node->next) return;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->next = node->prev = nullptr;
        size_ -= node->size;
    }

    void InsertHead(Node* node) {
        node->next = head_->next;
        node->prev = head_;
        head_->next->prev = node;
        head_->next = node;
        size_ += node->size;
    }

    Node* DeleteTail() {
        if (size_ == 0) {
            assert(head_->next == head_);
            assert(head_->prev == head_);
            return nullptr;
        }
        Node* tail = head_->prev;
        DeleteNode(tail);
        return tail;
    }

    template <typename T>
    void upsert(T* block) {
        while (size_ > kMaxCacheSize) Kick();

        if (offset2node_.find(block->offset) == offset2node_.end()) {
            Node* node = new Node(block, block->offset, sizeof(T));
            offset2node_.emplace(block->offset, node);
            InsertHead(node);
        } else {
            Node* node = offset2node_[block->offset];
            if (--node->ref == 0) InsertHead(node);
        }
    }

    template <typename T>
    T* get(int fd, off_t offset) {
        if (offset2node_.find(offset) == offset2node_.end()) {
            constexpr int size = sizeof(T);
#ifdef _WIN32
            HANDLE hFile = (HANDLE)_get_osfhandle(fd);
            if (hFile == INVALID_HANDLE_VALUE) Exit("_get_osfhandle");

            LARGE_INTEGER fileSize;
            if (!GetFileSizeEx(hFile, &fileSize)) Exit("GetFileSizeEx");
            if (fileSize.QuadPart < offset + size) {
                LARGE_INTEGER newSize;
                newSize.QuadPart = offset + size;
                if (!SetFilePointerEx(hFile, newSize, NULL, FILE_BEGIN) || 
                        !SetEndOfFile(hFile)) {
                    Exit("SetFilePointerEx/SetEndOfFile");
                }
            }

            HANDLE hMapFile = CreateFileMapping(
                    hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
            if (hMapFile == NULL) Exit("CreateFileMapping");

            void* mapAddr = MapViewOfFile(
                    hMapFile, FILE_MAP_ALL_ACCESS, 0, offset, size);
            if (mapAddr == NULL) {
                CloseHandle(hMapFile);
                Exit("MapViewOfFile");
            }

            T* block = static_cast<T*>(mapAddr);
            Node* node = new Node(block, offset, size);
            node->hMapFile = hMapFile;
#else
            struct stat st;
            if (fstat(fd, &st) != 0) Exit("fstat");
            if (st.st_size < offset + size && ftruncate(fd, offset + size) != 0) {
                Exit("ftruncate");
            }
            // Align offset to page size.
            off_t page_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
            void* addr = mmap(nullptr, size + offset - page_offset,
                                                PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_offset);
            if (MAP_FAILED == addr) Exit("mmap");
            char* start = static_cast<char*>(addr);
            T* block = reinterpret_cast<T*>(&start[offset - page_offset]);
            Node* node = new Node(block, offset, size);
#endif
            offset2node_.emplace(offset, node);
            return block;
        }

        Node* node = offset2node_[offset];
        ++node->ref;
        DeleteNode(node);
        return static_cast<T*>(node->block);
    }

 private:
    void Kick() {
        Node* tail = DeleteTail();
        if (nullptr == tail) return;

        assert(tail != head_);

#ifdef _WIN32
        UnmapViewOfFile(tail->block);
        CloseHandle(tail->hMapFile);
#else
        off_t page_offset = tail->offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
        char* start = reinterpret_cast<char*>(tail->block);
        void* addr = static_cast<void*>(&start[page_offset - tail->offset]);
        if (munmap(addr, tail->size + tail->offset - page_offset) != 0) {
            Exit("munmap");
        }
#endif
        offset2node_.erase(tail->offset);
        delete tail;
    }

    struct Node {
        Node()
                : block(nullptr),
                    offset(0),
                    size(0),
                    ref(0),
                    prev(nullptr),
                    next(nullptr) {}
#ifdef _WIN32
                    , hMapFile(NULL)
#endif

        Node(void* block_, off_t offset_, size_t size_)
                : block(block_),
                    offset(offset_),
                    size(size_),
                    ref(1),
                    prev(nullptr),
                    next(nullptr) {}
#ifdef _WIN32
                    , hMapFile(NULL)
#endif

        void* block;
        off_t offset;
        size_t size;
        size_t ref;
        Node* prev;
        Node* next;
#ifdef _WIN32
        HANDLE hMapFile;
#endif
    };

    Node* head_;
    size_t size_;
    std::unordered_map<off_t, Node*> offset2node_;
};

BPlusTree::BPlusTree(const char* path)
        : block_cache_(new BlockCache()) {
#ifdef _WIN32
    fd_ = _open(path, _O_CREAT | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
    fd_ = open(path, O_CREAT | O_RDWR, 0600);
#endif
    if (fd_ == -1) Exit("open");
    
    meta_ = map<Meta>(kMetaOffset);
    if (meta_->height == 0) {
        // Initialize B+tree;
        constexpr off_t of_root = kMetaOffset + sizeof(Meta);
        LeafNode* root = new (map<LeafNode>(of_root)) LeafNode();
        root->offset = of_root;
        meta_->height = 1;
        meta_->root = of_root;
        meta_->block = of_root + sizeof(LeafNode);
        unmap<LeafNode>(root);
    }
}

BPlusTree::~BPlusTree() {
    unmap(meta_);
    delete block_cache_;
#ifdef _WIN32
    _close(fd_);
#else
    close(fd_);
#endif
}

void BPlusTree::upsert(const std::string& key, const std::string& value) {
    // 1. Find Leaf node.
    off_t of_leaf = get_leaf_offset(key.data());
    LeafNode* leaf_node = map<LeafNode>(of_leaf);
    if (insert_kv_into_leaf_node(leaf_node, key.data(), value.data()) <=
            get_max_keys()) {
        // 2.If records of leaf node less than or equals kOrder - 1 then finish.
        unmap<LeafNode>(leaf_node);
        return;
    }

    // 3. Split leaf node to two leaf nodes.
    LeafNode* split_node = split_leaf_node(leaf_node);
    const char* mid_key = split_node->FirstKey();
    IndexNode* parent_node = get_or_create_parent(leaf_node);
    off_t of_parent = leaf_node->parent;
    split_node->parent = of_parent;

    // 4.Insert key to parent of splited leaf nodes and
    // link two splited left nodes to parent.
    if (insert_key_into_index_node(parent_node, mid_key, leaf_node, split_node) <=
            get_max_keys()) {
        unmap<LeafNode>(leaf_node);
        unmap<LeafNode>(split_node);
        unmap<IndexNode>(parent_node);
        return;
    }

    // 5.Split index node from bottom to up repeatedly
    // until count <= kOrder - 1.
    size_t count;
    do {
        IndexNode* child_node = parent_node;
        IndexNode* split_node = split_index_node(child_node);
        const char* mid_key = child_node->Key(child_node->count);
        parent_node = get_or_create_parent(child_node);
        of_parent = child_node->parent;
        split_node->parent = of_parent;
        count =
                insert_key_into_index_node(parent_node, mid_key, child_node, split_node);
        unmap<IndexNode>(child_node);
    } while (count > get_max_keys());
    unmap<IndexNode>(parent_node);
}

bool BPlusTree::remove(const std::string& key) {
    off_t of_leaf = get_leaf_offset(key.data());
    LeafNode* leaf_node = map<LeafNode>(of_leaf);
    // 1. remove key from leaf node
    int index = get_index_from_leaf_node(leaf_node, key.data());
    if (index == -1) {
        unmap(leaf_node);
        return false;
    }

    leaf_node->DeleteKVAtIndex(index);
    --meta_->size;
    // 2. If leaf_node is root then return.
    if (leaf_node->parent == 0) {
        unmap(leaf_node);
        return true;
    }

    // 3. If count of leaf_node >= get_min_keys() then return else execute step 3.
    if (leaf_node->count >= get_min_keys()) {
        unmap(leaf_node);
        return true;
    }

    // 4. If borrow from siblings successfully then return else execute step 4.
    if (borrow_from_leaf_sibling(leaf_node)) {
        unmap<LeafNode>(leaf_node);
        return true;
    }

    // 5. Merge two leaf nodes.
    leaf_node = merge_leaf(leaf_node);

    IndexNode* index_node = map<IndexNode>(leaf_node->parent);
    unmap<LeafNode>(leaf_node);

    // 6. If count of index_node >= get_min_keys() then return or execute 6.
    // 7. If count of one of sibling > get_min_keys() then swap its key and parent's
    // key then return or execute 7.
    while (index_node->parent != 0 && index_node->count < get_min_keys() &&
                 !borrow_from_index_sibling(index_node)) {
        // 8. Merge index_node and its' parent and sibling.
        IndexNode* old_index_node = merge_index(index_node);
        index_node = map<IndexNode>(old_index_node->parent);
        unmap(old_index_node);
    }

    if (index_node->parent == 0 && index_node->count == 0) {
        // 9. Root is removed, update new root and height.
        Node* new_root = map<Node>(index_node->indexes[0].offset);
        assert(new_root->left == 0);
        assert(new_root->right == 0);
        new_root->parent = 0;
        meta_->root = new_root->offset;
        --meta_->height;
        unmap(new_root);
        dealloc(index_node);
        return true;
    }

    unmap<IndexNode>(index_node);
    return true;
}

bool BPlusTree::get(const std::string& key, std::string& value) const {
    off_t of_leaf = get_leaf_offset(key.data());
    LeafNode* leaf_node = map<LeafNode>(of_leaf);
    int index = get_index_from_leaf_node(leaf_node, key.data());
    if (index == -1) {
        unmap<LeafNode>(leaf_node);
        return false;
    }
    value = leaf_node->Value(index);
    unmap<LeafNode>(leaf_node);
    return true;
}

template <typename T>
T* BPlusTree::map(off_t offset) const {
    return block_cache_->get<T>(fd_, offset);
}

template <typename T>
void BPlusTree::unmap(T* map_obj) const {
    block_cache_->upsert<T>(map_obj);
}

constexpr size_t BPlusTree::get_min_keys() const { return (kOrder + 1) / 2 - 1; }

constexpr size_t BPlusTree::get_max_keys() const { return kOrder - 1; }

BPlusTree::IndexNode* BPlusTree::get_or_create_parent(Node* node) {
    if (node->parent == 0) {
        // Split root node.
        IndexNode* parent_node = alloc<IndexNode>();
        node->parent = parent_node->offset;
        meta_->root = parent_node->offset;
        ++meta_->height;
        return parent_node;
    }
    return map<IndexNode>(node->parent);
}

template <typename T>
int BPlusTree::upper_bound(T arr[], int n, const char* key) const {
    assert(n <= get_max_keys());
    int l = 0, r = n - 1;
    while (l <= r) {
        int mid = (l + r) >> 1;
        if (std::strncmp(arr[mid].key, key, kMaxKeySize) <= 0) {
            l = mid + 1;
        } else {
            r = mid - 1;
        }
    }
    return l;
}

template <typename T>
int BPlusTree::lower_bound(T arr[], int n, const char* key) const {
    assert(n <= get_max_keys());
    int l = 0, r = n - 1;
    while (l <= r) {
        int mid = (l + r) >> 1;
        if (std::strncmp(arr[mid].key, key, kMaxKeySize) < 0) {
            l = mid + 1;
        } else {
            r = mid - 1;
        }
    }
    return l;
};

template <typename T>
T* BPlusTree::alloc() {
    T* node = new (map<T>(meta_->block)) T();
    node->offset = meta_->block;
    meta_->block += sizeof(T);
    return node;
}

template <typename T>
void BPlusTree::dealloc(T* node) {
    unmap<T>(node);
}

off_t BPlusTree::get_leaf_offset(const char* key) const {
    size_t height = meta_->height;
    off_t offset = meta_->root;
    if (height <= 1) {
        assert(height == 1);
        return offset;
    }
    // 1. Find bottom index node.
    IndexNode* index_node = map<IndexNode>(offset);
    while (--height > 1) {
        int index = upper_bound(index_node->indexes, index_node->count, key);
        off_t of_child = index_node->indexes[index].offset;
        unmap(index_node);
        index_node = map<IndexNode>(of_child);
        offset = of_child;
    }
    // 2. get offset of leaf node.
    int index = upper_bound(index_node->indexes, index_node->count, key);
    off_t of_child = index_node->indexes[index].offset;
    unmap<IndexNode>(index_node);
    return of_child;
}

inline size_t BPlusTree::insert_key_into_index_node(IndexNode* index_node,
                                                                                                const char* key,
                                                                                                Node* left_node,
                                                                                                Node* right_node) {
    assert(index_node->count <= get_max_keys());
    int index = upper_bound(index_node->indexes, index_node->count, key);
    index_node->InsertIndexAtIndex(index, key, left_node->offset);
    index_node->UpdateOffset(index + 1, right_node->offset);
    return index_node->count;
}

size_t BPlusTree::insert_kv_into_leaf_node(LeafNode* leaf_node, const char* key,
                                                                             const char* value) {
    assert(leaf_node->count <= get_max_keys());
    int index = upper_bound(leaf_node->records, leaf_node->count, key);
    if (index > 0 &&
            std::strncmp(leaf_node->Key(index - 1), key, kMaxKeySize) == 0) {
        leaf_node->UpdateValue(index - 1, value);
        return leaf_node->count;
    }

    leaf_node->InsertKVAtIndex(index, key, value);
    ++meta_->size;
    return leaf_node->count;
}

BPlusTree::LeafNode* BPlusTree::split_leaf_node(LeafNode* leaf_node) {
    assert(leaf_node->count == kOrder);
    constexpr int mid = (kOrder - 1) >> 1;
    constexpr int left_count = mid;
    constexpr int right_count = kOrder - mid;

    LeafNode* split_node = alloc<LeafNode>();

    // Change count.
    leaf_node->count = left_count;
    split_node->count = right_count;

    // Copy right part of index_node.
    std::memcpy(&split_node->records[0], &leaf_node->records[mid],
                            sizeof(split_node->records[0]) * right_count);

    // Link siblings.
    split_node->left = leaf_node->offset;
    split_node->right = leaf_node->right;
    leaf_node->right = split_node->offset;
    if (split_node->right != 0) {
        LeafNode* new_sibling = map<LeafNode>(split_node->right);
        new_sibling->left = split_node->offset;
        unmap(new_sibling);
    }
    return split_node;
}

BPlusTree::IndexNode* BPlusTree::split_index_node(IndexNode* index_node) {
    assert(index_node->count == kOrder);
    constexpr int mid = (kOrder - 1) >> 1;
    constexpr int left_count = mid;
    constexpr int right_count = kOrder - mid - 1;

    IndexNode* split_node = alloc<IndexNode>();

    // Change count.
    index_node->count = left_count;
    split_node->count = right_count;

    // Copy right part of index_node.
    std::memcpy(&split_node->indexes[0], &index_node->indexes[mid + 1],
                            sizeof(split_node->indexes[0]) * (right_count + 1));

    // Link old childs to new splited parent.
    for (int i = mid + 1; i <= kOrder; ++i) {
        off_t of_child = index_node->indexes[i].offset;
        LeafNode* child_node = map<LeafNode>(of_child);
        child_node->parent = split_node->offset;
        unmap(child_node);
    }

    // Link siblings.
    split_node->left = index_node->offset;
    split_node->right = index_node->right;
    index_node->right = split_node->offset;
    if (split_node->right != 0) {
        IndexNode* new_sibling = map<IndexNode>(split_node->right);
        new_sibling->left = split_node->offset;
        unmap<IndexNode>(new_sibling);
    }
    return split_node;
}

inline int BPlusTree::get_index_from_leaf_node(LeafNode* leaf_node,
                                            const char* key) const {
    int index = lower_bound(leaf_node->records, leaf_node->count, key);
    return index < static_cast<int>(leaf_node->count) &&
                                 std::strncmp(leaf_node->Key(index), key, kMaxKeySize) == 0
                         ? index
                         : -1;
}

std::vector<std::pair<std::string, std::string>> BPlusTree::get_range(
        const std::string& left_key, const std::string& right_key) const {
    std::vector<std::pair<std::string, std::string>> res;
    off_t of_leaf = get_leaf_offset(left_key.data());
    LeafNode* leaf_node = map<LeafNode>(of_leaf);
    int index = lower_bound(leaf_node->records, leaf_node->count, left_key.data());
    for (int i = index; i < leaf_node->count; ++i) {
        res.emplace_back(leaf_node->Key(i), leaf_node->Value(i));
    }

    of_leaf = leaf_node->right;
    bool finish = false;
    while (of_leaf != 0 && !finish) {
        LeafNode* right_leaf_node = map<LeafNode>(of_leaf);
        for (int i = 0; i < right_leaf_node->count; ++i) {
            if (strncmp(right_leaf_node->Key(i), right_key.data(), kMaxKeySize) <=
                    0) {
                res.emplace_back(right_leaf_node->Key(i), right_leaf_node->Value(i));
            } else {
                finish = true;
                break;
            }
        }
        of_leaf = right_leaf_node->right;
        unmap(right_leaf_node);
    }

    unmap(leaf_node);
    return res;
}

bool BPlusTree::empty() const { return meta_->size == 0; }

size_t BPlusTree::size() const { return meta_->size; }

// Try Borrow key from left sibling.
bool BPlusTree::borrow_from_left_leaf_sibling(LeafNode* leaf_node) {
    if (leaf_node->left == 0) return false;
    LeafNode* sibling = map<LeafNode>(leaf_node->left);
    if (sibling->parent != leaf_node->parent || sibling->count <= get_min_keys()) {
        if (sibling->parent == leaf_node->parent) {
            assert(sibling->count == get_min_keys());
        }
        unmap(sibling);
        return false;
    }
    // 1. Borrow last key from left sibling.
    leaf_node->InsertKVAtIndex(0, sibling->LastKey(), sibling->LastValue());
    --sibling->count;

    // 2. Update parent's key.
    IndexNode* parent_node = map<IndexNode>(leaf_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    parent_node->UpdateKey(index, leaf_node->FirstKey());
    unmap<IndexNode>(parent_node);
    unmap<LeafNode>(sibling);
    return true;
}

// Try Borrow key from right sibling.
bool BPlusTree::borrow_from_right_leaf_sibling(LeafNode* leaf_node) {
    if (leaf_node->right == 0) return false;
    LeafNode* sibling = map<LeafNode>(leaf_node->right);

    if (sibling->parent != leaf_node->parent || sibling->count <= get_min_keys()) {
        if (sibling->parent == leaf_node->parent) {
            assert(sibling->count == get_min_keys());
        }
        unmap(sibling);
        return false;
    }

    // 1. Borrow frist key from right sibling.
    leaf_node->UpdateKV(leaf_node->count++, sibling->FirstKey(),
                                            sibling->FirstValue());
    sibling->DeleteKVAtIndex(0);

    // 2. Update parent's key.
    IndexNode* parent_node = map<IndexNode>(leaf_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    parent_node->UpdateKey(index - 1, sibling->FirstKey());

    unmap<IndexNode>(parent_node);
    unmap<LeafNode>(sibling);
    return true;
}

inline bool BPlusTree::borrow_from_leaf_sibling(LeafNode* leaf_node) {
    assert(leaf_node->count == get_min_keys() - 1);
    assert(leaf_node->parent != 0);
    return borrow_from_left_leaf_sibling(leaf_node) ||
                 borrow_from_right_leaf_sibling(leaf_node);
}

// Try merge left leaf node.
bool BPlusTree::merge_left_leaf(LeafNode* leaf_node) {
    if (leaf_node->left == 0) return false;
    LeafNode* sibling = map<LeafNode>(leaf_node->left);
    if (sibling->parent != leaf_node->parent) {
        unmap(sibling);
        return false;
    }

    assert(sibling->count == get_min_keys());
    // 1. remove key from parent.
    IndexNode* parent_node = map<IndexNode>(leaf_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    parent_node->DeleteKeyAtIndex(index);

    // 2. Merge left sibling.
    leaf_node->MergeLeftSibling(sibling);

    // 3. Link new sibling.
    leaf_node->left = sibling->left;
    if (sibling->left != 0) {
        LeafNode* new_sibling = map<LeafNode>(sibling->left);
        new_sibling->right = leaf_node->offset;
        unmap(new_sibling);
    }

    unmap(parent_node);
    dealloc(sibling);
    return true;
}

// Try Merge right node.
bool BPlusTree::merge_right_leaf(LeafNode* leaf_node) {
    if (leaf_node->right == 0) return false;
    LeafNode* sibling = map<LeafNode>(leaf_node->right);
    if (sibling->parent != leaf_node->parent) {
        unmap(sibling);
        return false;
    }

    // 1. remove key from parent.
    IndexNode* parent_node = map<IndexNode>(leaf_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    parent_node->UpdateKey(index - 1, parent_node->Key(index));
    parent_node->DeleteKeyAtIndex(index);
    unmap(parent_node);

    // 2. Merge right sibling.
    leaf_node->MergeRightSibling(sibling);

    // 3. Link new sibling.
    leaf_node->right = sibling->right;
    if (sibling->right != 0) {
        LeafNode* new_sibling = map<LeafNode>(sibling->right);
        new_sibling->left = leaf_node->offset;
        unmap(new_sibling);
    }

    dealloc(sibling);
    return true;
}

inline BPlusTree::LeafNode* BPlusTree::merge_leaf(LeafNode* leaf_node) {
    // Merge left node to leaf_node or right node to leaf_node.
    assert(leaf_node->count == get_min_keys() - 1);
    assert(leaf_node->parent != 0);
    assert(meta_->root != leaf_node->offset);
    assert(merge_left_leaf(leaf_node) || merge_right_leaf(leaf_node));
    return leaf_node;
}

// Try Swap key between index_node's left sibling and index_node's parent.
bool BPlusTree::borrow_from_left_index_sibling(IndexNode* index_node) {
    if (index_node->left == 0) return false;
    IndexNode* sibling = map<IndexNode>(index_node->left);
    if (sibling->parent != index_node->parent || sibling->count <= get_min_keys()) {
        if (sibling->parent == index_node->parent) {
            assert(sibling->count == get_min_keys());
        }
        unmap(sibling);
        return false;
    }

    // 1.Insert parent'key to the first of index_node's keys.
    IndexNode* parent_node = map<IndexNode>(index_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    index_node->InsertKeyAtIndex(0, parent_node->Key(index));

    // 2. Change parent's key.
    parent_node->UpdateKey(index, sibling->LastKey());

    // 3. Link sibling's last child to index_node,
    // and delete sibling's last child.
    Node* last_sibling_child =
            map<Node>(sibling->indexes[sibling->count--].offset);
    index_node->indexes[0].offset = last_sibling_child->offset;
    last_sibling_child->parent = index_node->offset;

    unmap(last_sibling_child);
    unmap(parent_node);
    unmap(sibling);
    return true;
}

bool BPlusTree::borrow_from_right_index_sibling(IndexNode* index_node) {
    if (index_node->right == 0) return false;
    IndexNode* sibling = map<IndexNode>(index_node->right);
    if (sibling->parent != index_node->parent || sibling->count <= get_min_keys()) {
        if (sibling->parent == index_node->parent) {
            assert(sibling->count == get_min_keys());
        }
        unmap(sibling);
        return false;
    }

    // 1.Insert parent‘key to the last of index_node's keys.
    IndexNode* parent = map<IndexNode>(index_node->parent);
    int index = upper_bound(parent->indexes, parent->count, sibling->LastKey());
    index_node->UpdateKey(index_node->count++, parent->Key(index - 1));

    // 2. Change parent's key.
    parent->UpdateKey(index - 1, sibling->FirstKey());

    // 3. Link index_node's last child to sibling's first child,
    // and delete sibling's first child.
    Node* first_sibling_child = map<Node>(sibling->indexes[0].offset);
    index_node->indexes[index_node->count].offset = first_sibling_child->offset;
    first_sibling_child->parent = index_node->offset;
    sibling->DeleteKeyAtIndex(0);

    unmap(first_sibling_child);
    unmap(parent);
    unmap(sibling);
    return true;
}

inline bool BPlusTree::borrow_from_index_sibling(IndexNode* index_node) {
    assert(index_node->count == get_min_keys() - 1);
    return borrow_from_left_index_sibling(index_node) ||
                 borrow_from_right_index_sibling(index_node);
}

// Try merge left index node.
bool BPlusTree::merge_left_index(IndexNode* index_node) {
    if (index_node->left == 0) return false;
    IndexNode* sibling = map<IndexNode>(index_node->left);
    if (sibling->parent != index_node->parent) {
        unmap(sibling);
        return false;
    }

    assert(sibling->count == get_min_keys());
    // 1. Merge left sibling to index_node.
    index_node->MergeLeftSibling(sibling);

    // 2. Link sibling's childs to index_node.
    for (size_t i = 0; i < sibling->count + 1; ++i) {
        Node* child_node = map<Node>(sibling->indexes[i].offset);
        child_node->parent = index_node->offset;
        unmap(child_node);
    }

    // 3. Link new sibling.
    index_node->left = sibling->left;
    if (sibling->left != 0) {
        IndexNode* new_sibling = map<IndexNode>(sibling->left);
        new_sibling->right = index_node->offset;
        unmap(new_sibling);
    }

    // 4. Update index_node's mid key.
    IndexNode* parent_node = map<IndexNode>(index_node->parent);
    int index =
            upper_bound(parent_node->indexes, parent_node->count, sibling->LastKey());
    index_node->UpdateKey(sibling->count, parent_node->Key(index));

    // 5. remove parent's key.
    parent_node->DeleteKeyAtIndex(index);

    dealloc(sibling);
    return true;
}

// Try merge right index node.
bool BPlusTree::merge_right_index(IndexNode* index_node) {
    if (index_node->right == 0) return false;
    IndexNode* sibling = map<IndexNode>(index_node->right);
    if (sibling->parent != index_node->parent) {
        unmap(sibling);
        return false;
    }

    assert(sibling->count == get_min_keys());
    // 1. Update index_node's last key.
    IndexNode* parent = map<IndexNode>(index_node->parent);
    int index = upper_bound(parent->indexes, parent->count, sibling->LastKey());
    index_node->UpdateKey(index_node->count++, parent->Key(index - 1));

    // 2. Merge right sibling to index_node.
    index_node->MergeRightSibling(sibling);

    // 3. Link sibling's childs to index_node.
    for (size_t i = 0; i < sibling->count + 1; ++i) {
        Node* child_node = map<Node>(sibling->indexes[i].offset);
        child_node->parent = index_node->offset;
        unmap(child_node);
    }

    // 4. Link new sibling.
    index_node->right = sibling->right;
    if (sibling->right != 0) {
        IndexNode* new_sibling = map<IndexNode>(sibling->right);
        new_sibling->left = index_node->offset;
        unmap(new_sibling);
    }

    // 5. remove parent's key.
    parent->UpdateKey(index - 1, parent->Key(index));
    parent->DeleteKeyAtIndex(index);

    dealloc(sibling);
    return true;
}

inline BPlusTree::IndexNode* BPlusTree::merge_index(IndexNode* index_node) {
    assert(index_node->count == get_min_keys() - 1);
    assert(index_node->parent != 0);
    assert(meta_->root != index_node->offset);
    assert(merge_left_index(index_node) || merge_right_index(index_node));
    return index_node;
}

#ifdef DEBUG
#include <queue>
void BPlusTree::dump() {
    std::vector<std::vector<std::vector<std::string>>> res(
            5, std::vector<std::vector<std::string>>());
    std::queue<std::pair<off_t, int>> q;
    q.emplace(meta_->root, 1);
    while (!q.empty()) {
        auto cur = q.front();
        q.pop();
        if (cur.second < meta_->height) {
            IndexNode* index_node = map<IndexNode>(cur.first);
            std::vector<std::string> v;
            for (int i = 0; i < index_node->count + 1; ++i) {
                if (i == index_node->count) {
                    v.push_back("");
                } else {
                    v.push_back(index_node->indexes[i].key);
                }
                if (index_node->indexes[i].offset != 0) {
                    q.emplace(index_node->indexes[i].offset, cur.second + 1);
                }
            }
            res[cur.second].push_back(v);
            unmap(index_node);
        } else {
            LeafNode* leaf_node = map<LeafNode>(cur.first);
            std::vector<std::string> v;
            for (int i = 0; i < leaf_node->count; ++i) {
                v.push_back(leaf_node->records[i].key);
            }
            res[cur.second].push_back(v);
            unmap(leaf_node);
        }
    }

    for (int i = 1; i <= meta_->height; ++i) {
        for (int j = 0; j < meta_->height - i; ++j) {
            LOG2("%s", "\t");
        }
        for (auto& v : res[i]) {
            for (auto& k : v) {
                LOG2("%s,", k.data());
            }
            LOG2("%s", "    ");
        }
        LOG2("%s", "\n");
    }
}
#endif