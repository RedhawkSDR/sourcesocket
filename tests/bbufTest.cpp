#include<iostream>
#include<vector>
#include "../cpp/CircularBuffer.h"

//gcc -Wall -O3 -pthread test.cpp -lboost_thread-mt -o test.exe

int main(){

    bounded_buffer<char> cb(5);
    std::vector<char> readVec;
    std::vector<char> writeVec;

    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;

    writeVec.push_back('A');
    writeVec.push_back('B');
    cb.write((char*)&writeVec[0],writeVec.size());
    std::cout<<"wrote two elements: A, B"<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    readVec.resize(1);
    cb.read((char*)&readVec[0],readVec.size());
    std::cout<<"read 1 element: "<<readVec[0]<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    writeVec.resize(0);
    writeVec.push_back('c');
    cb.write((char*)&writeVec[0],writeVec.size());
    std::cout<<"wrote 1 element: c"<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    writeVec.resize(0);
    writeVec.push_back('D');
    writeVec.push_back('E');
    writeVec.push_back('F');
    cb.write((char*)&writeVec[0],writeVec.size());
    std::cout<<"wrote 3 bytes: D, E, F"<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    readVec.resize(3);
    cb.read((char*)&readVec[0],readVec.size());
    std::cout<<"read 3 elements: "<<readVec[0]<<readVec[1]<<readVec[2]<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    writeVec.resize(0);
    writeVec.push_back('g');
    writeVec.push_back('h');
    writeVec.push_back('i');
    cb.write((char*)&writeVec[0],writeVec.size());
    std::cout<<"wrote 3 bytes: g, h, i"<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    readVec.resize(5);
    cb.read((char*)&readVec[0],readVec.size());
    std::cout<<"read 5 elements: "<<readVec[0]<<readVec[1]<<readVec[2]<<readVec[3]<<readVec[4]<<std::endl;
    std::cout<<"cb.size="<<cb.size()<<" capacity="<<cb.capacity()<<" full="<<cb.full()<<" empty="<<cb.empty()<<std::endl;
    cb.dump();

    bounded_buffer<long> cb2(5);
    std::vector<long> longVec;
    std::cout<<"cb2.size="<<cb2.size()<<" capacity="<<cb2.capacity()<<" full="<<cb2.full()<<" empty="<<cb2.empty()<<std::endl;
    longVec.push_back(1024*1);
    longVec.push_back(1024*2);
    longVec.push_back(1024*3);
    longVec.push_back(1024*4);
    cb2.write(&longVec[0],longVec.size());
    std::cout<<"wrote 4 longs: 1024, 2048, 3072, 4096"<<std::endl;
    std::cout<<"cb2.size="<<cb2.size()<<" capacity="<<cb2.capacity()<<" full="<<cb2.full()<<" empty="<<cb2.empty()<<std::endl;
    cb2.dump();

    longVec.resize(3);
    cb2.read(&longVec[0],longVec.size());
    std::cout<<"read 3 elements: "<<longVec[0]<<","<<longVec[1]<<","<<longVec[2]<<std::endl;
    std::cout<<"cb2.size="<<cb2.size()<<" capacity="<<cb2.capacity()<<" full="<<cb2.full()<<" empty="<<cb2.empty()<<std::endl;
    cb2.dump();

    longVec.resize(0);
    longVec.push_back(1024*5);
    longVec.push_back(1024*6);
    longVec.push_back(1024*7);
    longVec.push_back(1024*8);
    longVec.push_back(1024*9);
    longVec.push_back(1024*10);
    std::cout<<"longVec, size: "<<longVec.size()<< " elements: "<<longVec[0]<<","<<longVec[1]<<","<<longVec[2]<<","<<longVec[3]<<","<<longVec[4]<<","<<longVec[5]<<std::endl;
    size_t count = cb2.write(&longVec[0],longVec.size());
    std::cout<<"tried to write "<<longVec.size()<<" but actually wrote "<<count<<" longs: 5k, 6k, 7k, 8k, 9k, 10k"<<std::endl;
    std::cout<<"cb2.size="<<cb2.size()<<" capacity="<<cb2.capacity()<<" full="<<cb2.full()<<" empty="<<cb2.empty()<<std::endl;
    cb2.dump();

    longVec.resize(0);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    longVec.push_back(1);
    count = cb2.read(&longVec[0],longVec.size());
    std::cout<<"tried to read"<<longVec.size()<< " but actually read "<<count<<" elements: "<<longVec[0]<<","<<longVec[1]<<","<<longVec[2]<<","<<longVec[3]<<","<<longVec[4]<<","<<longVec[5]<<","<<longVec[6]<<","<<longVec[7]<<std::endl;
    std::cout<<"cb2.size="<<cb2.size()<<" capacity="<<cb2.capacity()<<" full="<<cb2.full()<<" empty="<<cb2.empty()<<std::endl;
    cb2.dump();

    return 0;
}
