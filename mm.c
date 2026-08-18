#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <unistd.h>   /* for getpagesize() = 4096 no my system*/
#include <sys/mman.h> /* for mmap() call*/
#include "mm.h"

static size_t SYSTEM_PAGE_SIZE = 0; // this is in bytes
static vm_page_for_families_t *first_vm_page_for_families = NULL;

void mm_initializer()
{
    SYSTEM_PAGE_SIZE = getpagesize();
}

// a function to request Virtual Memory Page from the kernel.
static void *get_new_vm_page_from_kernel(int units)
{
    char *vm_page = mmap(
        NULL,
        units * SYSTEM_PAGE_SIZE, // in bytes
        PROT_EXEC | PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        0, 0);

    if (vm_page == MAP_FAILED)
    {
        printf("Error : Virtual Memory Page Allocation failed.\n");
        return NULL;
    }
    memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
    return (void *)vm_page;
}

// function to release the VM page back to the kernel.
void return_vm_page_to_kernel(void *vm_page, int units)
{
    int dealloc = munmap(vm_page, units * SYSTEM_PAGE_SIZE); // in bytes, returns '0' on success.
    if (dealloc == 0)
    {
        printf("Page successfully deallocated, and returned back to the kernel.\n");
    }
    else
    {
        printf("Error : Could not munmap VM page back to kernel.\n");
    }
}

void mm_instantiate_new_page_family(char *struct_name, __uint32_t struct_size)
{
    page_family_t *curr_family_iterator = NULL;
    vm_page_for_families_t *new_vm_page_for_families = NULL;
    if (struct_size > SYSTEM_PAGE_SIZE)
    {
        printf("Error : %s() Structure %s size exceeds the system page size\n",
               __FUNCTION__, struct_name);
        return;
    }

    if (first_vm_page_for_families == NULL) // first family handling
    {
        first_vm_page_for_families = (vm_page_for_families_t *)get_new_vm_page_from_kernel(1);
        first_vm_page_for_families->next = NULL;
        strncpy(first_vm_page_for_families->page_family[0].struct_name, struct_name, MM_MAX_STRUCTNAME_SIZE);
        first_vm_page_for_families->page_family[0].struct_size = struct_size;
        init_glthread(&first_vm_page_for_families->page_family[0].free_block_pq_list_head);
        // initializing the head of the doubly linked list priority queue.
        return;
    }

    __uint32_t count_of_families_in_vm_page = 0;
    ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, curr_family_iterator)
    {
        if (strncmp(curr_family_iterator->struct_name, struct_name, MM_MAX_STRUCTNAME_SIZE) != 0)
        {
            count_of_families_in_vm_page++;
            continue;
        }
        assert(0); // if the application registers the same page-family twice - error
    }
    ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, curr_family_iterator);

    if (count_of_families_in_vm_page == MAX_FAMILIES_PER_VM_PAGE) // we need a new page.
    {
        vm_page_for_families_t *new_allocated_vm_page = (vm_page_for_families_t *)get_new_vm_page_from_kernel(1);
        new_allocated_vm_page->next = first_vm_page_for_families;
        first_vm_page_for_families = new_allocated_vm_page;
        curr_family_iterator = &first_vm_page_for_families->page_family[0];
    }

    curr_family_iterator->struct_size = struct_size;
    strncpy(curr_family_iterator->struct_name, struct_name, MM_MAX_STRUCTNAME_SIZE);
    init_glthread(&curr_family_iterator->free_block_pq_list_head);
    return;
}

void mm_print_registered_page_families()
{
    vm_page_for_families_t *traversing_page = first_vm_page_for_families;
    while (traversing_page)
    {
        // page_family_t *curr_family = &traversing_page->page_family[0];
        page_family_t *curr_family = NULL;
        ITERATE_PAGE_FAMILIES_BEGIN(traversing_page, curr_family)
        {
            printf("Page_Family : %s | Family_Size : %u\n", curr_family->struct_name, curr_family->struct_size);
        }
        ITERATE_PAGE_FAMILIES_END(traversing_page, curr_family);
        traversing_page = traversing_page->next;
    }
}

