#pragma once
#include <memory>
#include <vector>
#include <cstdint>
using namespace std;


std::string to_string(uint64_t bits, int len);
uint64_t bitswap(uint64_t bits, int len);

struct Huffman_Node{
    int character;
    int frequency;
    bool is_leaf;
    shared_ptr<Huffman_Node> left = nullptr;
    shared_ptr<Huffman_Node> right = nullptr;
    Huffman_Node(int character, int frequency, bool is_leaf)
        : character(character), frequency(frequency), is_leaf(is_leaf) {}
};

struct Huffman_Dict{
    uint64_t bits;
    int len = 0;
    int character;
    Huffman_Dict(int bits, int len, int character):
        bits(bits), len(len), character(character) {};

};

struct Huffman_Comparator {
    bool operator()(const shared_ptr<Huffman_Node> a, const shared_ptr<Huffman_Node> b) const {
        return a->frequency > b->frequency; //lower frequency = higher priority
    }
};