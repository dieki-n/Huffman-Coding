#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <queue>
#include <cstdint>
#include <string>
#include <filesystem>

#include "main.hpp"
#include "lz77.hpp"

using namespace std;

//deflate rfc: https://datatracker.ietf.org/doc/html/rfc1951

std::string to_string(uint64_t bits, int len) {
    std::string s;
    s.reserve(len);
    for (int i = len-1; i >= 0; i--) {
        s.push_back(((bits >> i) & 1) ? '1' : '0');
    }
    return s;
}

vector<char> load_file(string filename){
    ifstream indata;
    indata.open(filename, ios::binary | ios::ate);
    streamsize size = indata.tellg();
    indata.seekg(0);
    vector<char> buffer(size);
    indata.read(buffer.data(), size);
    return buffer;
}

void save_file(string filename, vector<char> data){
    std::ofstream out(filename, std::ios::binary);
    out.write(data.data(), data.size());
}

//bit order:
//start of tree = first bit of first byte of file
//bit 0 = first bit
//bit 1,2,3,4 = increasing order

//recursively parse the tree and create a dictionary of the codewords
// vector of struct
// struct contains:
//   vector<bool> of previous bits
//   character value (-1 to start)
//
//at each node:
//  add bit value to vector
//  if there are any nodes under us:
//    call again and pass them current vector of structs
//    combine their 

//if we are a leaf node:
//  add character to vector
//  return vector
uint64_t bitswap(uint64_t bits, int len){
    uint64_t t = 0;
    for (int i = 0; i < len; i++){
        t += ((bits >> i) & 1) << (len - i - 1);
    }
    return t;
}

void build_dictionary_recursive(
        shared_ptr<Huffman_Node> node, 
        vector<unique_ptr<Huffman_Dict>>& dict,
        int bits,
        int bits_len
    ){

    if (!node) return;

    if (node->is_leaf == 1){
        dict.push_back(make_unique<Huffman_Dict>(bits, bits_len, node->character));
    } else{
        build_dictionary_recursive(node->left, dict, bits + (0 << bits_len), bits_len + 1);
        build_dictionary_recursive(node->right, dict, bits + (1 << bits_len), bits_len + 1);
    }
}

//wrapper to create the dict and start the recursive call
vector<unique_ptr<Huffman_Dict>> build_dictionary(shared_ptr<Huffman_Node> root){
    vector<unique_ptr<Huffman_Dict>> dict;
    build_dictionary_recursive(root, dict, 0, 0);
    

    //Turn the dictionary into a canonical huffman dictionary for easy representation
    std::sort(dict.begin(), dict.end(),
          [](const unique_ptr<Huffman_Dict>& a, const unique_ptr<Huffman_Dict>& b) {
                if (a->len == b->len) return a->character > b->character;
                return a->len < b->len;
          });
    int current_bits = 0;
    int current_bitlen = dict[0]->len;

    for (int i = 0; i < dict.size(); i++){
        //dict[i]->bits = current_bits; 
        dict[i]->bits = bitswap(current_bits, current_bitlen);
        if (i < dict.size() - 1) { 
            current_bits = (current_bits + 1) << (dict[i + 1]->len - current_bitlen);
            current_bitlen = dict[i + 1]->len;
        }
    }

    return dict;
}

vector<char> pack_dictionary(vector<unique_ptr<Huffman_Dict>>& dict){
    vector<char> packed(257);
    for(auto& ch: dict){
        packed[ch->character] = ch->len;
    }
    return packed;
}

vector<unique_ptr<Huffman_Dict>> unpack_dictionary(vector<char>& packed){

    vector<unique_ptr<Huffman_Dict>> dict;
    for (int i = 0; i < 257; i++){ //we assume the first 257 bytes are the dictionary
        if (packed[i] > 0){
            dict.push_back(make_unique<Huffman_Dict>(0, packed[i], i));
        }
    }

    std::sort(dict.begin(), dict.end(),
          [](const unique_ptr<Huffman_Dict>& a, const unique_ptr<Huffman_Dict>& b) {
                if (a->len == b->len) return a->character > b->character;
                return a->len < b->len;
          });

    int current_bits = 0;
    int current_bitlen = dict[0]->len;

    for (int i = 0; i < dict.size(); i++){
        dict[i]->bits = bitswap(current_bits, current_bitlen);
        if (i < dict.size() - 1) { 
            current_bits = (current_bits + 1) << (dict[i + 1]->len - current_bitlen);
            current_bitlen = dict[i + 1]->len;
        }
    }
    return dict;
}

shared_ptr<Huffman_Node> create_tree(vector<char> buff){
    vector<int> count(257, 0);
    priority_queue<shared_ptr<Huffman_Node>, std::vector<shared_ptr<Huffman_Node>>, Huffman_Comparator> pq;
    
    //get the frequency counts
    for (int i = 0; i < buff.size(); i++){
        count[buff[i]]++;
    }
    count[256] = 1; //add EOF character

    //Create one leaf node for each character
    for (int i = 0; i < count.size(); i++){
        if (count[i] > 0) {
            pq.push(make_shared<Huffman_Node>(i, count[i], true));
        }
    }

    //Create the internal nodes of the tree
    while (pq.size() > 1) {
        shared_ptr<Huffman_Node> a = pq.top(); pq.pop();
        shared_ptr<Huffman_Node> b = pq.top(); pq.pop();
        shared_ptr<Huffman_Node> new_node = make_shared<Huffman_Node>(0, a->frequency + b->frequency, false);
        new_node->left = a;
        new_node->right = b;
        pq.push(new_node);
    }
    shared_ptr<Huffman_Node> root = pq.top();

    return root;
}

