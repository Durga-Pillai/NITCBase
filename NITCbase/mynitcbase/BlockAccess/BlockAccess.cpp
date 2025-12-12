#include "BlockAccess.h"
#include <stdlib.h>
#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
  // get the previous search index of the relation relId from the relation cache
  // (use RelCacheTable::getSearchIndex() function)
  RecId prevRecId;
  RelCacheTable::getSearchIndex(relId, &prevRecId);

  // let block and slot denote the record id of the record being currently checked
  // printf("Came linear search\n");
  // if the current search index record is invalid(i.e. both block and slot = -1)
  int block = prevRecId.block, slot = prevRecId.slot;
  if (prevRecId.block == -1 && prevRecId.slot == -1)
  {
    // (no hits from previous search; search should start from the
    // first record itself)

    // get the first record block of the relation from the relation cache
    RelCatEntry RelCatBuffer;
    RelCacheTable::getRelCatEntry(relId, &RelCatBuffer);
    // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
    block = RelCatBuffer.firstBlk;
    slot = 0;
    // block = first record block of the relation
    // slot = 0
  }
  else
  {
    // (there is a hit from previous search; search should start from
    // the record next to the search index record)

    slot++;
    // block = search index's block
    // slot = search index's slot + 1
  }

  /* The following code searches for the next record in the relation
     that satisfies the given condition
     We start from the record id (block, slot) and iterate over the remaining
     records of the relation
  */
  RelCatEntry relCatbuffer;
  RelCacheTable::getRelCatEntry(relId, &relCatbuffer);
  while (block != -1)
  {
    /* create a RecBuffer object for block (use RecBuffer Constructor for
       existing block) */
    // printf("op:%d",op);
    RecBuffer bufferPtr(block);
    // printf("AKJNKJDSBHSJBH\n");

    // get the record with id (block, slot) using RecBuffer::getRecord()

    // get header of the block using RecBuffer::getHeader() function
    HeadInfo header;
    bufferPtr.getHeader(&header);
    // get slot map of the block using RecBuffer::getSlotMap() function
    unsigned char *slotMap = (unsigned char *)malloc(sizeof(unsigned char) * header.numSlots);
    bufferPtr.getSlotMap(slotMap);

    // If slot >= the number of slots per block(i.e. no more slots in this block)
    if (slot >= relCatbuffer.numSlotsPerBlk)
    {
      // update block = right block of block
      block = header.rblock;
      // printf("first if\n");
      //  update slot = 0
    
      slot = 0;
      continue; // continue to the beginning of this while loop
    }

    // if slot is free skip the loop
    // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
    if (slotMap[slot] == SLOT_UNOCCUPIED)
    {
      // increment slot and continue to the next record slot
      slot++;
      continue;
    }

    // compare record's attribute value to the the given attrVal as below:
    /*
        firstly get the attribute offset for the attrName attribute
        from the attribute cache entry of the relation using
        AttrCacheTable::getAttrCatEntry()
    */
    // printf("slot:%d\n",slot);
    AttrCatEntry attrcatbuffer;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrcatbuffer);
    /* use the attribute offset to get the value of the attribute from
       current record */

    // Attribute catRec[RELCAT_NO_ATTRS];
    // printf("illegal \n");
    // printf("op:%d",op);
    Attribute *record = (Attribute *)malloc(sizeof(Attribute) * header.numAttrs);
    bufferPtr.getRecord(record, slot);
    int attroff = attrcatbuffer.offset;
    // printf("illegal \n");

    // int cmpVal;  // will store the difference between the attributes
    //  set cmpVal using compareAttrs()
    int cmpVal = compareAttrs(record[attroff], attrVal, attrcatbuffer.attrType);
    BPlusTree::numComp++;
    // printf("illegal \n");

    /* Next task is to check whether this record satisfies the given condition.
       It is determined based on the output of previous comparison and
       the op value received.
       The following code sets the cond variable if the condition is satisfied.
    */
    // printf("op:%d,cmpVal:%d\n",op,cmpVal);
    if (
        (op == NE && cmpVal != 0) || // if op is "not equal to"
        (op == LT && cmpVal < 0) ||  // if op is "less than"
        (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
        (op == EQ && cmpVal == 0) || // if op is "equal to"
        (op == GT && cmpVal > 0) ||  // if op is "greater than"
        (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
    )
    {
      /*
      set the search index in the relation cache as
      the record id of the record that satisfies the given condition
      (use RelCacheTable::setSearchIndex function)
      */
      RecId index;
      index.block = block;
      index.slot = slot;
      // printf("Slot:%d\n",slot);
      RelCacheTable::setSearchIndex(relId, &index);

      return RecId{block, slot};
    }

    slot++;
  }

  // no record in the relation with Id relid satisfies the given condition
  return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
  /* reset the searchIndex of the relation catalog using
     RelCacheTable::resetSearchIndex() */
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute newRelationName; // set newRelationName with newName
  strcpy(newRelationName.sVal, newName);
  // search the relation catalog for an entry with "RelName" = newRelationName
  char relCatAttrRelName[ATTR_SIZE];
  strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);
  RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, newRelationName, EQ);

  // If relation with name newName already exists (result of linearSearch
  //                                               is not {-1, -1})
  //    return E_RELEXIST;
  if (recId.block != -1 && recId.slot != -1)
    return E_RELEXIST;
  /* reset the searchIndex of the relation catalog using
     RelCacheTable::resetSearchIndex() */
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute oldRelationName; // set oldRelationName with oldName
  strcpy(oldRelationName.sVal, oldName);
  // search the relation catalog for an entry with "RelName" = oldRelationName
  recId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, oldRelationName, EQ);
  // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
  //    return E_RELNOTEXIST;
  if (recId.block == -1 || recId.slot == -1)
    return E_RELNOTEXIST;

  /* get the relation catalog record of the relation to rename using a RecBuffer
     on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
  */
  Attribute record[RELCAT_NO_ATTRS];
  RecBuffer recBuffer(RELCAT_BLOCK);
  recBuffer.getRecord(record, recId.slot);

  /* update the relation name attribute in the record with newName.
     (use RELCAT_REL_NAME_INDEX) */
  strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
  // set back the record value using RecBuffer.setRecord
  recBuffer.setRecord(record, recId.slot);
  /*

  update all the attribute catalog entries in the attribute catalog corresponding
  to the relation with relation name oldName to the relation name newName
  */

  /* reset the searchIndex of the attribute catalog using
     RelCacheTable::resetSearchIndex() */
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  int numofattrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
  char attrCatAttrRelname[ATTR_SIZE];
  strcpy(attrCatAttrRelname, ATTRCAT_ATTR_RELNAME);

  // for i = 0 to numberOfAttributes :
  //     linearSearch on the attribute catalog for relName = oldRelationName
  //     get the record using RecBuffer.getRecord
  //
  //     update the relName field in the record to newName
  //     set back the record using RecBuffer.setRecord
  for (int i = 0; i < numofattrs; i++)
  {
    RecId rec_id = BlockAccess::linearSearch(ATTRCAT_RELID, attrCatAttrRelname, oldRelationName, EQ);
    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    RecBuffer attrBuffer(rec_id.block);
    attrBuffer.getRecord(attrRecord, rec_id.slot);

    strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newRelationName.sVal);

    attrBuffer.setRecord(attrRecord, rec_id.slot);
  }

  return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{

  /* reset the searchIndex of the relation catalog using
     RelCacheTable::resetSearchIndex() */
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute relNameAttr; // set relNameAttr to relName
  strcpy(relNameAttr.sVal, relName);
  // Search for the relation with name relName in relation catalog using linearSearch()
  // If relation with name relName does not exist (search returns {-1,-1})
  //    return E_RELNOTEXIST;
  char relCatAttrRelname[ATTR_SIZE];
  strcpy(relCatAttrRelname, RELCAT_ATTR_RELNAME);
  RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelname, relNameAttr, EQ);

  if (recId.block == -1 && recId.slot == -1)
    return E_RELNOTEXIST;
  /* reset the searchIndex of the attribute catalog using
     RelCacheTable::resetSearchIndex() */
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  /* declare variable attrToRenameRecId used to store the attr-cat recId
  of the attribute to rename */
  RecId attrToRenameRecId = {-1, -1};
  Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

  /* iterate over all Attribute Catalog Entry record corresponding to the
     relation to find the required attribute */
  char attrCatAttrRelname[ATTR_SIZE];
  strcpy(attrCatAttrRelname, ATTRCAT_ATTR_RELNAME);
  while (true)
  {
    // linear search on the attribute catalog for RelName = relNameAttr
    RecId rec_Id = BlockAccess::linearSearch(ATTRCAT_RELID, attrCatAttrRelname, relNameAttr, EQ);

    // if there are no more attributes left to check (linearSearch returned {-1,-1})
    //     break;
    if (rec_Id.block == -1 && rec_Id.slot == -1)
      break;

    /* Get the record from the attribute catalog using RecBuffer.getRecord
      into attrCatEntryRecord */
    RecBuffer attrBuffer(rec_Id.block);
    attrBuffer.getRecord(attrCatEntryRecord, rec_Id.slot);
    // if attrCatEntryRecord.attrName = oldName
    //     attrToRenameRecId = block and slot of this record
    if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
    {
      attrToRenameRecId.block = rec_Id.block;
      attrToRenameRecId.slot = rec_Id.slot;
    }
    // if attrCatEntryRecord.attrName = newName
    //     return E_ATTREXIST;
    if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
      return E_ATTREXIST;
  }

  // if attrToRenameRecId == {-1, -1}
  //     return E_ATTRNOTEXIST;
  if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
    return E_ATTRNOTEXIST;

  // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
  RecBuffer attrToRenameBuffer(attrToRenameRecId.block);
  attrToRenameBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
  strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
  /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
       attrToRenameRecId.slot */
  //   update the AttrName of the record with newName
  //   set back the record with RecBuffer.setRecord
  attrToRenameBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

  return SUCCESS;
}
int BlockAccess::insert(int relId, Attribute *record) {
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(relId, &relCatEntry);

  int blockNum = relCatEntry.lastBlk;

  RecId recId = {-1, -1};
  int numOfSlots = relCatEntry.numSlotsPerBlk;
  int numOfAttributes = relCatEntry.numAttrs;
  int prevBlockNum = -1;

  // find the block and slot which is free for insertion
  while (blockNum != -1) {
    RecBuffer blockBuffer(blockNum);

    HeadInfo header;
    blockBuffer.getHeader(&header);

    unsigned char slotMap[numOfSlots];
    blockBuffer.getSlotMap(slotMap);

    // loops through the slot map and find a free slot
    for (int slotIndex = 0; slotIndex < numOfSlots; slotIndex++) {
      if (slotMap[slotIndex] == SLOT_UNOCCUPIED) {
        recId.block = blockNum;
        recId.slot = slotIndex;
        break;
      }
    }

    if (recId.block != -1 and recId.slot != -1) {
      break;
    }

    // go to next block
    prevBlockNum = blockNum;
    blockNum = header.rblock;
  }

  // there is no free slots in any of the blocks allocated for the relation
  // => we need to allocate a new block for the relation
  if (recId.block == -1 && recId.slot == -1) {
    // RELCAT can span only one block => 20 slots
    if (relId == RELCAT_RELID) return E_MAXRELATIONS;

    // allocate a new free block
    RecBuffer blockBuffer;
    int blockNum = blockBuffer.getBlockNum();
    if (blockNum == E_DISKFULL) return E_DISKFULL;

    recId.block = blockNum;
    recId.slot = 0;

    // create a header for the new block
    HeadInfo head;
    head.blockType = REC;
    head.pblock = -1;
    head.lblock = relCatEntry.numRecs == 0 ? -1 : prevBlockNum;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = numOfAttributes;
    head.numSlots = numOfSlots;

    blockBuffer.setHeader(&head);

    // mark all the slots as SLOT_UNOCCUPIED
    unsigned char slotMap[numOfSlots];
    for (int i = 0; i < numOfSlots; i++) {
      slotMap[i] = SLOT_UNOCCUPIED;
    }
    blockBuffer.setSlotMap(slotMap);

    if (prevBlockNum != -1) {
      // update the linked list of block (r block of prevBlock points to the current block)
      RecBuffer prevBlock(prevBlockNum);
      HeadInfo prevHead;
      prevBlock.getHeader(&prevHead);

      prevHead.rblock = blockNum;
      prevBlock.setHeader(&prevHead);
    } else {
      // this is the first block of the particular relation
      relCatEntry.firstBlk = recId.block;
      RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // since there was no space in between => this was the last block of that particular relation
    relCatEntry.lastBlk = recId.block;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);
  }

  // insert the record into the slot
  RecBuffer blockBuffer(recId.block);
  int ret = blockBuffer.setRecord(record, recId.slot);
  if (ret != SUCCESS) {
    exit(FAILURE);
  }

  unsigned char slotMap[numOfSlots];
  blockBuffer.getSlotMap(slotMap);

  // update the slotmap
  slotMap[recId.slot] = SLOT_OCCUPIED;
  blockBuffer.setSlotMap(slotMap);

  HeadInfo header;
  blockBuffer.getHeader(&header);

  // increment the number of entries in the block and set the header
  header.numEntries++;
  blockBuffer.setHeader(&header);

  // increment the number of records in the relCatEntry in relCache
  relCatEntry.numRecs++;
  RelCacheTable::setRelCatEntry(relId, &relCatEntry);

  //STAGE 11//
  int flag = SUCCESS;
    // Iterate over all the attributes of the relation
    // (let attrOffset be iterator ranging from 0 to numOfAttributes-1)
    for(int i=0;i<numOfAttributes;i++)
    {
        // get the attribute catalog entry for the attribute from the attribute cache
        // (use AttrCacheTable::getAttrCatEntry() with args relId and attrOffset)
        AttrCatEntry attrcatentry;
        AttrCacheTable::getAttrCatEntry(relId,i,&attrcatentry);


        // get the root block field from the attribute catalog entry
        int rootblock = attrcatentry.rootBlock;

        // if index exists for the attribute(i.e. rootBlock != -1)
        if(rootblock!=-1)
        {
            /* insert the new record into the attribute's bplus tree using
             BPlusTree::bPlusInsert()*/
            int retVal = BPlusTree::bPlusInsert(relId, attrcatentry.attrName,
                                                record[i], recId);

            if (retVal == E_DISKFULL) {
                //(index for this attribute has been destroyed)
                // flag = E_INDEX_BLOCKS_RELEASED
                flag = E_INDEX_BLOCKS_RELEASED;
            }
        }
    }

    return flag;
}

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
  // user is not allowed to delete RELAIONCAT and ATTRIBUTECAT
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
  {
    return E_NOTPERMITTED;
  }

  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  Attribute relNameAttr;
  strcpy(relNameAttr.sVal, relName);

  // finding the block and slot of the relation catalog entry of the relation in the relation catalog block
  char relcatAttrRelname[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
  RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrRelname, relNameAttr, EQ);
  if (relCatRecId.block == -1 || relCatRecId.slot == -1)
  {
    return E_RELNOTEXIST;
  }

  Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
  RecBuffer relCatBlock(relCatRecId.block);
  relCatBlock.getRecord(relCatEntryRecord, relCatRecId.slot);

  int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
  int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

  int currentBlock = firstBlock;

  // delete all the record blocks of the relations
  while (currentBlock != -1)
  {
    RecBuffer currentBlockBuffer(currentBlock);

    HeadInfo currentBlockHeader;
    currentBlockBuffer.getHeader(&currentBlockHeader);

    currentBlock = currentBlockHeader.rblock;
    currentBlockBuffer.releaseBlock();
  }

  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  int numOfAttributesDeleted = 0;

  RelCatEntry attrCatEntry;
  RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
  int correctNumSlotsForAttrCat = attrCatEntry.numSlotsPerBlk;

  while (true)
  {
    char relcatRelName[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
    RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relcatRelName, relNameAttr, EQ);
    if (attrCatRecId.block == -1 and attrCatRecId.slot == -1)
    {
      break;
    }

    numOfAttributesDeleted++;

    RecBuffer attrCatBuffer(attrCatRecId.block);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);

    int numOfSlots = correctNumSlotsForAttrCat;

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBuffer.getRecord(attrCatRecord, attrCatRecId.slot);

    int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

    // update the slotmap entry of the attribute
    unsigned char slotMap[numOfSlots];
    attrCatBuffer.getSlotMap(slotMap);
    slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
    attrCatBuffer.setSlotMap(slotMap);

    // update the number of entries in the attribute catalog header
    attrCatHeader.numEntries--;
    attrCatBuffer.setHeader(&attrCatHeader);

    // release the block if there is no entries in the block
    if (attrCatHeader.numEntries == 0)
    {
      // update the linkedlist of block after deletion of the middle block
      int lBlockNum = attrCatHeader.lblock;
      int rBlockNum = attrCatHeader.rblock;

      if (lBlockNum != -1)
      {
        RecBuffer prevBlock(lBlockNum);
        HeadInfo prevHeader;
        prevBlock.getHeader(&prevHeader);
        prevHeader.rblock = rBlockNum;
        prevBlock.setHeader(&prevHeader);
      }
      else
      {
        attrCatEntry.firstBlk = rBlockNum;
      }

      if (rBlockNum != -1)
      {
        RecBuffer nextBlock(rBlockNum);
        HeadInfo nextHeader;
        nextBlock.getHeader(&nextHeader);
        nextHeader.lblock = lBlockNum;
        nextBlock.setHeader(&nextHeader);
      }
      else
      {
        attrCatEntry.lastBlk = lBlockNum;
      }

      RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
      attrCatBuffer.releaseBlock();
    }

    if (rootBlock != -1)
    {
      // do BPlusDestroy here
      BPlusTree::bPlusDestroy(rootBlock);
    }
  }

  // decrement the number of entries in the relation catalog
  HeadInfo relCatHeader;
  relCatBlock.getHeader(&relCatHeader);

  relCatHeader.numEntries--;
  relCatBlock.setHeader(&relCatHeader);

  // update the slotmap in relation catalog block
  unsigned char slotMap[relCatHeader.numSlots];
  relCatBlock.getSlotMap(slotMap);

  slotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;
  relCatBlock.setSlotMap(slotMap);

  // update the relation catalog entry in relCache
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);

  relCatEntry.numRecs--;
  RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

  // update the attribute catalog entry in relCache
  RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);

  relCatEntry.numRecs -= numOfAttributesDeleted;
  RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);

  return SUCCESS;
}
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
  // Declare a variable called recid to store the searched record
  RecId recId;

  /* get the attribute catalog entry from the attribute cache corresponding
  to the relation with Id=relid and with attribute_name=attrName  */
  AttrCatEntry attrCatEntry;
  int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
  if (ret != SUCCESS)
    return ret;
  // if this call returns an error, return the appropriate error code

  // get rootBlock from the attribute catalog entry
  /* if Index does not exist for the attribute (check rootBlock == -1) */ 
    int rootBlock = attrCatEntry.rootBlock;
    if (rootBlock == -1){
      recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);
      //printf("Number of comparisons before indexing=%d\n",BPlusTree::numComp);
    }

    /* search for the record id (recid) corresponding to the attribute with
       attribute name attrName, with value attrval and satisfying the
       condition op using linearSearch()
    */
  

  /* else */ 
    // (index exists for the attribute)
    else recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    /* search for the record id (recid) correspoding to the attribute with
    attribute name attrName and with value attrval and satisfying the
    condition op using BPlusTree::bPlusSearch() */
  

  // if there's no record satisfying the given condition (recId = {-1, -1})
  //     return E_NOTFOUND;
  if(recId.block== -1 and recId.slot==-1) return E_NOTFOUND;

  /* Copy the record with record id (recId) to the record buffer (record).
     For this, instantiate a RecBuffer class object by passing the recId and
     call the appropriate method to fetch the record
  */

  RecBuffer block(recId.block);
  return block.getRecord(record,recId.slot);
}

