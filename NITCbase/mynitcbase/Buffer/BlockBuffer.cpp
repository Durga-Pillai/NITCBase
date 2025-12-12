

#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

int compareAttrs(Attribute attr1, Attribute attr2, int attrType) {

  double diff;
  if (attrType == STRING)
    return strcmp(attr1.sVal, attr2.sVal);
  else {
    if (attr1.nVal < attr2.nVal)
      return -1;
    else if (attr1.nVal > attr2.nVal)
      return 1;
    else
      return 0;
  }
}

BlockBuffer::BlockBuffer(int blockNum) { this->blockNum = blockNum; }

BlockBuffer::BlockBuffer(char blockTypeChar) {
  unsigned char *bufferPtr;
  int blockType = blockTypeChar == 'R'   ? REC
                  : blockTypeChar == 'I' ? IND_INTERNAL
                  : blockTypeChar == 'L' ? IND_LEAF
                                         : UNUSED_BLK;

  int freeBlockNum = getFreeBlock(blockType);

  if (freeBlockNum < 0 || blockNum >= DISK_BLOCKS) {
    std::cout << "Failed to get a free block" << std::endl;
    this->blockNum = freeBlockNum;
  } else {
    this->blockNum = freeBlockNum;
  }
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}


RecBuffer::RecBuffer() : BlockBuffer::BlockBuffer('R') {}

int BlockBuffer::getBlockNum() { return this->blockNum; }

int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;
  memcpy(&head->blockType, bufferPtr + 0, 4);
  memcpy(&head->pblock, bufferPtr + 4, 4);
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);

  return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  if (ret != SUCCESS) return ret;

  struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

  bufferHeader->blockType = head->blockType;
  bufferHeader->pblock = head->pblock;
  bufferHeader->lblock = head->lblock;
  bufferHeader->rblock = head->rblock;
  bufferHeader->numEntries = head->numEntries;
  bufferHeader->numAttrs = head->numAttrs;
  bufferHeader->numSlots = head->numSlots;

  int ret_ = StaticBuffer::setDirtyBit(this->blockNum);
  if (ret_ != SUCCESS) return ret_;

  return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  BlockBuffer::getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  unsigned char *bufferPtr;
  int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) return ret;

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + slotNum * recordSize;

  memcpy(rec, slotPointer, recordSize);
  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  unsigned char *buffer;
  int ret = RecBuffer::loadBlockAndGetBufferPtr(&buffer);

  if (ret != SUCCESS) return ret;

  struct HeadInfo head;
  BlockBuffer::getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  if (slotNum < 0 || slotNum >= slotCount) return E_OUTOFBOUND;

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = buffer + HEADER_SIZE + slotCount + slotNum * recordSize;

  memcpy(slotPointer, rec, recordSize);

  int ret_ = StaticBuffer::setDirtyBit(this->blockNum);

  if (ret_ != SUCCESS) std::cout << "Failed to set dirty bit" << std::endl;

  return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferPtr) {
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (blockNum == E_OUTOFBOUND) return E_OUTOFBOUND;

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  else {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
      if (bufferIndex == bufferNum)
        StaticBuffer::metainfo[bufferIndex].timeStamp = 0;
      else
        StaticBuffer::metainfo[bufferIndex].timeStamp++;
    }
  }

  *bufferPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;
  int result = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
  if (result != SUCCESS) {
    return result;
  }

  struct HeadInfo head;
  BlockBuffer::getHeader(&head);

  int numOfSlots = head.numSlots;
  memcpy(slotMap, bufferPtr + HEADER_SIZE, numOfSlots);

  return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;
  int result = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
  if (result != SUCCESS) {
    return result;
  }

  struct HeadInfo head;
  BlockBuffer::getHeader(&head);

  int numSlots = head.numSlots;
  memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

  int ret = StaticBuffer::setDirtyBit(this->blockNum);
  if (ret != SUCCESS) {
    return ret;
  }

  return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  (*(int32_t *)bufferPtr) = blockType;

  StaticBuffer::blockAllocMap[this->blockNum] = blockType;

  int ret_ = StaticBuffer::setDirtyBit(this->blockNum);
  if (ret_ != SUCCESS) {
    return ret_;
  }

  return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType) {
  int blockNum;
  for (blockNum = 0; blockNum < DISK_BLOCKS; blockNum++) {
    if (StaticBuffer::blockAllocMap[blockNum] == UNUSED_BLK) {
      break;
    }
  }

  if (blockNum == DISK_BLOCKS) {
    return E_DISKFULL;
  }

  this->blockNum = blockNum;

  int bufferIndex = StaticBuffer::getFreeBuffer(blockNum);
  if (bufferIndex < 0 || bufferIndex >= BUFFER_CAPACITY) {
    std::cout << "ERROR: Buffer Full" << std::endl;
    return bufferIndex;
  }

  HeadInfo newHeader;
  newHeader.lblock = -1;
  newHeader.rblock = -1;
  newHeader.pblock = -1;
  newHeader.numAttrs = 0;
  newHeader.numEntries = 0;
  newHeader.numSlots = 0;

  BlockBuffer::setHeader(&newHeader);
  BlockBuffer::setBlockType(blockType);

  return blockNum;
}