page_family_t *lookup_page_family_by_name(char *family_name)
{
    vm_page_for_families_t *traversing_page = first_vm_page_for_families;
    while (traversing_page)
    {
        page_family_t *curr_family = NULL;
        ITERATE_PAGE_FAMILIES_BEGIN(traversing_page, curr_family)
        {
            if (strncmp(curr_family->struct_name, family_name, MM_MAX_STRUCTNAME_SIZE) == 0)
            {
                return curr_family;
            }
        }
        ITERATE_PAGE_FAMILIES_END(traversing_page, curr_family);
        traversing_page = traversing_page->next;
    }
    return NULL;
}

// merges 'second' into 'first' when both are free and physically adjacent (first->next_block == second)
void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second)
{
    assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);

    first->next_block = second->next_block;
    if (second->next_block)
    {
        second->next_block->previous_block = first;
    }
    first->data_block_size += second->data_block_size + sizeof(block_meta_data_t);
}

vm_bool_t mm_is_vm_page_empty(vm_page_t *vm_page)
{
    if (vm_page->meta_block.is_free == MM_TRUE &&
        vm_page->meta_block.next_block == NULL &&
        vm_page->meta_block.previous_block == NULL)
    {
        return MM_TRUE;
    }
    return MM_FALSE;
}

static __uint32_t mm_max_page_allocatable_memory(int units)
{
    // __uint32_t memory_available = (units*SYSTEM_PAGE_SIZE) - (sizeof(vm_page_t)); -- this too works
    __uint32_t memory_available = (units * SYSTEM_PAGE_SIZE) - OFFSET_OF(vm_page_t, page_memory);
    return memory_available;
}

vm_page_t *allocate_vm_page(page_family_t *page_family)
{
    vm_page_t *new_allocated_vm_page = (vm_page_t *)get_new_vm_page_from_kernel(1);
    if (new_allocated_vm_page == NULL)
    {
        return NULL;
    }
    new_allocated_vm_page->pg_family = page_family;
    init_glthread(&new_allocated_vm_page->meta_block.free_block_pq_list_glue);
    MARK_VM_PAGE_EMPTY(new_allocated_vm_page);
    new_allocated_vm_page->next = page_family->first_page;
    if (page_family->first_page != NULL)
        page_family->first_page->prev = new_allocated_vm_page;

    page_family->first_page = new_allocated_vm_page;
    new_allocated_vm_page->prev = NULL;

    new_allocated_vm_page->meta_block.offset = OFFSET_OF(vm_page_t, meta_block);
    new_allocated_vm_page->meta_block.data_block_size = mm_max_page_allocatable_memory(1);

    return new_allocated_vm_page;
}

void mm_vm_page_delete_and_free(vm_page_t *vm_page)
{
    page_family_t *family = vm_page->pg_family;
    //  deletion of the head.
    if (vm_page->prev == NULL)
    {
        family->first_page = vm_page->next;
        if (vm_page->next != NULL)
            vm_page->next->prev = NULL;
        vm_page->next = NULL;
        vm_page->pg_family = NULL;
    }
    else
    {
        if (vm_page->next == NULL)
        {
            vm_page->prev->next = NULL;
            vm_page->prev = NULL;
        }
        else
        {
            vm_page->prev->next = vm_page->next;
            vm_page->next->prev = vm_page->prev;
            vm_page->next = NULL;
            vm_page->prev = NULL;
        }
    }
    return_vm_page_to_kernel(vm_page, 1);
}

static int free_blocks_comparison_function(void *Block_1, void *Block_2)
{
    block_meta_data_t *freeBlock_1 = (block_meta_data_t *)Block_1;
    block_meta_data_t *freeBlock_2 = (block_meta_data_t *)Block_2;

    if (freeBlock_1->data_block_size > freeBlock_2->data_block_size)
        return -1;
    else if (freeBlock_2->data_block_size > freeBlock_1->data_block_size)
        return 1;
    else
        return 0;
}

void mm_add_free_block_meta_data_to_free_block_list(page_family_t *family, block_meta_data_t *free_meta_block)
{
    assert(free_meta_block->is_free);
    glthread_priority_insert(&family->free_block_pq_list_head, &free_meta_block->free_block_pq_list_glue, free_blocks_comparison_function, OFFSET_OF(block_meta_data_t, free_block_pq_list_glue));
}

vm_page_t *mm_add_new_page_to_family(page_family_t *pg_family)
{
    vm_page_t *newPage = allocate_vm_page(pg_family);
    if (newPage == NULL)
    {
        return NULL;
    }
    mm_add_free_block_meta_data_to_free_block_list(pg_family, &newPage->meta_block);
    return newPage;
}

