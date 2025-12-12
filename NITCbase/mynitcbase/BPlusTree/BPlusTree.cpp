#include "BPlusTree.h"
#include <iostream>

#include <cstring>
int BPlusTree::numComp = 0;
RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    // declare searchIndex which will be used to store search index for attrName.
    IndexId searchIndex;

    // get the searchindex crsponding to attrname and relid
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);
    AttrCatEntry attrCatEntry;
    // load the attribute chache into attrCatEntry
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    // declare variables block and index which will be used during search
    int block = -1, index = -1;

    if (searchIndex.block == -1 && searchIndex.index == -1)
    {

        // search for first time
        // start the search from the first entry of root.
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1)
        {
            return RecId{-1, -1};
        }
    }
    else
    {
        // not first hit , continue from previous hit
        block = searchIndex.block;
        index = searchIndex.index + 1; // search is resumed from the next index.

        // load block into leaf using IndLeaf::IndLeaf().
        IndLeaf leaf(block);

        // declare leafHead which will be used to hold the header of leaf.
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            // all entries in block searched , search from
            // begining of next leadf index block
            // update block to rblock of current block and index to 0.
            block = leafHead.rblock;
            index = 0;
            if (block == -1)
            {
                // (end of linked list reached - the search is done.)
                return RecId{-1, -1};
            }
        }
    }

    /******  Traverse through all the internal nodes according to value
             of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    { // use StaticBuffer::getStaticBlockType()

        // load the block into internalBlk using IndInternal::IndInternal().
        IndInternal internalBlk(block);

        HeadInfo intHead;
        internalBlk.getHeader(&intHead);
        // declare intEntry which will be used to store an entry of internalBlk.
        InternalEntry intEntry;

        if (op == LE || op == LT || op == NE)
        {

            // if the op is LE ,NE,LT always move to left

            // load entry in the first slot of the block into intEntry
            // using IndInternal::getEntry().
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else
        {
            int entryindex = 0;

            // loop through all entries in internal index block to find the path
            while (entryindex < intHead.numEntries)
            {
                int ret = internalBlk.getEntry(&intEntry, entryindex);
                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);
                BPlusTree::numComp++;
                if ((op == EQ && cmpVal >= 0) or (op == GE && cmpVal >= 0) or (op == GT and cmpVal > 0))
                    break;
                entryindex++;
            }

            if (entryindex < intHead.numEntries)
            {
                // move to the left child of that entry
                block = intEntry.lChild; // left child of the entry
            }
            else
            {
                // move to the right child of the last entry of the block
                // i.e numEntries - 1 th entry of the block

                block = intEntry.rChild; // right child of last entry
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block.

    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while (block != -1)
    {
        // load the block into leafBlk using IndLeaf::IndLeaf().
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        // declare leafEntry which will be used to store an entry from leafBlk
        Index leafEntry;

        while (index < leafHead.numEntries)
        {

            // load entry corresponding to block and index into leafEntry
            // using IndLeaf::getEntry().
            leafBlk.getEntry(&leafEntry, index);

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);
            BPlusTree::numComp++;

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0))
            {
                searchIndex.block = block;
                searchIndex.index = index;

                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                return RecId{leafEntry.block, leafEntry.slot};
            }

            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */

                return RecId{-1, -1};
            }

            // search next index.
            ++index;
        }

        if (op != NE)
            break;

        block = leafHead.rblock; // next block in the linked list, i.e., the rblock in leafHead.
        index = 0;
    }

    return RecId{-1, -1};
}

// no entry satisying the op was found; return the recId {-1,-1}

