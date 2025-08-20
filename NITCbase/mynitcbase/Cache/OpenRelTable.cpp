#include "OpenRelTable.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable()
{
  for (int i = 0; i < MAX_OPEN; i++)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  // relation catalog block to relation cache
  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  char relCatName[ATTR_SIZE];
  strcpy(relCatName, relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  // attribute catalog block to relation cache
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

  char attrCatName[ATTR_SIZE];
  strcpy(attrCatName, relCatRecord[ATTRCAT_REL_NAME_INDEX].sVal);

  struct RelCacheEntry attrCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &attrCacheEntry.relCatEntry);
  attrCacheEntry.recId.block = RELCAT_BLOCK;
  attrCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;

  // relation catalog to attribute cache
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry *head = nullptr;
  AttrCacheEntry *prev = nullptr;

  for (int i = 0; i < RELCAT_NO_ATTRS; i++)
  {
    attrCatBlock.getRecord(attrCatRecord, i);
    AttrCacheEntry *curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
    curr->recId.block = ATTRCAT_BLOCK;
    curr->recId.slot = i;
    curr->next = nullptr;

    if (prev)
      prev->next = curr;
    else
      head = curr;

    prev = curr;
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = head;

  // attribute catalog to attribute cache
  AttrCacheEntry *head2 = nullptr;
  AttrCacheEntry *prev2 = nullptr;

  for (int i = 0; i < ATTRCAT_NO_ATTRS; i++)
  {
    int slotNum = i + RELCAT_NO_ATTRS;
    attrCatBlock.getRecord(attrCatRecord, slotNum);
    AttrCacheEntry *curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
    curr->recId.block = ATTRCAT_BLOCK;
    curr->recId.slot = slotNum;
    curr->next = nullptr;

    if (prev2)
      prev2->next = curr;
    else
      head2 = curr;
    if (curr)
      prev2 = curr;
  }
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head2;

  // setting metatableinfo
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (i == RELCAT_RELID)
    { // i == 0
      OpenRelTable::tableMetaInfo[i].free = false;
      strcpy(OpenRelTable::tableMetaInfo[0].relName, RELCAT_RELNAME);
    }
    else if (i == ATTRCAT_RELID)
    { // i == 1
      OpenRelTable::tableMetaInfo[i].free = false;
      strcpy(OpenRelTable::tableMetaInfo[1].relName, ATTRCAT_RELNAME);
    }
    else
    {
      OpenRelTable::tableMetaInfo[i].free = true;
    }
  }
}
OpenRelTable::~OpenRelTable()
{
  // free all the memory that you allocated in the constructor
  // OpenRelTable::~OpenRelTable() {
  for (int i = 2; i < MAX_OPEN; ++i)
  {
    if (!tableMetaInfo[i].free)
      OpenRelTable::closeRel(i);
  }
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (RelCacheTable::relCache[i])
    {
      free(RelCacheTable::relCache[i]);
      RelCacheTable::relCache[i] = nullptr;
    }
  }

  for (int i = 0; i < MAX_OPEN; i++)
  {
    AttrCacheEntry *entry = AttrCacheTable::attrCache[i];
    while (entry)
    {
      AttrCacheEntry *next = entry->next;
      free(entry);
      entry = next;
    }
    AttrCacheTable::attrCache[i] = nullptr;
  }

  // }
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
  printf("came");
  return E_CACHEFULL;
  // if found return the relation id, else return E_CACHEFULL.
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]){
  // if relation with relName already has an entry in the Open Relation Table, return the rel-id
  int relId = getRelId(relName);
  if(relId != E_RELNOTOPEN){
      return relId;
  }

  // find a free slot in the Open Relation Table
  relId = getFreeOpenRelTableEntry();
  if(relId < 0){
      return E_CACHEFULL;
  }

  /******* Setting up Relation Cache entry for the free slot *********/
  // search for the entry with the relation name, relName, in the Relation Catalog using linearSearch()
  Attribute relationName;
  strcpy(relationName.sVal, relName);
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  char relCatAttrRelName[ATTR_SIZE];
  strcpy(relCatAttrRelName, RELCAT_ATTR_RELNAME);

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, relationName, EQ);

  // if the relation is not found in the Relation Catalog.
  if(relCatRecId.block == -1 && relCatRecId.slot == -1){
      return E_RELNOTEXIST;
  }

  /*
      Read the record entry corresponding to the relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RecCacheTable::recordToRelCatEntry().
      Update the recId field of this Relation Cache entry to relcatRecId.
      Use the relation cache entry to set the relId-th entry of the RelCacheTable.

      NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RecBuffer relCatBlock(relCatRecId.block); // here instead we can also use RELCAT_RELID
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, relCatRecId.slot);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId = relCatRecId;
  RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[relId]) = relCacheEntry;

  /******** Setting up Attribute Cache entry for the relation *********/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead, *current;

  /*
      Iterate over all the entries in the Attribute Catalog corresponding to each
      attribute of the relation relName by multiple calls of BlockAccess::linearSearch().
      Care should be take to reset the searchIndex of the relation, ATTRCAT_RELID,
      corresponding to Attribute Catalog before the first call to linearSearch().
  */
  RecId attrCatRecord;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  for(int i = 0; i<relCacheEntry.relCatEntry.numAttrs; i++){
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
         in the Attribute Catalog.
      */
      RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relCatAttrRelName, relationName, EQ);
      /*  read the record entry corresponding to attrcatRecId and create an
          Attribute Cache entry on it using RecBuffer::getRecord() and
          AttrCacheTable::recordToAttrCatEntry().
          update the recId field of this Attribute Cache entry to attrcatRecId.
          add the Attribute Cache entry to the linked list of listHead .
      */
    // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
    RecBuffer attrCatBlock(attrcatRecId.block);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId = attrcatRecId;

    if(i==0){
      listHead = attrCacheEntry;
      current = attrCacheEntry;
    }
    else{
      current->next = attrCacheEntry;
      current = attrCacheEntry;
    }
  }

  current->next = nullptr;

  // set the relId-th entry of the AttrCacheTable to listHead
  AttrCacheTable::attrCache[relId] = listHead;


  /********* Setting up metadata in the Open Relation Table for the relation *********/
  // update the relIdth entry of the tableMetaInfo with free as false and relName as the input.
  OpenRelTable::tableMetaInfo[relId].free = false;
  strcpy(OpenRelTable::tableMetaInfo[relId].relName, relName);

  return relId;

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
  return SUCCESS;
}
