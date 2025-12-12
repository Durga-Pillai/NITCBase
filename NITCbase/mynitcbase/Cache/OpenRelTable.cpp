#include "OpenRelTable.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable:: OpenRelTable() {
  // initialize relCache and attrCache with nullptr
 
  for(int i = 0; i< MAX_OPEN; i++){
      RelCacheTable::relCache[i] = nullptr;
      AttrCacheTable::attrCache[i] = nullptr;
  }

  
  /******* Setting up Relation Cache entries *********/
  // we need to populate relation cache with entries for the relation and attribute catalog.

  // Setting up Relation Catalog relation in the Relation Cache Table
  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];

  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT; // 0
  // allocate this on the heap beacuse we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;



  // Setting up Attribute Catalog relation in the Relation Cache Table
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK; //block number 4 (relational catalog block)
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT; // 1
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


  /******** Setting up Attribute Cache Entries *********/
  // Setting up Relation Catalog relation in the Attribute Cache Table
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  // iterate through all the attributes of the relation catalog and create a linked list
  // NOTE: allocate each entry dynamically using malloc
  struct AttrCacheEntry *head, *last;
  for(int i = 0; i<6; i++){
      attrCatBlock.getRecord(attrCatRecord,i);
      struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
      attrCacheEntry->recId.block = ATTRCAT_BLOCK;
      attrCacheEntry->recId.slot = i;
      if(i == 0){
          head = attrCacheEntry;
          last = attrCacheEntry;
      }
      else{
          last->next = attrCacheEntry;
          last = last->next;
      }
  }
  last->next = nullptr;
  AttrCacheTable::attrCache[RELCAT_RELID] = head;


  // Setting up Attribute Catalog relation in the Attribute Cache Table
  for(int i = 6; i<12; i++){
      attrCatBlock.getRecord(attrCatRecord,i);
      struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
      AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
      attrCacheEntry->recId.block = ATTRCAT_BLOCK;
      attrCacheEntry->recId.slot = i;
      if(i == 6){
          head = attrCacheEntry;
          last = attrCacheEntry;
      }
      else{
          last->next = attrCacheEntry;
          last = attrCacheEntry;
      }
  }
  last->next = nullptr;
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

  /*************** Setting up tableMetaInfo entries *******************/
  for(int i = 0; i<MAX_OPEN; i++){
      if(i == RELCAT_RELID){ // i == 0
          tableMetaInfo[i].free = false;
          strcpy(tableMetaInfo[0].relName, RELCAT_RELNAME);
      }
      else if(i == ATTRCAT_RELID){    // i == 1
          tableMetaInfo[i].free = false;
          strcpy(tableMetaInfo[1].relName, ATTRCAT_RELNAME);
      }
      else{
          tableMetaInfo[i].free = true;
      }
  }


}

OpenRelTable::~OpenRelTable(){

    // close all the open relations from rel-id = 2 onwards
    for(int i = 2; i<MAX_OPEN; i++){
        if(!tableMetaInfo[i].free){
            OpenRelTable::closeRel(i);
        }
    }

    /**** Closing the catalog relations in the relation cache *****/
    
    // releasing the relation cache entry of the attribute catalog
    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true){
         /* 
            Get the Relation Catalog entry from RelCacheTable::relCache
            Then convert it to record
         */
        RelCatEntry relCatEntry = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);

        // get the record id form the relCache, it stores where the relation is present in Relation Catalog
        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);

        // write back to the buffer using relCatBlock.setRecord()
        relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    // free the memory which was dynamically allocted to to relCache[ATTRCAT_RELID]
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    // releasing the relation cache entry of the relation catalog
    if(RelCacheTable::relCache[RELCAT_RELID]->dirty == true){
        /*
            Get the Relation catalog entry from RelCacheTable::relCache
            and convert it to record
         */
         RelCatEntry relCatEntry = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
         Attribute relCatRecord[RELCAT_NO_ATTRS];
         RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);

         RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
         RecBuffer relCatBlock(recId.block);
         relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    free(RelCacheTable::relCache[RELCAT_RELID]);


    // free the memory allocated for the attribute cache entries of the relation catalog and the attribute catalog
    // NOTE: they are never changed because the attributes of relcat and attrcat are never modified.
    for(int i = 0; i < 2; i++){
        struct AttrCacheEntry *entry = AttrCacheTable::attrCache[i];
        while(entry != nullptr){
            struct AttrCacheEntry *temp = entry;
            entry = entry->next;
            free(temp);
        }
    }

}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (tableMetaInfo[i].free == false && strcmp(relName, tableMetaInfo[i].relName) == 0)
      return i;
  }
  return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry()
{

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
  for (int i = 0; i < MAX_OPEN; i++)
  {

    if (OpenRelTable::tableMetaInfo[i].free)
      return i;
    
  }
  
  return E_CACHEFULL;
  // if found return the relation id, else return E_CACHEFULL.
}