vm_bool_t mm_split_free_data_block_for_allocation(page_family_t *pg_family, block_meta_data_t *freeMetaBlock, __uint32_t requested_size)
{

    // task-1 -> guards. block must be free, and big enough to serve the request.
    assert(freeMetaBlock->is_free);
    if (freeMetaBlock->data_block_size < requested_size)
        return MM_FALSE;

    // computed before data_block_size is overwritten below, else the original size is lost.
    __uint32_t remainingSizeAfterAllocation = freeMetaBlock->data_block_size - requested_size;
    block_meta_data_t *newMetaBlockAfterSplit = NULL;

    // task-2 -> updating our current allocated metaBlock. applies to all 3 cases.
    // offset is deliberately left alone - the block has not moved within the page.
    freeMetaBlock->is_free = MM_FALSE;
    freeMetaBlock->data_block_size = requested_size;
    remove_glthread(&freeMetaBlock->free_block_pq_list_glue);

    // task-3 -> the two cases that need no further work.
    // case 1 : no split. block consumed exactly, nothing left over.
    if (remainingSizeAfterAllocation == 0)
    {
        return MM_TRUE;
    }
    // case 2 : hard internal fragmentation. leftover cannot hold a meta block plus
    // one whole struct, so it could never serve a future request from this family.
    // absorbed into the allocated block instead of being tracked. no new block, so
    // no linkage or priority-queue changes are needed.
    else if (remainingSizeAfterAllocation < (sizeof(block_meta_data_t) + pg_family->struct_size))
    {
        return MM_TRUE;
    }

    // case 3 : full split. the residual is guaranteed to hold at least one whole
    // struct, because anything smaller was already caught by the hard-IF check above.
    // note: the course's looser threshold (remaining < sizeof(block_meta_data_t)) also
    // admits "soft internal fragmentation" here - a residual block with 0 usable bytes.
    // the threshold used above rules that out entirely, so only full splits reach here.

    // task-4 -> creating new meta block and filling its fields.
    // relies on data_block_size already being requested_size, so the macro lands
    // just past the allocated data.
    newMetaBlockAfterSplit = NEXT_META_BLOCK_BY_SIZE(freeMetaBlock);
    newMetaBlockAfterSplit->is_free = MM_TRUE;
    newMetaBlockAfterSplit->data_block_size = remainingSizeAfterAllocation - sizeof(block_meta_data_t);
    newMetaBlockAfterSplit->offset = freeMetaBlock->offset + sizeof(block_meta_data_t) + requested_size;

    // task-5 -> publishing the new block. stitch it into the P/N chain, then make it
    // visible to the allocator by inserting it into the family's free block list.
    init_glthread(&newMetaBlockAfterSplit->free_block_pq_list_glue);
    MM_BIND_BLOCKS_FOR_ALLOCATION(freeMetaBlock, newMetaBlockAfterSplit);
    mm_add_free_block_meta_data_to_free_block_list(pg_family, newMetaBlockAfterSplit);
    return MM_TRUE;
}

block_meta_data_t *mm_allocate_free_data_block(page_family_t *pg_family, __uint32_t requested_size)
{
    vm_bool_t status = MM_FALSE;
    vm_page_t *new_page = NULL;

    block_meta_data_t *biggestFreeBlock = mm_get_biggest_free_block_page_family(pg_family);
    if (biggestFreeBlock == NULL || biggestFreeBlock->data_block_size < requested_size)
    {
        // New Page
        new_page = mm_add_new_page_to_family(pg_family);
        if (!new_page)
            return NULL;

        status = mm_split_free_data_block_for_allocation(pg_family, &new_page->meta_block, requested_size);
        if (status)
        {
            return &new_page->meta_block;
        }
        return NULL;
    }
    status = mm_split_free_data_block_for_allocation(pg_family, biggestFreeBlock, requested_size);
    if (status)
        return biggestFreeBlock;

    return NULL;
}

