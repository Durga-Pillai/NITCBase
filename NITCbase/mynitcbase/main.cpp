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
    

  StaticBuffer buffer;
  OpenRelTable cache;
  
  

  //STAGE 3 
  // for (int i = 0; i <= 2; i++)
  // {
  //   RelCatEntry relCatBuffer;
  //    RelCacheTable::getRelCatEntry(i,&relCatBuffer);
  //       printf("Relation[%d]: %s\n",i, relCatBuffer.relName);
  //   //printf("%d",relCatBuffer.numAttrs);
  //    for(int j=0;j<relCatBuffer.numAttrs;j++) {
  //    // printf("2nd loop");
  //     AttrCatEntry attrCatBuffer[ATTRCAT_NO_ATTRS];
  //      AttrCacheTable::getAttrCatEntry(i,j,attrCatBuffer);
  //      const char* attrType = attrCatBuffer->attrType == NUMBER? "NUM" : "STR";
  //      printf("  %s: %s\n", attrCatBuffer->attrName, attrType);
                                                   
  //    }
  //    }
   
  //STAGE 4
  
  return FrontendInterface::handleFrontend(argc, argv);
   }
