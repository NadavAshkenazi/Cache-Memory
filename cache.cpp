//
// Created by nadav ashkenazi on 15/12/2020.
//

#include <map>
#include <math.h>
#include <vector>
#include <stdio.h>
#include <iostream>
#include "list"

using namespace std;

enum HIERARCHY {L1, L2, MEM};
enum OPERATION {READ, WRITE};

/**
 * returns the tag which is the bits from #(blockBits + setBits) until the end  where every bit not
 * used for tag 0;
 * @param address - address to extract tag from
 * @param numOfSetBits - number of bit used for deciding set
 * @param bSize -block size
 * @return the relevant tag
 */
unsigned int getTag(uint32_t address, int numOfSetBits, int bSize){
    uint32_t mask = 0;
    for (int i = 0; i < numOfSetBits + bSize; i++){
        mask = mask << 1;
        mask += 1;
    }
    uint32_t res = address & (~mask);
//    cout << "numOfBits " << numOfSetBits << " address: " << address << " tag: " << res << endl; //todo: debug
    return res;
}

/**
 * returns the set which is the bits from #(blockBits) until #(blockBits+setBits) as a 32 bit address where every bit not
 * used for set is 0;
 * @param address -address to extract set from
 * @param numOfSetBits - number of bit used for deciding set
 * @param bSize -block size
 * @return the relevant set
 */
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

/**
 * returns the offset which is the bits from the start until #(blockBits) as a 32 bit address where every bit not
 * used for offset is 0;
 * @param address -address to extract set from
 * @param bSize -block size
 * @return the relevant set
 */
unsigned int getOffset(uint32_t address, int bSize){
    uint32_t mask = 0;

    for (int i = 0; i < bSize; i++){
        mask = mask << 1;
        mask += 1;
    }
    return address & mask;
}

/**
 * a Class that hold the relevant data for an entry inside the cache
 */
class Entry{
public:
    const uint32_t address;
    bool dirtyBit;
    bool validBit;
    Entry(uint32_t address, bool dirtyBit = 0, bool validBit =1) : address(address), dirtyBit(dirtyBit), validBit(validBit){}
};


/**
 * a Class that represents a single hierarchy inside the cache memory such as L1 or L2
 */
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

/**
 * Checks if the data in address is in the cache hierarchy
 * @param address for check
 * @return true if the data is inside the cache hierarchy and false otherwise
 */
bool CacheHierarchy::snoop(uint32_t address){

    list<Entry> temp = L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp.begin(); it != temp.end(); ++it){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize))
            return true;
    }
    return false;
}

/**
 * adds the block holding address to the cache hierarchy
 * @param address to add to cache
 */
void CacheHierarchy::add(uint32_t address){
    Entry entry = Entry(address);
    L[getSet(address, numOfSetBits, bSize)].push_front(entry);
}

/**
 * updates the cache hierarchies LRU order to reflect that address was used last
 * @param address most recently used
 */
void CacheHierarchy::updateByLRU(uint32_t address){
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); ++it){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize)){
            Entry entry = Entry(*it);
            temp->erase(it);
            temp->push_front(entry);
            return;
        }
    }
}

/**
 * removes the entry that holds the block relevant to address from cache hierarchy
 * @param address for removal
 * @return pointer to the entry that was removed
 */
Entry* CacheHierarchy::remove(uint32_t address){
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); ++it){
        if (getTag(address,numOfSetBits,bSize) == getTag(it->address, numOfSetBits, bSize)){
            Entry* entry = new Entry(*it);
            temp->erase(it);
            return entry;
        }
    }
    return NULL;
}

/**
 * removes the last block in the LRU order In a relevant set.
 * @param address to decide which set needs removing from.
 * @return pointer to the entry that was removed
 */
Entry* CacheHierarchy::removeLast(uint32_t address) {
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator lastElement = temp->end();
    lastElement--;
    Entry* entry = new Entry(*lastElement);
    temp->erase(lastElement);
    return entry;
}

/**
 * checks if a relevant set is in capacity
 * @param address to decide which set needs checking.
 * @return true if set is full and false otherwise
 */