void *xcalloc(char *struct_name, int units)
{
    page_family_t *pg_family = lookup_page_family_by_name(struct_name);
    if (pg_family == NULL)
    {
        printf("Error : Structure %s not registered with Memory Manager, so I can't allocate memory.\n", struct_name);
        return NULL;
    }
    __uint64_t requested_size = (__uint64_t)units * pg_family->struct_size;

    if (requested_size > mm_max_page_allocatable_memory(1))
    {
        printf("Error : Memory requested exceeds page size\n");
        return NULL;
    }

    block_meta_data_t *allocatedMetaBlock =
        mm_allocate_free_data_block(pg_family, (__uint32_t)requested_size);

    if (allocatedMetaBlock)
    {
        memset((char *)(allocatedMetaBlock + 1), 0, allocatedMetaBlock->data_block_size);
        return (void *)(allocatedMetaBlock + 1);
    }

    // could not allocate, so allocatedMetaBlock = NULL
    return NULL;
}

/* --- printing / diagnostics (S9) --------------------------------------- */

// dumps every meta block of a single page, in physical order.
void mm_print_vm_page_details(vm_page_t *vm_page)
{
    printf("\t next = %p, prev = %p\n", (void *)vm_page->next, (void *)vm_page->prev);
    printf("\t page family = %s\n", vm_page->pg_family->struct_name);

    __uint32_t j = 0;
    ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page, curr)
    {
        printf("\t\t%-14p Block %-3u %s  data_block_size = %-6u  offset = %-6u  prev = %-14p  next = %p\n",
               (void *)curr,
               j++,
               curr->is_free ? "F R E E D" : "ALLOCATED",
               curr->data_block_size,
               curr->offset,
               (void *)curr->previous_block,
               (void *)curr->next_block);
    }
    ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page, curr);
}

// per family : every page, and every block within it.
// pass NULL to dump all families.
void mm_print_memory_usage(char *struct_name)
{
    __uint32_t page_count = 0;
    vm_page_t *vm_page_curr = NULL;
    page_family_t *curr_family = NULL;
    vm_page_for_families_t *traversing_page = first_vm_page_for_families;

    printf("\nPage Size = %zu Bytes\n", SYSTEM_PAGE_SIZE);

    while (traversing_page)
    {
        ITERATE_PAGE_FAMILIES_BEGIN(traversing_page, curr_family)
        {
            if (struct_name &&
                strncmp(struct_name, curr_family->struct_name, MM_MAX_STRUCTNAME_SIZE) != 0)
            {
                continue;
            }

            printf("\nvm_page_family : %s, struct size = %u\n",
                   curr_family->struct_name, curr_family->struct_size);

            ITERATE_VM_PAGE_BEGIN(curr_family, vm_page_curr)
            {
                page_count++;
                printf("\n\tPage Index : %u , address = %p\n", page_count - 1, (void *)vm_page_curr);
                mm_print_vm_page_details(vm_page_curr);
            }
            ITERATE_VM_PAGE_END(curr_family, vm_page_curr);
        }
        ITERATE_PAGE_FAMILIES_END(traversing_page, curr_family);
        traversing_page = traversing_page->next;
    }

    printf("\n# Of VM Pages in Use : %u (%zu Bytes)\n",
           page_count, page_count * SYSTEM_PAGE_SIZE);
}

// per family summary : total / free / occupied block counts, plus app memory usage.
// this is the Q6 audit that used to live inside ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN.
void mm_print_block_usage()
{
    vm_page_t *vm_page_curr = NULL;
    page_family_t *curr_family = NULL;
    vm_page_for_families_t *traversing_page = first_vm_page_for_families;

    printf("\n");
    while (traversing_page)
    {
        ITERATE_PAGE_FAMILIES_BEGIN(traversing_page, curr_family)
        {
            __uint32_t total_block_count = 0;
            __uint32_t free_block_count = 0;
            __uint32_t occupied_block_count = 0;
            __uint32_t application_memory_usage = 0;

            ITERATE_VM_PAGE_BEGIN(curr_family, vm_page_curr)
            {
                ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page_curr, curr)
                {
                    total_block_count++;

                    /* sanity : an allocated block must be out of the PQ,
                     * a free block must be in it. */
                    if (curr->is_free == MM_FALSE)
                    {
                        assert(IS_GLTHREAD_LIST_EMPTY(&curr->free_block_pq_list_glue));
                        occupied_block_count++;
                        application_memory_usage +=
                            curr->data_block_size + sizeof(block_meta_data_t);
                    }
                    else
                    {
                        assert(!IS_GLTHREAD_LIST_EMPTY(&curr->free_block_pq_list_glue));
                        free_block_count++;
                    }
                }
                ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page_curr, curr);
            }
            ITERATE_VM_PAGE_END(curr_family, vm_page_curr);

            printf("%-20s   TBC : %-4u    FBC : %-4u    OBC : %-4u    AppMemUsage : %u\n",
                   curr_family->struct_name, total_block_count,
                   free_block_count, occupied_block_count, application_memory_usage);
        }
        ITERATE_PAGE_FAMILIES_END(traversing_page, curr_family);
        traversing_page = traversing_page->next;
    }
}