int BlockAccess::project(int relId, Attribute *record)
{
  // get the last hit block and slot -> search index
  RecId prevSearchIndex;
  RelCacheTable::getSearchIndex(relId, &prevSearchIndex);

  int block, slot;

  if (prevSearchIndex.block == -1 and prevSearchIndex.slot == -1)
  {
    // new project operation -> start from beginning
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    block = relCatEntry.firstBlk;
    slot = 0;
  }
  else
  {
    // a project operation is done already -> start from next slot
    block = prevSearchIndex.block;
    slot = prevSearchIndex.slot + 1;
  }

  // loop until we get a satisfying slot i.e.,
  //    if this is the last slot, go to next block
  //    if the slot is unoccupied move to next slot
  while (block != -1)
  {
    RecBuffer currentBlock(block);

    HeadInfo blockHeader;
    currentBlock.getHeader(&blockHeader);

    unsigned char slotMap[blockHeader.numSlots];
    currentBlock.getSlotMap(slotMap);

    if (slot >= blockHeader.numSlots)
    {
      block = blockHeader.rblock;
      slot = 0;
    }
    else if (slotMap[slot] == SLOT_UNOCCUPIED)
    {
      slot++;
    }
    else
    {
      break;
    }
  }

  if (block == -1)
  {
    return E_NOTFOUND;
  }

  // update the search index (last hit)
  RecId nextRecId{block, slot};
  RelCacheTable::setSearchIndex(relId, &nextRecId);

  RecBuffer blockBuffer(block);
  return blockBuffer.getRecord(record, slot);
}