bool CacheHierarchy::isSetFull(uint32_t address) {
    int waysNum = pow(2,lAssoc);
    if (L[getSet(address, numOfSetBits, bSize)].size() > waysNum)
        throw std::exception();
    if (L[getSet(address, numOfSetBits, bSize)].size() == waysNum)
        return true;
    return false;
}

/**
 * changes the dirty bit of a given block to a given state
 * @param address to decide the block
 * @param isDirty wanted state
 */
void CacheHierarchy::updateDirty(uint32_t address, bool isDirty) {
    list<Entry>* temp = &L[getSet(address, numOfSetBits, bSize)];
    std::list<Entry>::iterator it;
    for (it = temp->begin(); it != temp->end(); ++it){
        if (getTag(address, numOfSetBits, bSize) == getTag(it->address, numOfSetBits, bSize)){
            it->dirtyBit = isDirty;
        }
    }
}


/**
 * a Class that represents an entire cache memory
 */
class Cache{
public:
    int memCyc;
    unsigned int bSize;
    unsigned int wrAllocate;
    unsigned int l1accesses;
    unsigned int l1Misses;
    unsigned int l2Misses;
    uint32_t addToL1(uint32_t address, OPERATION op);
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

/**
 * checks if the data in a given address is kept in the cache memory
 * @param address for checking
 * @return true if a block holding the data is in the cache memory false otherwise
 */
HIERARCHY Cache::inCache(uint32_t address){
    if (this->l1.snoop(address))
        return L1;
    if (this->l2.snoop(address))
        return L2;
    return MEM;
}

/**
 * updates the cache to hold a specific block by LRU order
 * @param address to dicide which block need keeping
 * @param op operation that was preformed on the block
 */
void Cache::update(uint32_t address, OPERATION op) {
//    cout << "address: " << address << " l2 tag: " << (getTag(address, l2.numOfSetBits, bSize) >> (l2.numOfSetBits+bSize)) << endl; //todo: debug
    l1accesses++;
    HIERARCHY location = this->inCache(address);
    if(location == L1) {
        l1.updateByLRU(address);
        if (op == WRITE)
            l1.updateDirty(address, true);
    }
    else if (location == L2){
        l1Misses++;
        if (op == READ || this->wrAllocate) {
            uint32_t L1Removed = addToL1(address, op);
            l1.updateByLRU(address);
            if (op == WRITE)
                l1.updateDirty(address, true);
            l2.updateByLRU(address);
            if (L1Removed != -1 and l2.snoop(L1Removed)){
                l2.updateDirty(L1Removed, true);
                l2.updateByLRU(L1Removed);
            }
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
            uint32_t L1Removed = addToL1(address, op);
            l1.updateByLRU(address);
            if (op == WRITE)
                l1.updateDirty(address, true);
            l2.updateByLRU(address);
            if (L1Removed != -1 and l2.snoop(L1Removed)){
                l2.updateDirty(L1Removed, true);
                l2.updateByLRU(L1Removed);
            }
        }
        else
            return; //writen only to mem
    }
}

/**
 * adds a specific block to L1, and removes the LRU block if necessary
 * @param address to decide which block needs to be added
 * @param op that was preformed on the address
 * @return a the address of the removed entry if there was one, -1 otherwise
 */
uint32_t Cache::addToL1(uint32_t address, OPERATION op){
    uint32_t retAdr = -1;
    if (op == READ || wrAllocate){
        if (l1.isSetFull(address)){
            Entry l1Remove = *l1.removeLast(address);
            if (l1Remove.dirtyBit){
                l2.updateDirty(l1Remove.address, true);
                l2.updateByLRU(l1Remove.address);
                retAdr = l1Remove.address;
            }
        }
        l1.add(address);
        l1.updateByLRU(address);
        return retAdr;
    }
    else // op == WRITE with no Write Allocate
        return -1; //writen only to mem
}


/**
 * adds a specific block to L2, and removes the LRU block if necessary
 * @param address to decide which block needs to be added
 * @param op that was preformed on the address
 */
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