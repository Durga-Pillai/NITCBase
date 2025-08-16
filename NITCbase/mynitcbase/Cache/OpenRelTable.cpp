#include "OpenRelTable.h"
#include<stdlib.h>
#include<iostream>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable()
{
    for(int i=0; i<MAX_OPEN; i++)
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

    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*) malloc (sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    //attribute catalog block to relation cache
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

    char attrCatName[ATTR_SIZE];
    strcpy(attrCatName, relCatRecord[ATTRCAT_REL_NAME_INDEX].sVal);

    struct RelCacheEntry attrCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &attrCacheEntry.relCatEntry);
    attrCacheEntry.recId.block = RELCAT_BLOCK;
    attrCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *) malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;

    //relation catalog to attribute cache
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *prev = nullptr;

    for(int i=0; i<RELCAT_NO_ATTRS; i++)
    {
        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheEntry * curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
        curr->recId.block = ATTRCAT_BLOCK;  
        curr->recId.slot = i;
        curr->next = nullptr;

        if(prev)
            prev->next = curr;
        else
            head = curr;
       
         prev = curr;
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    //attribute catalog to attribute cache
    AttrCacheEntry *head2 = nullptr;
    AttrCacheEntry *prev2 = nullptr;

    for (int i = 0; i < ATTRCAT_NO_ATTRS; i++) 
    {
        int slotNum = i + RELCAT_NO_ATTRS;
        attrCatBlock.getRecord(attrCatRecord, slotNum);
        AttrCacheEntry* curr = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
        curr->recId.block = ATTRCAT_BLOCK;
        curr->recId.slot = slotNum;
        curr->next = nullptr;

        if (prev2) 
            prev2->next = curr;
        else 
            head2 = curr;
        if(curr)
        prev2 = curr;
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head2;

    //students  to relation cache

    
    relCatBlock.getRecord(relCatRecord, 2);
    
    char studName[ATTR_SIZE];
    strcpy(studName, relCatRecord[0].sVal);

    struct RelCacheEntry studEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &studEntry.relCatEntry);
    studEntry.recId.block = RELCAT_BLOCK;
    studEntry.recId.slot = 2;

    RelCacheTable::relCache[2] = (struct RelCacheEntry*) malloc (sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = studEntry;

    //attributes to attribute cache
    AttrCacheEntry *head3 = nullptr;
    AttrCacheEntry *prev3 = nullptr;

    for (int i = 0; i < studEntry.relCatEntry.numAttrs; i++) 
    {
        int slotNum = i + RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS;
        attrCatBlock.getRecord(attrCatRecord, slotNum);
        AttrCacheEntry* curr = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
        curr->recId.block = ATTRCAT_BLOCK;
        curr->recId.slot = slotNum;
        curr->next = nullptr;

        if (prev3) 
            prev3->next = curr;
        else 
            head3 = curr;
        if(curr)
        prev3 = curr;
    }
    AttrCacheTable::attrCache[2] = head3;
    OpenRelTable::tableMetaInfo[2].free = false;
    strcpy(OpenRelTable::tableMetaInfo[2].relName,"Students");

}
  OpenRelTable::~OpenRelTable() {
    // free all the memory that you allocated in the constructor
   // OpenRelTable::~OpenRelTable() {
        for (int i = 0; i < MAX_OPEN; i++) {
            if (RelCacheTable::relCache[i]) {
                free(RelCacheTable::relCache[i]);
                RelCacheTable::relCache[i] = nullptr;
            }
        }
    
        for (int i = 0; i < MAX_OPEN; i++) {
            AttrCacheEntry *entry = AttrCacheTable::attrCache[i];
            while (entry) {
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
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    // if relname is RELCAT_RELNAME, return RELCAT_RELID
    if(strcmp(relName,RELCAT_RELNAME)==0) return RELCAT_RELID;
    if(strcmp(relName ,ATTRCAT_RELNAME)==0) return ATTRCAT_RELID;
    // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
    if(strcmp(relName,"Students")==0) return 2;
  //printf("getRelId came");
    return E_RELNOTOPEN;
  }