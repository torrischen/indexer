#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <iostream>

namespace bptree {

/* predefined B+ info */
#define BP_ORDER 20

/* key/value type */
struct value_t {
    char v[128];

    value_t(const char *str = "") {
        bzero(v, sizeof(v));
        strncpy(v, str, sizeof(v) - 1);
    }

    value_t(const std::string &str) {
        bzero(v, sizeof(v));
        strncpy(v, str.c_str(), sizeof(v) - 1);
    }

    operator bool() const {
        return strcmp(v, "");
    }
};
struct key_t {
    char k[64];

    key_t(const char *str = "") {
        bzero(k, sizeof(k));
        strcpy(k, str);
    }

    key_t(const std::string &str) {
        bzero(k, sizeof(k));
        strcpy(k, str.c_str());
    }

    operator bool() const {
        return strcmp(k, "");
    }
};

int keycmp(const key_t &a, const key_t &b) {
    int x = strlen(a.k) - strlen(b.k);
    return x == 0 ? strcmp(a.k, b.k) : x;
}

#define OPERATOR_KEYCMP(type) \
    bool operator< (const key_t &l, const type &r) {\
        return keycmp(l, r.key) < 0;\
    }\
    bool operator< (const type &l, const key_t &r) {\
        return keycmp(l.key, r) < 0;\
    }\
    bool operator== (const key_t &l, const type &r) {\
        return keycmp(l, r.key) == 0;\
    }\
    bool operator== (const type &l, const key_t &r) {\
        return keycmp(l.key, r) == 0;\
    }

}

#endif /* end of PREDEFINED_H */