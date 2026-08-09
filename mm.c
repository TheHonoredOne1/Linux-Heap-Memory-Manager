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
    new_allocated_vm_page->pg_family = page_family;
    MARK_VM_PAGE_EMPTY(new_allocated_vm_page);
    new_allocated_vm_page->next = page_family->first_page;
    if(page_family->first_page != NULL)
        page_family->first_page->prev = new_allocated_vm_page;

    page_family->first_page = new_allocated_vm_page;
    new_allocated_vm_page->prev = NULL;

    new_allocated_vm_page->meta_block.offset = OFFSET_OF(vm_page_t, meta_block);
    new_allocated_vm_page->meta_block.data_block_size = mm_max_page_allocatable_memory(1);

    return new_allocated_vm_page;
}

void mm_vm_page_delete_and_free(vm_page_t* vm_page)
{
    page_family_t* family = vm_page->pg_family;
    //  deletion of the head.
    if(vm_page->prev == NULL){
        family->first_page = vm_page->next;
        if(vm_page->next != NULL)
            vm_page->next->prev = NULL;
        vm_page->next = NULL;
        vm_page->pg_family = NULL;
    }
    else
    {
        if(vm_page->next == NULL)
        {
            vm_page->prev->next = NULL;
            vm_page->prev = NULL;
        }
        else
        {
            vm_page->prev->next = vm_page->next;
            vm_page->next->prev = vm_page->prev;
            vm_page ->next = NULL;
            vm_page ->prev = NULL;
        }
    }
    return_vm_page_to_kernel(vm_page, 1);
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