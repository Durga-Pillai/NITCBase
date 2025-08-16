#include "BlockAccess.h"
#include<stdlib.h>
#include <cstring>
#include<iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);

    // let block and slot denote the record id of the record being currently checked
   // printf("Came linear search\n");
    // if the current search index record is invalid(i.e. both block and slot = -1)
    int block = prevRecId.block,slot = prevRecId.slot;
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        RelCatEntry RelCatBuffer;
        RelCacheTable::getRelCatEntry(relId,&RelCatBuffer);
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        block = RelCatBuffer.firstBlk;
        slot =0;
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
   RelCacheTable::getRelCatEntry(relId,&relCatbuffer);
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
           //printf("op:%d",op);
           RecBuffer bufferPtr(block);
           //printf("AKJNKJDSBHSJBH\n");


        // get the record with id (block, slot) using RecBuffer::getRecord()
        
        // get header of the block using RecBuffer::getHeader() function
        HeadInfo header;
        bufferPtr.getHeader(&header);
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char *slotMap = (unsigned char*) malloc(sizeof(unsigned char)*header.numSlots);
        bufferPtr.getSlotMap(slotMap);

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot >= relCatbuffer.numSlotsPerBlk)
        {
            // update block = right block of block
            block = header.rblock;
            //printf("first if\n");
            // update slot = 0
            slot =0;
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot] == SLOT_UNOCCUPIED)
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
       //printf("slot:%d\n",slot);
       AttrCatEntry attrcatbuffer;
       AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatbuffer);
        /* use the attribute offset to get the value of the attribute from
           current record */
        
       // Attribute catRec[RELCAT_NO_ATTRS];
       //printf("illegal \n");
       //printf("op:%d",op);
        Attribute * record = (Attribute*) malloc(sizeof(Attribute)*header.numAttrs);
        bufferPtr.getRecord(record,slot);
        int attroff = attrcatbuffer.offset;
        //printf("illegal \n");
        

        //int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        int cmpVal = compareAttrs(record[attroff],attrVal,attrcatbuffer.attrType);
        //printf("illegal \n");

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
      // printf("op:%d,cmpVal:%d\n",op,cmpVal);
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
           RecId index;
           index.block = block;
           index.slot = slot;
           //printf("Slot:%d\n",slot);
           RelCacheTable::setSearchIndex(relId,&index);

            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}
