#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstring>
using namespace std;

int main(int argc, char *argv[])
{
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;
  // unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer,2048);
  // //char message[] = "hello";
  // memcpy(buffer+20,message,6);
  // Disk::writeBlock(buffer,7000);

  //unsigned char buffer2[BLOCK_SIZE];
  // unsigned char buffer2[4];
  /*STAGE 1 EXERCISE*/
  //Disk::readBlock(buffer2, 0);
  // int message2;
  //  for (int i = 0; i < 6; i++)
  //  {
  //     //memcpy(&message2, buffer2+i, 1);
  //    unsigned char message2=buffer2[i];
  //    std::cout << (int)message2 << " ";
  //  }

  // RecBuffer relCatBuffer(RELCAT_BLOCK);
  // RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
  // HeadInfo relCatHeader;
  // HeadInfo attrCatHeader;
  // relCatBuffer.getHeader(&relCatHeader);
  // attrCatBuffer.getHeader(&attrCatHeader);
  // int attrCatSlotIndex = 0;
  // for(int i=0;i<relCatHeader.numEntries;i++){
  //   Attribute relCatRecord[RELCAT_NO_ATTRS];
  //   relCatBuffer.getRecord(relCatRecord,i);
  //   cout << i<< endl;
  //   printf("Relation: %s\n",relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
  //   int j=0;
  //   for(;j<relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;j++,attrCatSlotIndex++) {
  //     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  //     attrCatBuffer.getRecord(attrCatRecord,attrCatSlotIndex);
  //     //printf("Attributes :%s\n",attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal);

  //     if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0){
  //       const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
  //       printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
  //     }
  //   // relCatBuffer =attrCatHeader.rblock;
  //   }
  //   printf("\n");
  // }
  /*STAGE 2 EXERCISE*/
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  int totRel = relCatHeader.numEntries;

  vector<Attribute *>allAttrRecords;
  int attrBlockNum = ATTRCAT_BLOCK;

  while(attrBlockNum != -1)
  {
    RecBuffer attrCatBuffer(attrBlockNum);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);

    for (int j = 0; j < attrCatHeader.numEntries; ++j) 
    {
      Attribute* attrRec = new Attribute[ATTRCAT_NO_ATTRS];
      if (attrCatBuffer.getRecord(attrRec, j) == SUCCESS){ 
      const char *attrName = attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal;
      
      if( strcmp(attrName,"Class")==0) {
        
        memset(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal,0,ATTR_SIZE);
        memcpy(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch",6);
        attrCatBuffer.setRecord(attrRec,j);
      }
      allAttrRecords.push_back(attrRec);
    }
      else 
        delete[] attrRec;
      }

      attrBlockNum = attrCatHeader.rblock;
    }
    for (int i = 0; i < totRel; ++i) {
      Attribute relCatRecord[RELCAT_NO_ATTRS];
      if (relCatBuffer.getRecord(relCatRecord, i) != SUCCESS)
        continue;
  
      const char *relName = relCatRecord[RELCAT_REL_NAME_INDEX].sVal;
      printf("Relation: %s\n", relName);
      int k =0;
      for (auto attrRec : allAttrRecords) 
      {
        const char *recRelName = attrRec[ATTRCAT_REL_NAME_INDEX].sVal;
        if (strcmp(recRelName, relName) != 0) continue;
  
        const char *attrName = attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal;
        // if( strcmp(attrName,"Class")==0) {
        //   memset(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal,0,ATTR_SIZE);
        //   memcpy(attrRec[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch",6);
        //   attrCatBuffer.setRecord(attrRec,k);
        // }
        // k++;

        int type = attrRec[ATTRCAT_ATTR_TYPE_INDEX].nVal;
        const char *attrType = (type == NUMBER ? "NUM" : "STR");
        
        printf("  %s : %s\n", attrName, attrType);
      }
    
    }
  printf("\n");

  return 0;

  // return FrontendInterface::handleFrontend(argc, argv);
}
