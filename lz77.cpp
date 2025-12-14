#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <queue>
#include <cstdint>
#include <string>
#include <filesystem>
#include "lz77.hpp"
using namespace std;

inline uint32_t hash3(uint8_t a, uint8_t b, uint8_t c) {
    uint32_t h = a;
    h = (h << 5) ^ b;
    h = (h << 5) ^ c;
    return h & ((1 << 15) - 1);
}

//goal: replace repeated strings with references to the first string
//at each character, lookahead and behind. search context window for matches to next few characters.
//when you find a match, you need to indicate:
//this is not a literal
//the offset of the match
//the number of characters

void LZ77_hashchain::insert(uint8_t b1, uint8_t b2, uint8_t b3, uint32_t pos)
{
        uint32_t h = hash3(b1, b2, b3);
        prev[pos & WINDOW_MASK] = head[h];
        head[h] = pos;
}


LZ77_Token LZ77_hashchain::find_match(const vector<char>& window, const vector<char>& input_buff, int32_t pos)
{
    if (pos > input_buff.size() - 2) {
        throw; //we should not be looking past the end of the input buffer
    }
    uint32_t h = hash3(input_buff[pos], input_buff[pos+1], input_buff[pos+2]);
    int match_pos = head[h];
    uint32_t matches = 0, lookahead = 0, best_match_pos = 0, best_match_len = 0;
    while ((match_pos != 0) && (match_pos > pos - WINDOW_SIZE) && (matches < MAX_MATCHES)){
        lookahead = 0;
        while (
            (lookahead < MAX_LOOKAHEAD) 
            && (pos + lookahead < input_buff.size()) 
            && (match_pos + lookahead < input_buff.size())
        ){
            if (input_buff[pos+lookahead] != input_buff[match_pos + lookahead]) break;
            lookahead++;
        }
        if (lookahead > best_match_len) {
            best_match_len = lookahead;
            best_match_pos = match_pos;
        }
        match_pos = prev[match_pos & WINDOW_MASK];
        matches++;
    }
    if (best_match_len > 3){ //if we found a match greater than 3 characters, send it
        return {
            .distance = pos-best_match_pos,
            .length   = best_match_len,
            .isLiteral = false,
            .literal  = 0
        };
    } else { //otherwise return a literal
        return {
            .distance = 0,
            .length   = 1,
            .isLiteral = true,
            .literal  = input_buff[pos]
        };
    }

}
vector <char> lz77_decompress(vector <LZ77_Token>& input_tokens){
    vector <char> output;
    int pos = 0;
    for (LZ77_Token token: input_tokens){
        if (token.isLiteral){
            output.push_back(token.literal);
        } else {
            pos = output.size() - token.distance;
            for (int i = 0; i < token.length; i++){
                if (pos + i > output.size()) throw;
                output.push_back(output[pos + i]);
            }
        }
    }
    return output;
}
vector <LZ77_Token> lz77_compress(const vector<char>& input_buff){

    unique_ptr<LZ77_hashchain> chain = make_unique<LZ77_hashchain>();
    vector<char> window(WINDOW_SIZE, 0);
    
    vector<LZ77_Token> output;
    LZ77_Token match;
    int32_t window_pos = 0, i = 0, h = 0;
    while (i <= input_buff.size() - 3){
        
        //chain->insert(input_buff[i], input_buff[i+1], input_buff[i+2], i);
        match = chain->find_match(window, input_buff, i);
        output.push_back(match);
        for (int j = 0; j < match.length; j++){
            if (i + j <= input_buff.size() - 3){
                chain->insert(input_buff[i + j], input_buff[i + j + 1], input_buff[i + j + 2], i + j);
                window[window_pos] = input_buff[i + j];
                window_pos = (window_pos + 1) & WINDOW_MASK;
            }
        }
        i = i + match.length;
    }

    //Handle last few bytes where we don't have three bytes to calculate a hash. Just add them as literals.
    for (i; i < input_buff.size(); i++){
        output.push_back(LZ77_Token{
            .distance = 0,
            .length   = 1,
            .isLiteral = true,
            .literal  = input_buff[i]
        });
    }

    vector<char> final = lz77_decompress(output);
    
    cout << "Original size: " << input_buff.size() << " bytes\n";
    cout << "Compressed size: " << output.size() << " tokens\n";
    cout << "Decompressed size: " << final.size() << " bytes\n";

    if (input_buff.size() == final.size()){
        for (int i = 0; i < input_buff.size(); i++){
            if (input_buff[i] != final[i]){
                cout << "Difference at byte " << i << '\n';
            }
        }
    }
    return output;
}
