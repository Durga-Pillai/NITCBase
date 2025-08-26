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
    char *relCatAttrRelName;
    strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, newRelationName, EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if (recId.block == -1 && recId.slot == -1)
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
    if (recId.block == -1 && recId.slot == -1)
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
    char *attrCatAttrRelname;
    strcpy(attrCatAttrRelname, ATTRCAT_ATTR_RELNAME);
    // for i = 0 to numberOfAttributes :
    //     linearSearch on the attribute catalog for relName = oldRelationName
    //     get the record using RecBuffer.getRecord
    //
    //     update the relName field in the record to newName
    //     set back the record using RecBuffer.setRecord
    for (int i = 0; i < numofattrs; i++)
    {
        RecId rec_id = BlockAccess::linearSearch(ATTRCAT_RELID, attrCatAttrRelname,oldRelationName, EQ);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        RecBuffer attrBuffer(rec_id.block);
        attrBuffer.getRecord(attrRecord, rec_id.slot);

        attrRecord[ATTRCAT_REL_NAME_INDEX] = newRelationName;
        attrBuffer.setRecord(record, rec_id.slot);
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
    char *relCatAttrRelname;
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
    char *attrCatAttrRelname;
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
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0)
            return E_ATTREXIST;
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) return E_ATTRNOTEXIST;

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    RecBuffer attrToRenameBuffer(attrToRenameRecId.block);
    attrToRenameBuffer.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord
    attrToRenameBuffer.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);

    return SUCCESS;
}