static int getHardInternalFragmentedMemSize(block_meta_data_t *first, block_meta_data_t *second)
{
    // return ((unsigned long)second - (unsigned long)first - first->data_block_size - sizeof(block_meta_data_t));
    block_meta_data_t *nextBlockBySize = NEXT_META_BLOCK_BY_SIZE(first);
    return (int)((unsigned long)second - (unsigned long)nextBlockBySize);
}

block_meta_data_t* freeBlock(block_meta_data_t *metaBlockToBeFreed)
{

    block_meta_data_t *blockTobeReturned = NULL;
    assert(metaBlockToBeFreed->is_free == MM_FALSE);

    vm_page_t *hostingPage = MM_GET_PAGE_FROM_META_BLOCK(metaBlockToBeFreed);
    page_family_t *pgFamily = hostingPage->pg_family;

    metaBlockToBeFreed->is_free = MM_TRUE;
    blockTobeReturned = metaBlockToBeFreed;
    block_meta_data_t *nextMetaBlock = NEXT_META_BLOCK(metaBlockToBeFreed);

    if (nextMetaBlock == NULL) // [last meta block on the page]
    {
        metaBlockToBeFreed->data_block_size += getHardInternalFragmentedMemSize(metaBlockToBeFreed, (block_meta_data_t *)((char *)hostingPage + SYSTEM_PAGE_SIZE));
    }
    else
    {
        metaBlockToBeFreed->data_block_size += getHardInternalFragmentedMemSize(metaBlockToBeFreed, nextMetaBlock);
    }

    // now we handled the cases of fragmentation -- so now we just have to merge our empty meta-block with the blocks above or below
    // 1] if merging possible with block above.
    if(nextMetaBlock && nextMetaBlock->is_free == MM_TRUE){
        mm_union_free_blocks(metaBlockToBeFreed, nextMetaBlock);
        blockTobeReturned = metaBlockToBeFreed;
    }

    // 2] if merging possible with block below.
    block_meta_data_t* prevBlock = PREV_META_BLOCK(metaBlockToBeFreed);
    if(prevBlock && prevBlock->is_free == MM_TRUE){
        mm_union_free_blocks(prevBlock, metaBlockToBeFreed);
        blockTobeReturned = prevBlock;
    }

    // now at this stage it is possible that virtual data memory page is empty.
    if(mm_is_vm_page_empty(hostingPage)){
        mm_vm_page_delete_and_free(hostingPage);
        return NULL;
    }

    mm_add_free_block_meta_data_to_free_block_list(pgFamily, blockTobeReturned);
    return blockTobeReturned;
}

void xfree(void *dataAddress)
{
    block_meta_data_t *linkedMetaBlock = (block_meta_data_t *)((char *)dataAddress - sizeof(block_meta_data_t));
    assert(linkedMetaBlock->is_free == MM_FALSE); // Error if already free(MM_TRUE)

    freeBlock(linkedMetaBlock); // main function -> does the major work of handling HARD-INTERNAL_FRAGMENTATION
}


// int main()
// {
//     mm_initializer();
//     printf("The size of the page on my system : %zu\n", SYSTEM_PAGE_SIZE);
//     void *p1Add = get_new_vm_page_from_kernel(1);
//     void *p2Add = get_new_vm_page_from_kernel(1);
//     printf("Address of Page_1 : %p\nAddress of Page_2 : %p\n", p1Add, p2Add);
//     // printf("%d -------\n", (p1Add - p2Add));
//     // if((p1Add - p2Add)%SYSTEM_PAGE_SIZE == 0){
//     //     printf("Yes\n");
//     // }
//     return_vm_page_to_kernel(p1Add, 1);
//     return_vm_page_to_kernel(p2Add, 1);
//     return 0;
// }