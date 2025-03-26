# Indexer
## ***A powerful tool to index your project***
> Suitable for Golang, Python, Java, JavaScript

### How to use

#### 1. Install dependencies

```bash
git submodule update --init
cd thirdparty

cd tree-sitter
make && make install

cd tree-sitter-python
make && make install

cd tree-sitter-go
make && make install

sudo ldconfig
```

#### 2. Build and install indexer

```bash
mkdir build
cd build
cmake .. && make && make install
```
