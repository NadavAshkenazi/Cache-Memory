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

unsigned int getTag(uint32_t address, int numOfSetBits, int bSize){
    uint32_t mask = 0;
    for (int i = 0; i < numOfSetBits + bSize; i++){
        mask = mask << 1;
        mask += 1;
    }
    uint32_t res = address & (~mask);
    return res;
}


unsigned int getSet(uint32_t address, int numOfSetBits, int bSize){
    uint32_t mask = 0;
    for (int i = 0; i < numOfSetBits; i++){
        mask = mask << 1;
        mask += 1;
    }
    for (int i = 0; i <  bSize; i++){
        mask = mask << 1;
    }
    uint32_t res = address & mask;
    return res;
}

unsigned int getOffset(uint32_t address, int numOfSetBits, int bSize){
    uint32_t mask = 0;

    for (int i = 0; i < bSize; i++){
        mask = mask << 1;
        mask += 1;
    }
    return address & mask;
}


class Entry{
public:
    const uint32_t address;
    bool dirtyBit;
    bool validBit;
    Entry(uint32_t address, bool dirtyBit = 0, bool validBit =1) : address(address), dirtyBit(dirtyBit), validBit(validBit){}
};





class CacheHierarchy{
public:
    const unsigned int numOfSetBits;
    const unsigned int lAssoc;
    const unsigned int lCyc;
    const unsigned int bSize;
    map<int,list<Entry>> L;
    CacheHierarchy(unsigned int lSize = 0, unsigned int lAssoc =0, unsigned int lCyc = 0, unsigned int bSize =0) :
            numOfSetBits(lSize - bSize - lAssoc),
            lAssoc(lAssoc),
            lCyc(lCyc),
            bSize(bSize){
        this->L = map<int,list<Entry>>();
    }
    bool snoop(uint32_t address);
    void add(uint32_t address);
    void updateByLRU(uint32_t address);
    Entry* remove(uint32_t address);
    Entry* removeLast(uint32_t address);
    bool isSetFull(uint32_t address);
    void updateDirty(uint32_t address, bool isDirty);
};

bool CacheHierarchy::snoop(uint32_t address){

    list<Entry> temp = L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp.begin(); it != temp.end(); it++){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize))
            return true;
    }
    return false;
}

void CacheHierarchy::add(uint32_t address){
    Entry entry = Entry(address);
    L[getSet(address, numOfSetBits, bSize)].push_front(entry);
}


void CacheHierarchy::updateByLRU(uint32_t address){
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); it++){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize)){
            Entry entry = Entry(*it);
            temp->erase(it);
            temp->push_front(entry);
        }
    }
}

Entry* CacheHierarchy::remove(uint32_t address){
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); it++){
        if (getTag(address,numOfSetBits,bSize) == getTag(it->address, numOfSetBits, bSize)){
            Entry* entry = new Entry(*it);
            temp->erase(it);
            return entry;
        }
    }
    return NULL;
}

Entry* CacheHierarchy::removeLast(uint32_t address) {
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator lastElement = temp->end();
    lastElement--;
    Entry* entry = new Entry(*lastElement);
    temp->erase(lastElement);
    return entry;
}

bool CacheHierarchy::isSetFull(uint32_t address) {
    int waysNum = pow(2,lAssoc);
    if (L[getSet(address, numOfSetBits, bSize)].size() > waysNum)
        throw std::exception();
    if (L[getSet(address, numOfSetBits, bSize)].size() == waysNum)
        return true;
    return false;
}

void CacheHierarchy::updateDirty(uint32_t address, bool isDirty) {
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); it++){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize)){
            it->dirtyBit = isDirty;
        }
    }
}



class Cache{
public:
    int memCyc;
    unsigned int bSize;
    unsigned int wrAllocate;
    unsigned int l1accesses;
    unsigned int l1Misses;
    unsigned int l2Misses;
    void addToL1(uint32_t address, OPERATION op);
    void addToL2(uint32_t address, OPERATION op);
    CacheHierarchy l1;
    CacheHierarchy l2;
    Cache(int memCyc, unsigned int bSize, unsigned int wrAllocate, unsigned int l1Size, unsigned int l1Assoc,
          unsigned int l1Cyc, unsigned int l2Size, unsigned int l2Assoc, unsigned int l2Cyc) : memCyc(memCyc),
                                                                                            bSize(bSize),
                                                                                            wrAllocate(wrAllocate),
                                                                                            l1(l1Size, l1Assoc, l1Cyc, bSize),
                                                                                            l2(l2Size, l2Assoc, l2Cyc, bSize),
                                                                                            l1accesses(0),
                                                                                            l1Misses(0), l2Misses(0){}
    HIERARCHY inCache(uint32_t address);
    void update(uint32_t address, OPERATION op);
    double getL1MissRate(){
        return double(l1Misses)/double(l1accesses);
    }
    double getL2MissRate(){
        return double(l2Misses)/double(l1Misses);
    }
    double accTimeAVG(){
        return (double((l1accesses * l1.lCyc) + (l1Misses * l2.lCyc) + (l2Misses * memCyc)) / double(l1accesses));
    }
};

HIERARCHY Cache::inCache(uint32_t address){
    if (this->l1.snoop(address))
        return L1;
    if (this->l2.snoop(address))
        return L2;
    return MEM;
}

void Cache::update(uint32_t address, OPERATION op) {
    l1accesses++;
    HIERARCHY location = this->inCache(address);
    if(location == L1) {
        l1.updateByLRU(address);
        l1.updateDirty(address, true);
    }
    else if (location == L2){
        l1Misses++;
        if (op == READ || this->wrAllocate) {
            addToL1(address, op);
            l1.updateByLRU(address);
            l1.updateDirty(address, true);
            l2.updateByLRU(address);
        }
        else{  // op == WRITE with no Write Allocate
            l2.updateByLRU(address);
            l2.updateDirty(address, true);
        }
    }
    else {
        l1Misses++;
        l2Misses++;
        if (op == READ || this->wrAllocate){
            addToL2(address, op);
            addToL1(address, op);
            l1.updateByLRU(address);
            l1.updateDirty(address, true);
            l2.updateByLRU(address);
        }
        else
            return; //writen only to mem
    }
}

void Cache::addToL1(uint32_t address, OPERATION op){
    if (op == READ || wrAllocate){
        if (l1.isSetFull(address)){
            Entry l1Remove = *l1.removeLast(address);
            if (l1Remove.dirtyBit){
                l2.updateDirty(l1Remove.address, true);
            }
        }
        l1.add(address);
        l1.updateByLRU(address);
    }
    else // op == WRITE with no Write Allocate
        return; //writen only to mem
}

void Cache::addToL2(uint32_t address, OPERATION op){
    if (op == READ || wrAllocate){
        if (l2.isSetFull(address)){
            Entry l2Remove = *l2.removeLast(address);
            if (l1.snoop(l2Remove.address)){
                Entry l1Remove = *l1.remove(l2Remove.address);
                // if l1Remove is dirty write its value to mem, else write l2Remove
            }
        }
        l2.add(address);
        l2.updateByLRU(address);
    }
    else{ // op == WRITE with no Write Allocate
            return;

    }
}