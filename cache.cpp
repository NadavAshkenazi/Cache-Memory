//
// Created by nadav ashkenazi on 15/12/2020.
//

#include <map>
#include <math.h>
#include <vector>
#include <string>
#include <map>
#include <stdio.h>
#include <iostream>
#include "list"

using namespace std;

enum HIERARCHY {L1, L2, MEM};
enum OPERATION {READ, WRITE};


class Entry{
public:
    const int address;
    const int tag;
    const int set;
    bool dirtyBit;
    bool validBit;
    const unsigned int wayIndex;
};

class CacheHierarchy{
public:
    const unsigned int lSize;
    const unsigned int lAssoc;
    const unsigned int lCyc;
    map<int,list<Entry>> L;
    CacheHierarchy(unsigned int lSize = 0, unsigned int lAssoc =0, unsigned int lCyc = 0) :    lSize(lSize),
                                                                                    lAssoc(lAssoc),
                                                                                    lCyc(lCyc){
        this->L = map<int,list<Entry>>();
    }
    bool snoop(int address);
    void updateByLRU(int address);
    void add(int address);
    Entry remove(int address;
    bool isFull();
    Entry removeLast();
    void updateDirty(int address, bool isDirty);
};


class Cache{
private:
    int memCyc;
    unsigned int bSize;
    unsigned int wrAllocate;
    void addToL1(int address, OPERATION op);
    void addToL2(int address, OPERATION op);
    CacheHierarchy l1;
    CacheHierarchy l2;
public:
    Cache(int memCyc, unsigned int bSize, unsigned int wrAllocate, unsigned int l1Size, unsigned int l1Assoc,
          unsigned int l1Cyc, unsigned int l2Size, unsigned int l2Assoc, unsigned int l2Cyc) : memCyc(memCyc),
                                                                                            bSize(bSize),
                                                                                            wrAllocate(wrAllocate),
                                                                                            l1(l1Size, l1Assoc, l1Cyc),
                                                                                            l2(l2Size, l2Assoc, l2Cyc){}
    HIERARCHY inCache(int address);
    void update(int address, OPERATION op);
    void updateLRUAll(int address){
        l1.updateByLRU(address);
        l2.updateByLRU(address);
    }
};

HIERARCHY Cache::inCache(int address){
    if (this->l1.snoop(address))
        return L1;
    if (this->l2.snoop(address))
        return L2;
    return MEM;
}

void Cache::update(int address, OPERATION op) {
    HIERARCHY location = this->inCache(address);
    switch (location) {
        case L1:
            updateLRUAll(address);
        case L2:
            if (op == READ || this->wrAllocate) {
                addToL1(address, op);
                updateLRUAll(address);
            }
            else{  // op == WRITE with no Write Allocate
                l2.updateDirty(address, true);
                l2.updateByLRU(address);
            }

        case MEM:
            if (op == READ || this->wrAllocate){
                addToL2(address, op);
                addToL1(address, op);
            }
            else
                return; //writen only to mem
    }
}

void Cache::addToL1(int address, OPERATION op){
    if (op == READ || wrAllocate){
        if (l1.isFull()){
            Entry l1Remove = l1.remove(address);
            l2.updateDirty(l1Remove.address, true);
        }
        l1.add(address);
        l1.updateByLRU(address);
    }
    else // op == WRITE with no Write Allocate
        return; //writen only to mem
}

void Cache::addToL2(int address, OPERATION op){
    if (op == READ || wrAllocate){
        if (l2.isFull()){
            Entry l2Remove = l2.removeLast();
            if (l1.snoop(l2Remove.address)){
                Entry l1Remove = l1.remove(address);
                // if l1Remove is dirty write its value to mem, else write l2Remove
            }
        }
        l2.add(address);
        l2.updateByLRU(address);
    }
    else{ // op == WRITE with no Write Allocate
            return; //writen only to mem

    }
}