// STAGE 11 //
int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE])
{

    if (relId == RELCAT_RELID or relId == ATTRCAT_RELID)
    {
        return E_NOTPERMITTED;
    }
    AttrCatEntry attrCatentry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatentry);
    if (ret != SUCCESS)
        return ret;
    if (attrCatentry.rootBlock != -1)
    {
        return SUCCESS;
    }

    /******Creating a new B+ Tree ******/

    // get a free leaf block using constructor 1 to allocate a new block
    IndLeaf rootBlockBuf;

    // (if the block could not be allocated, the appropriate error code
    //  will be stored in the blockNum member field of the object)

    // declare rootBlock to store the blockNumber of the new leaf block
    int rootBlock = rootBlockBuf.getBlockNum();

    // if there is no more disk space for creating an index
    if (rootBlock == E_DISKFULL)
    {
        return E_DISKFULL;
    }
    attrCatentry.rootBlock=rootBlock;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatentry);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;
    int numofSlots = relCatEntry.numSlotsPerBlk;
    int numAttrs = relCatEntry.numAttrs;

    /***** Traverse all the blocks in the relation and insert them one
           by one into the B+ Tree *****/
    while (block != -1)
    {

        // declare a RecBuffer object for `block` (using appropriate constructor)
        RecBuffer currentBlock(block);
        unsigned char slotMap[numofSlots];
        currentBlock.getSlotMap(slotMap);

        for (int slot = 0; slot < numofSlots; slot++)
        {
            if (slotMap[slot] == SLOT_OCCUPIED)
            {
                Attribute record[numAttrs];
                currentBlock.getRecord(record, slot);

                RecId rec_id{block, slot};

                // insert into BplusTree
                ret = BPlusTree::bPlusInsert(relId, attrName, record[attrCatentry.offset], rec_id);
                // retVal = bPlusInsert(relId, attrName, attribute value, recId);
                if (ret == E_DISKFULL)
                    return E_DISKFULL;
                // if (retVal == E_DISKFULL) {
                //     // (unable to get enough blocks to build the B+ Tree.)
                //     return E_DISKFULL;
                // }
            }
        }

        // get the header of the block using BlockBuffer::getHeader()
        HeadInfo header;
        currentBlock.getHeader(&header);
        block = header.rblock;
        // set block = rblock of current block (from the header)
    }

    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum)
{
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);
    if (type == IND_LEAF)
    {
        // declare an instance of IndLeaf for rootBlockNum using appropriate
        // constructor
        IndLeaf leafblock(rootBlockNum);
        leafblock.releaseBlock();

        // release the block using BlockBuffer::releaseBlock().

        return SUCCESS;
    }
    else if (type == IND_INTERNAL)
    {
        // declare an instance of IndInternal for rootBlockNum using appropriate
        // constructor
        IndInternal internalblock(rootBlockNum);
        HeadInfo header;
        internalblock.getHeader(&header);
        // release left and right blocsks recursively
        InternalEntry entry;
        internalblock.getEntry(&entry, 0);
        BPlusTree::bPlusDestroy(entry.lChild);
        for (int index = 0; index < header.numEntries; index++)
        {
            internalblock.getEntry(&entry, index);
            BPlusTree::bPlusDestroy(entry.rChild);
        }
        // release the block using BlockBuffer::releaseBlock().
        internalblock.releaseBlock();
        return SUCCESS;
    }
    else
    {
        // (block is not an index block.)
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId)
{
    AttrCatEntry attrCatentry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatentry);
    if (ret != SUCCESS)
        return ret;
    int blockNum = attrCatentry.rootBlock;
 
    if (blockNum == -1)
    {
        return E_NOINDEX;
    }

    // find the leaf block to which insertion is to be done using the
    // findLeafToInsert() function

    int leafBlkNum = findLeafToInsert(blockNum, attrVal, attrCatentry.attrType);

    Index indexentry;
    indexentry.attrVal = attrVal;
    indexentry.block = recId.block;
    indexentry.slot = recId.slot;

    if (insertIntoLeaf(relId, attrName, leafBlkNum, indexentry) == E_DISKFULL)
    {
        // destroy the existing B+ tree by passing the rootBlock to bPlusDestroy().
        BPlusTree::bPlusDestroy(blockNum);
        attrCatentry.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatentry);
        // update the rootBlock of attribute catalog cacheen entry to -1 using
        // AttrCacheTable::setAttrCatEntry().

        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType)
{
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF)
    { // use StaticBuffer::getStaticBlockType()

        // declare an IndInternal object for block using appropriate constructor
        IndInternal internalblock(blockNum);
        HeadInfo header;
        internalblock.getHeader(&header);
        // get header of the block using BlockBuffer::getHeader()
        int index = 0;
        while (index < header.numEntries)
        {
            InternalEntry entry;
            internalblock.getEntry(&entry, index);
            if (compareAttrs(attrVal, entry.attrVal, attrType) <= 0)
                break;
            index++;
        }

        if (index == header.numEntries)
        {
            // none of the entries are greater than attrval,the choose the right most child
            InternalEntry lentry;
            internalblock.getEntry(&lentry, header.numEntries-1);
            blockNum = lentry.rChild;
            // set blockNum = rChild of (nEntries-1)'th entry of the block
            // (i.e. rightmost child of the block)
        }
        else
        {
            // choode left child of that entry
            InternalEntry lentry;
            internalblock.getEntry(&lentry, index);
            blockNum = lentry.lChild;
            // set blockNum = lChild of the entry that was found
        }
    }

    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry)
{
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrcatentry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrcatentry);

    IndLeaf leafblock(blockNum);

    // declare an IndLeaf instance for the block using appropriate constructor

    HeadInfo blockHeader;
    leafblock.getHeader(&blockHeader);
    // store the header of the leaf index block into blockHeader
    // using BlockBuffer::getHeader()

    // the following variable will be used to store a list of index entries with
    // existing indices + the new index to insert
    Index indices[blockHeader.numEntries + 1];
    int inserted = 0;

   //iterate thrgh all entries
   //keep the sorted order
   for(int i=0;i<blockHeader.numEntries;i++){
    Index entry;
    leafblock.getEntry(&entry,i);
    if(compareAttrs(entry.attrVal,indexEntry.attrVal,attrcatentry.attrType)<=0){
        indices[i] = entry;
    }else{
        indices[i]=indexEntry;
        inserted = 1;

        for(i++;i<blockHeader.numEntries;i++){
            leafblock.getEntry(&entry,i-1);
            indices[i] = entry;
        }
        break;
    }
   }
   // if not inserted all values are smaller than the block so 
   // insert in last pos
   if(!inserted){
        indices[blockHeader.numEntries] = indexEntry;
   }

    if (blockHeader.numEntries != MAX_KEYS_LEAF)
    {

        // increment blockHeader.numEntries and update the header of block
        // using BlockBuffer::setHeader().
        blockHeader.numEntries++;
        leafblock.setHeader(&blockHeader);
        // iterate through all the entries of the array `indices` and populate the
        // entries of block with them using IndLeaf::setEntry().
        for(int i=0;i<blockHeader.numEntries;i++){
            leafblock.setEntry(&indices[i],i);
        }

        return SUCCESS;
    }

    // If we reached here, the `indices` array has more than entries than can fit
    // in a single leaf index block. Therefore, we will need to split the entries
    // in `indices` between two leaf blocks. We do this using the splitLeaf() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitLeaf(blockNum, indices);

    // if splitLeaf() returned E_DISKFULL
    //     return E_DISKFULL
    if(newRightBlk == E_DISKFULL) return E_DISKFULL;
    if (blockHeader.pblock != -1)
    { // check pblock in header
       InternalEntry middleEntry;
       middleEntry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
       middleEntry.lChild = blockNum;
       middleEntry.rChild = newRightBlk;
       return BPlusTree::insertIntoInternal(relId,attrName,blockHeader.pblock,middleEntry);


    }
    else
    {
       return BPlusTree::createNewRoot(relId,attrName,indices[MIDDLE_INDEX_LEAF].attrVal,blockNum,newRightBlk);
    }

    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
    //new right block and get the block for left
    IndLeaf rightblock;
    IndLeaf leftblock(leafBlockNum);
    

    int rightBlkNum = rightblock.getBlockNum();
    int leftBlkNum = leftblock.getBlockNum();

    if (rightBlkNum == E_DISKFULL) {
        //(failed to obtain a new leaf index block because the disk is full)
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    rightblock.getHeader(&rightBlkHeader);
    leftblock.getHeader(&leftBlkHeader);

    // set rightBlkHeader with the following values
    // - number of entries = (MAX_KEYS_LEAF+1)/2 = 32,
    // - pblock = pblock of leftBlk
    // - lblock = leftBlkNum
    // - rblock = rblock of leftBlk
    // and update the header of rightBlk using BlockBuffer::setHeader()
    rightBlkHeader.blockType=leftBlkHeader.blockType;
    rightBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock=leftBlkNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightblock.setHeader(&rightBlkHeader);

    // set leftBlkHeader with the following values
    // - number of entries = (MAX_KEYS_LEAF+1)/2 = 32
    // - rblock = rightBlkNum
    // and update the header of leftBlk using BlockBuffer::setHeader() */
    leftBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
    leftBlkHeader.rblock = rightBlkNum;
    leftblock.setHeader(&leftBlkHeader);

    // set the first 32 entries of leftBlk = the first 32 entries of indices array
    // and set the first 32 entries of newRightBlk = the next 32 entries of
    // indices array using IndLeaf::setEntry().
    for(int i=0;i<(MAX_KEYS_LEAF+1)/2;i++){
        leftblock.setEntry(&indices[i],i);
        rightblock.setEntry(&indices[i+MIDDLE_INDEX_LEAF+1],i);
    }

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
    // get the attribute cache entry corresponding to attrName
    // using AttrCacheTable::getAttrCatEntry().
    AttrCatEntry attrcatentry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatentry);


    // declare intBlk, an instance of IndInternal using constructor 2 for the block
    // corresponding to intBlockNum
    IndInternal indBlock(intBlockNum);

    HeadInfo blockHeader;
    indBlock.getHeader(&blockHeader);
    // load blockHeader with header of intBlk using BlockBuffer::getHeader().

    // declare internalEntries to store all existing entries + the new entry
    InternalEntry internalEntries[blockHeader.numEntries + 1];

    //iterate through all entries and insert in corrct position
    int inserted = -1;
    for(int entryindex =0;entryindex<blockHeader.numEntries;entryindex++){
        InternalEntry internalblockentry;
        indBlock.getEntry(&internalblockentry,entryindex);
        if(compareAttrs(internalblockentry.attrVal,intEntry.attrVal,attrcatentry.attrType)<=0){
            internalEntries[entryindex]=internalblockentry;
        }
        else{
            internalEntries[entryindex]=intEntry;
            inserted = entryindex;
            for(entryindex++;entryindex<=blockHeader.numEntries;entryindex++){
                indBlock.getEntry(&internalblockentry,entryindex-1);
                internalEntries[entryindex]=internalblockentry;
            }
            break;
        }
    }
    if(inserted == -1){
        internalEntries[blockHeader.numEntries] = intEntry;
        inserted = blockHeader.numEntries;
    }
    if(inserted > 0){
        internalEntries[inserted-1].rChild = intEntry.lChild;
    }
    if(inserted<blockHeader.numEntries){
        internalEntries[inserted+1].lChild=intEntry.rChild;

    }


    if (blockHeader.numEntries != MAX_KEYS_INTERNAL) {
        // (internal index block has not reached max limit)

        // increment blockheader.numEntries and update the header of intBlk
        // using BlockBuffer::setHeader().
        blockHeader.numEntries++;
        indBlock.setHeader(&blockHeader);

        // iterate through all entries in internalEntries array and populate the
        // entries of intBlk with them using IndInternal::setEntry().
        for(int entryindex =0;entryindex<blockHeader.numEntries;entryindex++){
            indBlock.setEntry(&internalEntries[entryindex],entryindex);
        }

        return SUCCESS;
    }

    // If we reached here, the `internalEntries` array has more than entries than
    // can fit in a single internal index block. Therefore, we will need to split
    // the entries in `internalEntries` between two internal index blocks. We do
    // this using the splitInternal() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk == E_DISKFULL) {

        // Using bPlusDestroy(), destroy the right subtree, rooted at intEntry.rChild.
        // This corresponds to the tree built up till now that has not yet been
        // connected to the existing B+ Tree
        BPlusTree::bPlusDestroy(intEntry.rChild);

        return E_DISKFULL;
    }

    if (blockHeader.pblock != -1) {  // (check pblock in header)
        // insert the middle value from `internalEntries` into the parent block
        // using the insertIntoInternal() function (recursively).
        InternalEntry middleEntry;
        middleEntry.lChild = intBlockNum;
        middleEntry.rChild = newRightBlk;
        middleEntry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        return insertIntoInternal(relId,attrName,blockHeader.pblock,middleEntry);
    } else {
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        // To do this, call the createNewRoot() function with the following arguments
        return createNewRoot(relId,attrName,internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,intBlockNum,newRightBlk);
        // createNewRoot(relId, attrName,
        //               internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,
        //               current block, new right block)
    }

    // if either of the above calls returned an error (E_DISKFULL), then return that
    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) {
    // declare rightBlk, an instance of IndInternal using constructor 1 to obtain new
    // internal index block that will be used as the right block in the splitting
    IndInternal rightblock;
    IndInternal leftblock(intBlockNum);
    // declare leftBlk, an instance of IndInternal using constructor 2 to read from
    // the existing internal index block


    int rightBlkNum = rightblock.getBlockNum();
    int leftBlkNum = leftblock.getBlockNum();

    if (rightBlkNum == E_DISKFULL) {
        //(failed to obtain a new internal index block because the disk is full)
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    // get the headers of left block and right block using BlockBuffer::getHeader()
    leftblock.getHeader(&leftBlkHeader);
    rightblock.getHeader(&rightBlkHeader);

   int newNumEntries = MAX_KEYS_INTERNAL/2;
   rightBlkHeader.numEntries=newNumEntries;
   rightBlkHeader.pblock=leftBlkHeader.pblock;
   rightblock.setHeader(&rightBlkHeader);

   leftBlkHeader.numEntries = newNumEntries;
   leftBlkHeader.rblock = rightBlkNum;
   leftblock.setHeader(&leftBlkHeader);
   
   for(int i=0;i<MIDDLE_INDEX_INTERNAL;i++){
    leftblock.setEntry(&internalEntries[i],i);
    rightblock.setEntry(&internalEntries[i+MIDDLE_INDEX_INTERNAL+1],i);
   }

    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild);
    //            (use StaticBuffer::getStaticBlockType())
    BlockBuffer blockbuffer(internalEntries[MIDDLE_INDEX_INTERNAL+1].lChild);
    HeadInfo blockheader;
    blockbuffer.getHeader(&blockheader);
    blockheader.pblock = rightBlkNum;
    blockbuffer.setHeader(&blockheader);

    for (int i=0;i<newNumEntries;i++) {
        // declare an instance of BlockBuffer to access the child block using
        // constructor 2
        BlockBuffer blockbuffer(internalEntries[i+MIDDLE_INDEX_INTERNAL+1].rChild);

        blockbuffer.getHeader(&blockheader);
        blockheader.pblock=rightBlkNum;
        blockbuffer.setHeader(&blockheader);


        // update pblock of the block to rightBlkNum using BlockBuffer::getHeader()
        // and BlockBuffer::setHeader().
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
    AttrCatEntry attrcatentry;
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatentry);

    IndInternal newRootBlk;

    // declare newRootBlk, an instance of IndInternal using appropriate constructor
    // to allocate a new internal index block on the disk

    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL) {
        // (failed to obtain an empty internal index block because the disk is full)

       BPlusTree::bPlusDestroy(rChild);

        return E_DISKFULL;
    }

    // update the header of the new block with numEntries = 1 using
    // BlockBuffer::getHeader() and BlockBuffer::setHeader()
    HeadInfo header;
    newRootBlk.getHeader(&header);
    header.numEntries =1;
    newRootBlk.setHeader(&header);

    // create a struct InternalEntry with lChild, attrVal and rChild from the
    // arguments and set it as the first entry in newRootBlk using IndInternal::setEntry()
    InternalEntry  entry;
    entry.attrVal = attrVal;
    entry.lChild = lChild;
    entry.rChild = rChild;
    newRootBlk.setEntry(&entry,0);

    // declare BlockBuffer instances for the `lChild` and `rChild` blocks using
    // appropriate constructor and update the pblock of those blocks to `newRootBlkNum`
    // using BlockBuffer::getHeader() and BlockBuffer::setHeader()
    BlockBuffer leftblock(lChild),rightblock(rChild);
    HeadInfo leftheader,rightheader;
    leftblock.getHeader(&leftheader);
    leftheader.pblock = newRootBlkNum;
    leftblock.setHeader(&leftheader);

    rightblock.getHeader(&rightheader);
    rightheader.pblock = newRootBlkNum;
    rightblock.setHeader(&rightheader);


    // update rootBlock = newRootBlkNum for the entry corresponding to `attrName`
    // in the attribute cache using AttrCacheTable::setAttrCatEntry().
    attrcatentry.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrcatentry);

    return SUCCESS;
}


