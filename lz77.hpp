#pragma once
#include <memory>
#include <vector>
#include <cstdint>
using namespace std;

const int HASH_SIZE =  1 << 15;
const int WINDOW_SIZE = 1 << 15;
const int WINDOW_MASK = WINDOW_SIZE - 1;
const int MAX_LOOKAHEAD = 128;
const int MAX_MATCHES = 32;

struct LZ77_Token{
    uint32_t distance;
    uint32_t length;
    bool isLiteral;
    char literal; 
};

class LZ77_hashchain{
    public:
        int32_t head[HASH_SIZE] = {};
        int32_t prev[WINDOW_SIZE] = {};
        void insert(uint8_t b1, uint8_t b2, uint8_t b3, uint32_t pos);
        LZ77_Token find_match(const vector<char>& window, const vector<char>& lookahead_buff, int32_t pos);
};

vector <char> lz77_decompress(vector <LZ77_Token>& input_tokens);
vector <LZ77_Token> lz77_compress(const vector<char>& input_buff);