void BlockBuffer::releaseBlock(){

  // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
  if(blockNum == INVALID_BLOCKNUM || StaticBuffer::blockAllocMap[this->blockNum] == UNUSED_BLK){
    return;
  }
  // else
      /* get the buffer number of the buffer assigned to the block
         using StaticBuffer::getBufferNum().
         (this function return E_BLOCKNOTINBUFFER if the block is not
         currently loaded in the buffer)
          */
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if(bufferNum >=0 && bufferNum <BUFFER_CAPACITY){
    StaticBuffer::metainfo[bufferNum].free = true;
  }
      // if the block is present in the buffer, free the buffer
      // by setting the free flag of its StaticBuffer::tableMetaInfo entry
      // to true.

      // free the block in disk by setting the data type of the entry
      // corresponding to the block number in StaticBuffer::blockAllocMap
      // to UNUSED_BLK.
  StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
  this->blockNum = INVALID_BLOCKNUM;
      // set the object's blockNum to INVALID_BLOCK (-1)
}

// STAGE 10 //

// call the corresponding parent constructor
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}

// call the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}

IndLeaf::IndLeaf() : IndBuffer('L'){} // this is the way to call parent non-default constructor.
                      // 'L' used to denote IndLeaf.

//this is the way to call parent non-default constructor.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

IndInternal::IndInternal() : IndBuffer('I'){}
// call the corresponding parent constructor
// 'I' used to denote IndInternal.

IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}
// call the corresponding parent constructor


int IndInternal::getEntry(void *ptr, int indexNum) {
    
    // check the range of indexNum
    if(indexNum <0 || indexNum >MAX_KEYS_INTERNAL-1){
      return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    // to get the starting address 
    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) return ret;

    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    /*
    - copy the entries from the indexNum`th entry to *internalEntry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);//child ptr 4B and attr size 16B

    memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(internalEntry->rChild), entryPtr + 20, 4);

     return SUCCESS;
}

int IndLeaf::getEntry(void *ptr, int indexNum) {

    //check range of IndexNum
    if(indexNum < 0 || indexNum >MAX_KEYS_LEAF-1){
      return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
   //get starting address
   int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
   if(ret != SUCCESS) return ret;


    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

     return SUCCESS;
}




// STAGE 11 //
int IndLeaf::setEntry(void *ptr, int indexNum) {

   if(indexNum<0 || indexNum>=MAX_KEYS_LEAF)
  {
    return E_OUTOFBOUND;
   }

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) return ret;
  

    // copy the Index at ptr to indexNum'th entry in the buffer using memcpy

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    //struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy(entryPtr, (struct Index *)ptr, LEAF_ENTRY_SIZE);

    // update dirty bit using setDirtyBit()
    // if setDirtyBit failed, return the value returned by the call
    int retVal = StaticBuffer::setDirtyBit(indexNum) ;
    if(retVal != SUCCESS) return retVal;

    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
    
   if(indexNum<0 || indexNum>=MAX_KEYS_INTERNAL){
    return E_OUTOFBOUND;
   }

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) return ret;
   
   
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    /*
    - copy the entries from *internalEntry to the indexNum`th entry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */

    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */

    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(entryPtr, &(internalEntry->lChild), 4);
    memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + 20, &(internalEntry->rChild), 4);
    int retVal = StaticBuffer::setDirtyBit(indexNum) ;
    if(retVal != SUCCESS) return retVal;


   
     return SUCCESS;
}

