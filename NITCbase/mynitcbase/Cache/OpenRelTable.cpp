#include "OpenRelTable.h"
#include<stdlib.h>

#include <cstring>
// OpenRelTable::OpenRelTable() {

//     // initialize relCache and attrCache with nullptr
//     for (int i = 0; i < MAX_OPEN; ++i) {
//       RelCacheTable::relCache[i] = nullptr;
//       AttrCacheTable::attrCache[i] = nullptr;
//     }
  
//     /************ Setting up Relation Cache entries ************/
//     // (we need to populate relation cache with entries for the relation catalog
//     //  and attribute catalog.)
  
//     /**** setting up Relation Catalog relation in the Relation Cache Table****/
//     RecBuffer relCatBlock(RELCAT_BLOCK);
  
//     Attribute relCatRecord[RELCAT_NO_ATTRS];
//     relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  
//     struct RelCacheEntry relCacheEntry;
//     RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
//     relCacheEntry.recId.block = RELCAT_BLOCK;
//     relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
  
//     // allocate this on the heap because we want it to persist outside this function
//     RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
//     *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;
  
//     /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
  
//     // set up the relation cache entry for the attribute catalog similarly
//     RecBuffer attrCatBlock(ATTRCAT_BLOCK);
//     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//     attrCatBlock.getRecord(attrCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);


//     // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  
//     // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  
  
//     /************ Setting up Attribute cache entries ************/
//     // (we need to populate attribute cache with entries for the relation catalog
//     //  and attribute catalog.)
  
//     /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
//     RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  
//     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  
//     // iterate through all the attributes of the relation catalog and create a linked
//     // list of AttrCacheEntry (slots 0 to 5)
//     // for each of the entries, set
//     //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
//     //    attrCacheEntry.recId.slot = i   (0 to 5)
//     //    and attrCacheEntry.next appropriately
//     // NOTE: allocate each entry dynamically using malloc
  
//     // set the next field in the last entry to nullptr
  
//     AttrCacheTable::attrCache[RELCAT_RELID] = /* head of the linked list */;
  
//     /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
  
//     // set up the attributes of the attribute cache similarly.
//     // read slots 6-11 from attrCatBlock and initialise recId appropriately
  
//     // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
//   }

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
        curr->recId.block = ATTRCAT_BLOCK;  //CONFUSION
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

    for (int i = 0; i < 4; i++) 
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