shared_ptr<Huffman_Node> create_tree_from_dict(vector<unique_ptr<Huffman_Dict>>& dict){
    auto root = make_shared<Huffman_Node>(0,0,false);
    shared_ptr<Huffman_Node> current = root;

    for (auto& ch : dict){
        for(int i = 0; i < ch->len; i++){
            if ((ch->bits >> i) & 1){
                if (!current->right) current->right = make_shared<Huffman_Node>(0,0,false);
                current = current->right;
            } else {
                if (!current->left) current->left = make_shared<Huffman_Node>(0,0,false);
                current = current->left;
            }
        }
        current->character = ch->character;
        current->is_leaf = true;
        current = root;
    }

    return root;
}



vector <char> compress(vector<char> input_buff){
    uint64_t bitstring_buff = 0;
    int bitstring_len = 0;

    auto tree = create_tree(input_buff);
    auto char_dict = build_dictionary(tree);
    vector<char> output_buff = pack_dictionary(char_dict);

    vector<unique_ptr<Huffman_Dict>> dict(257);
    for (auto& ch : char_dict){
        dict[ch->character] = move(ch);
    }

    for (int i = 0; i < input_buff.size(); i++){  
        bitstring_buff += dict[input_buff[i]]->bits << bitstring_len;
        bitstring_len += dict[input_buff[i]]->len;
        while (bitstring_len >= 8){
            output_buff.push_back(bitstring_buff & 255);
            bitstring_buff >>= 8;
            bitstring_len = bitstring_len - 8;
        }
    }

    //add EOF marker
    bitstring_buff += dict[256]->bits << bitstring_len;
    bitstring_len += dict[256]->len;
    while (bitstring_len > 0){
        output_buff.push_back(bitstring_buff & 255);
        bitstring_buff >>= 8;
        bitstring_len = bitstring_len - 8;
    }
    return output_buff;
}

vector <char> decompress(vector<char> input_buff){

    uint64_t codebook_buff = 0;
    int codebook_len = 0;

    vector<char> output_buff;
    auto dict = unpack_dictionary(input_buff);
    auto canonical_tree = create_tree_from_dict(dict);

    shared_ptr<Huffman_Node> current = canonical_tree;
    for (int i = 257; i < input_buff.size(); i++){  
        for (int j=0; j < 8; j++){
            if (((input_buff[i] >> j) & 1) == 0){
                current = current->left;
            } else{
                current = current->right;
            }
            if (current->is_leaf){
                if (current->character == 256) return output_buff;
                output_buff.push_back(current->character);
                current = canonical_tree;
            }
        }
    }
    return output_buff;
}

vector <char> deflate_compress(vector<char> input_buff){
    uint64_t bitstring_buff = 0;
    int bitstring_len = 0;

    auto tree = create_tree(input_buff);
    auto char_dict = build_dictionary(tree);
    vector<char> output_buff = pack_dictionary(char_dict);

    vector<unique_ptr<Huffman_Dict>> dict(257);
    for (auto& ch : char_dict){
        dict[ch->character] = move(ch);
    }

    for (int i = 0; i < input_buff.size(); i++){  
        bitstring_buff += dict[input_buff[i]]->bits << bitstring_len;
        bitstring_len += dict[input_buff[i]]->len;
        while (bitstring_len >= 8){
            output_buff.push_back(bitstring_buff & 255);
            bitstring_buff >>= 8;
            bitstring_len = bitstring_len - 8;
        }
    }

    //add EOF marker
    bitstring_buff += dict[256]->bits << bitstring_len;
    bitstring_len += dict[256]->len;
    while (bitstring_len > 0){
        output_buff.push_back(bitstring_buff & 255);
        bitstring_buff >>= 8;
        bitstring_len = bitstring_len - 8;
    }
    return output_buff;
}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: program [-c | -d] <input_file> <output_file>\n";
        return 1;
    }
    if (std::string(argv[1])== "-c"){
        if (!filesystem::exists(std::string(argv[2]))){
            cerr << argv[2] << " file not found";
            return 1;
        }
        vector <char> buff = load_file(std::string(argv[2]));        
        vector <char> output = compress(buff);
        cout << buff.size() << " Bytes Original \n";
        cout << output.size() << " Bytes Compressed \n";
        double ratio = static_cast<double>(buff.size()) / output.size(); 
        std::cout << "Compression ratio: " << ratio << ":1\n";
    
        save_file(std::string(argv[3]), output);
    } else if (std::string(argv[1]) == "-d"){

        if (!filesystem::exists(std::string(argv[2]))){
            cerr << argv[2] << " file not found";
            return 1;
        }

        vector <char> buff = load_file(std::string(argv[2]));
        vector <char> output = decompress(buff);
        save_file(std::string(argv[3]), output);
        cout << buff.size() << " Bytes Compressed \n";
        cout << output.size() << " Bytes Decompressed \n";

    } else {
        std::cerr << "Usage: program [-c | -d] <input_file> <output_file>\n";
        return 1;
    }

    return 0;
}