#include "Schema.h"

#include <cmath>
#include <cstring>
#include <iostream>

int Schema::openRel(char relName[ATTR_SIZE])
{
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if (ret >= 0)
  {
    return SUCCESS;
  }

  // otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE])
{
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
  {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId == E_RELNOTOPEN)
  {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE])
{
  // if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
  // return E_NOTPERMITTED

  if (strcmp(oldRelName, RELCAT_RELNAME) == 0 || strcmp(oldRelName, ATTRCAT_RELNAME) == 0 || strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, ATTRCAT_RELNAME) == 0)
  {

    return E_NOTPERMITTED;
  }
  // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
  // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)

  // if the relation is open
  //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
  //    return E_RELOPEN

  int relId = OpenRelTable::getRelId(oldRelName);
  if (relId >= 0)
    return E_RELOPEN;

  int retVal = BlockAccess::renameRelation(oldRelName, newRelName);

  return retVal;
}
int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName)
{
  // if the relName is either Relation Catalog or Attribute Catalog,
  // return E_NOTPERMITTED
  // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
  // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    return E_NOTPERMITTED;

  // if the relation is open
  //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
  //    return E_RELOPEN
  int relId = OpenRelTable::getRelId(relName);
  if (relId >= 0)
    return E_RELOPEN;

  int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
  return retVal;
  // return the value returned by the above renameAttribute() call
}

int Schema::createRel(char relName[], int nAttrs, char attrs[][ATTR_SIZE], int attrType[])
{
  Attribute relNameAsAttribute;
  strcpy(relNameAsAttribute.sVal, relName);

  RecId targetRelId = {-1, -1};

  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  char relCatAttrRelName[ATTR_SIZE] = RELCAT_ATTR_RELNAME;
  targetRelId = BlockAccess::linearSearch(RELCAT_RELID, relCatAttrRelName, relNameAsAttribute, EQ);

  // checking whether a table with that name already exists
  if (targetRelId.block != -1 && targetRelId.slot != -1)
  {
    return E_RELEXIST;
  }

  // checking whether the given list of attributes itself contains any duplicates
  for (int i = 0; i < nAttrs; i++)
  {
    for (int j = i + 1; j < nAttrs; j++)
    {
      if (strcmp(attrs[i], attrs[j]) == 0)
      {
        return E_DUPLICATEATTR;
      }
    }
  }

  // created the rel cat entry to be inserted in the relation catalog
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
  relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
  relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
  relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
  relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
  relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor((2016 * 1.00) / (16 * nAttrs + 1));

  // puts the created relation catalog entry in relation catalog
  int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
  if (retVal != SUCCESS)
  {
    return retVal;
  }

  // inserting each attribute into the attribute catalog
  for (int i = 0; i < nAttrs; i++)
  {
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
    strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);
    attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrType[i];
    attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
    attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
    attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

    retVal = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
    // if insertion fails, the relation catalog entry in relation catalog should be deleted
    if (retVal != SUCCESS)
    {
      Schema::deleteRel(relName);
      return retVal;
    }
  }

  return SUCCESS;
}

int Schema::deleteRel(char *relName)
{
  if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
  {
    return E_NOTPERMITTED;
  }
  int relId = OpenRelTable::getRelId(relName);
  if (relId >= 0 && relId <= MAX_OPEN)
    return E_RELOPEN;
  int ret = BlockAccess::deleteRelation(relName);
  return ret;
}

int Schema::createIndex(char relName[ATTR_SIZE], char attrName[ATTR_SIZE])
{
  if (strcmp(relName, RELCAT_RELNAME) == 0 or strcmp(relName, ATTRCAT_RELNAME) == 0)
    return E_NOTPERMITTED;
  // get the relation's rel-id using OpenRelTable::getRelId() method
  int relId = OpenRelTable::getRelId(relName);
  if (relId == E_RELNOTOPEN)
    return relId;

  // create a bplus tree using BPlusTree::bPlusCreate() and return the value
  return BPlusTree::bPlusCreate(relId, attrName);
}

int Schema::dropIndex(char *relName, char *attrName)
{

  if (strcmp(relName, RELCAT_RELNAME) == 0 or strcmp(relName, ATTRCAT_RELNAME) == 0)
    return E_NOTPERMITTED;
  // get the relation's rel-id using OpenRelTable::getRelId() method
  int relId = OpenRelTable::getRelId(relName);
  if (relId == E_RELNOTOPEN)
    return relId;
  AttrCatEntry attrCatentry;
  int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatentry);
  if(ret != SUCCESS) return E_ATTRNOTEXIST;
  // if getAttrCatEntry() fails, return E_ATTRNOTEXIST

  int rootBlock = attrCatentry.rootBlock;

  if (rootBlock == -1)
  {
    return E_NOINDEX;
  }

  // destroy the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
  BPlusTree::bPlusDestroy(rootBlock);

  // set rootBlock = -1 in the attribute cache entry of the attribute using
  // AttrCacheTable::setAttrCatEntry()
  attrCatentry.rootBlock =-1;
  AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatentry);

  return SUCCESS;
}

