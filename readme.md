Implementation of [Huffman compression](https://en.wikipedia.org/wiki/Huffman_coding) in C++. 

Gets about 1.75:1 compression ratio on english text.

Compilation:

g++ -g main.cpp -o huffman.exe

Usage:
huffman.exe -c shakespeare.txt compressedfile.dat

huffman.exe -d compressedfile.dat decompressed.txt

On shakespeare.txt:
5458199 Bytes Original 
3157510 Bytes Compressed
Compression ratio: 1.72864:1