int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  // If the relation is already opened, then we will return the relId of the opened relation.
  int alreadyExistingRelId = OpenRelTable::getRelId(relName);
  if (alreadyExistingRelId >= 0) return alreadyExistingRelId;

  // find a free slot in the Cache to put the newly opening table
  int freeSlot = OpenRelTable::getFreeOpenRelTableEntry();
  if (freeSlot == E_CACHEFULL) return E_CACHEFULL;

  int relId = freeSlot;
  RelCacheTable::relCache[relId] = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));

  // reset the search index for RELATIONCAT to search the given relation name in the relation catalog.
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  union Attribute relNameAttribute;
  strcpy(relNameAttribute.sVal, relName);
  char relCatAttrRelName[ATTR_SIZE];
  strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

  // we find the entry of the relation in the RELATIONCAT using linear search.
  RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, relNameAttribute, EQ);
  if (relCatRecId.block == -1 || relCatRecId.slot == -1) return E_RELNOTEXIST;

  // add the relation catalog entry for the opening table into the relCache as a relCacheEntry
  struct RelCacheEntry relCacheEntry;
  RecBuffer relCatBlock(relCatRecId.block);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, relCatRecId.slot);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);

  RelCacheTable::relCache[relId] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[relId]) = relCacheEntry;
  RelCacheTable::relCache[relId]->recId.block = relCatRecId.block;
  RelCacheTable::relCache[relId]->recId.slot = relCatRecId.slot;

  // add the attributes of the corresponding relation into the attrCache as attrCacheEntries
  AttrCacheEntry *listHead = nullptr;
  int numOfAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

  AttrCacheEntry *prev = nullptr;

  // resets the search index for attribute catalog to search the attributes of the given table in the attribute catalog
  // blocks.
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  for (int i = 0; i < numOfAttrs; i++) {
    // searches and find all the "numAttrs" number of attributes of the given relation and puts it into the AttrCache
    // linked list.
    RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relCatAttrRelName, relNameAttribute, EQ);
    if (attrCatRecId.block == -1 || attrCatRecId.slot == -1) {
      return E_ATTRNOTEXIST;
    }

    struct AttrCacheEntry *curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    RecBuffer attrCatBlock(attrCatRecId.block);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
    curr->next = nullptr;
    curr->recId.block = attrCatRecId.block;
    curr->recId.slot = attrCatRecId.slot;

    if (prev)
      prev->next = curr;
    else
      listHead = curr;

    prev = curr;
  }

  // saves the head of the linked list in the attrCache and initializes the other metadata.
  AttrCacheTable::attrCache[relId] = listHead;
  RelCacheTable::relCache[relId]->dirty = false;
  OpenRelTable::tableMetaInfo[relId].free = false;
  strcpy(OpenRelTable::tableMetaInfo[relId].relName, relName);

  if (AttrCacheTable::attrCache[relId] == nullptr) return E_ATTRNOTEXIST;

  return relId;
  //return SUCCESS;
}


int OpenRelTable::closeRel(int relId)
{
 // printf("%d\n",relId);
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
  {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN)
  {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free == true)
  {
    return E_RELNOTOPEN;
  }

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  if(RelCacheTable::relCache[relId]->dirty == true){
    Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry),record);

    RecId recId = RelCacheTable::relCache[relId]->recId;
    RecBuffer relCatBlock(recId.block);
    int ret = relCatBlock.setRecord(record,recId.slot);
  }
  for(AttrCacheEntry*entry = AttrCacheTable::attrCache[relId];entry!=nullptr;entry = entry->next){
    //if the entry id dirty ,write back
    if(entry->dirty==true){
      AttrCatEntry dirtyentry = entry->attrCatEntry;
      RecId recId = entry->recId;
      //convert to record for writing back
      Attribute record[ATTRCAT_NO_ATTRS];
      AttrCacheTable::attrCatEntryToRecord(&dirtyentry,record);

      RecBuffer dirtyblock(recId.block);
      dirtyblock.setRecord(record,recId.slot);
    }
  }
  free(RelCacheTable::relCache[relId]);
  AttrCacheEntry *entry, *tem;
  entry = AttrCacheTable::attrCache[relId];
  while (entry != nullptr)
  {
    tem = entry;
    entry = entry->next;
    free(tem);
  }
  // update `tableMetaInfo` to set `relId` as a free slot
  tableMetaInfo[relId].free = true;
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  RelCacheTable::relCache[relId] = nullptr;
  AttrCacheTable::attrCache[relId] = nullptr;
  strcpy(tableMetaInfo[relId].relName,"");
  return SUCCESS;
}
