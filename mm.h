#define MM_MAX_STRUCTNAME_SIZE 32

// --- forward declares ---
typedef struct vm_page_for_families_ vm_page_for_families_t;
typedef struct vm_page_ vm_page_t;

// --- plain structs (no dependency on the above) ---
typedef enum {
    MM_FALSE,
    MM_TRUE
} vm_bool_t;

typedef struct block_meta_data_  block_meta_data_t;
struct block_meta_data_{
    vm_bool_t is_free;
    __uint32_t data_block_size;
    __uint32_t offset; // distance from the page start
    block_meta_data_t* previous_block;
    block_meta_data_t* next_block;
};

// --- structs that reference/embed the above ---
typedef struct page_family_
{
    char struct_name[MM_MAX_STRUCTNAME_SIZE];
    __UINT32_TYPE__ struct_size;
    vm_page_t * first_page; // to point to the first data-page, of the chain corresponding to this family.
} page_family_t;

struct vm_page_for_families_
{
    vm_page_for_families_t *next;
    page_family_t page_family[0];
};

struct vm_page_{
    vm_page_t * next;
    vm_page_t * prev;
    page_family_t * pg_family;
    block_meta_data_t meta_block;
    char page_memory[0];
};


#define MAX_FAMILIES_PER_VM_PAGE \
    ((SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(page_family_t))


#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr_family_iterator)             \
{                                                                               \
    __uint32_t _count = 0;                                                      \
    for (curr_family_iterator = (page_family_t *)&(vm_page_for_families_ptr->page_family[0]);   \
     _count < MAX_FAMILIES_PER_VM_PAGE && (curr_family_iterator->struct_size);                  \
     _count++, curr_family_iterator++)                                                          \
    {
/*
void* start_address = &(vm_page_for_families_ptr -> page_family[0]);
    for (curr_family_iterator = (page_family_t *)start_address;
     _count < MAX_FAMILIES_PER_VM_PAGE && (curr_family_iterator->struct_size);
     _count++, curr_family_iterator++)
    {
*/
#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr_family_iterator) }}


page_family_t* lookup_page_family_by_name(char * family_name);

#define OFFSET_OF(structure_type, field_name)\
    ((size_t)&(((structure_type*)0)->field_name))

#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)\
    ((void*)((char*)block_meta_data_ptr - block_meta_data_ptr->offset))

#define NEXT_META_BLOCK(block_meta_data_ptr)\
    (block_meta_data_ptr->next_block)

#define PREV_META_BLOCK(block_meta_data_ptr)\
    (block_meta_data_ptr->previous_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)\
    ((void*)((char*)block_meta_data_ptr + (sizeof(block_meta_data_t) + (block_meta_data_ptr->data_block_size))))



// block splitting and merging
#define MM_BIND_BLOCKS_FOR_ALLOCATION(allocated_meta_block, free_meta_block) \
    do {                                                                     \
        NEXT_META_BLOCK(free_meta_block) = NEXT_META_BLOCK(allocated_meta_block); \
        if(NEXT_META_BLOCK(allocated_meta_block) != NULL) {                  \
            PREV_META_BLOCK(NEXT_META_BLOCK(allocated_meta_block)) = free_meta_block; \
        }                                                                     \
        NEXT_META_BLOCK(allocated_meta_block) = free_meta_block;             \
        PREV_META_BLOCK(free_meta_block) = allocated_meta_block;             \
    } while(0)

void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second);



// TEMP - Q6 counters, move into calling function later
block_meta_data_t *largest_free_block   = NULL;
block_meta_data_t *largest_alloc_block  = NULL;
__uint32_t free_block_count  = 0;
__uint32_t alloc_block_count = 0;
vm_bool_t prev_block_was_free = MM_FALSE;

// iterate vm_pages.
#define ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(first_meta_block, curr){ \
    block_meta_data_t* curr = first_meta_block;\
    for( ; curr != NULL ; curr = NEXT_META_BLOCK(curr)) {\
        if(curr->is_free){                      \
            if(prev_block_was_free){            \
                assert(0);                      \
            }                                   \
            free_block_count++;                 \
            if(largest_free_block == NULL){     \
                largest_free_block = curr;      \
            }                                   \
            else{                               \
                if(curr->data_block_size > largest_free_block->data_block_size) \
                {                               \
                    largest_free_block = curr;  \
                }                               \
            }                                   \
            prev_block_was_free = MM_TRUE;      \
        }                                       \
        else{                                   \
            alloc_block_count++;                \
            if(largest_alloc_block == NULL){    \
                largest_alloc_block = curr;     \
            }                                   \
            else{                               \
                if(curr->data_block_size > largest_alloc_block->data_block_size) \
                {                               \
                    largest_alloc_block = curr; \
                }                               \
            }                                   \
            prev_block_was_free = MM_FALSE;     \
        }                                       \

#define ITERATE_VM_PAGE_ALL_BLOCKS_END(first_meta_block, curr) }\
    }\


vm_bool_t mm_is_vm_page_empty(vm_page_t * vm_page);


// it is given that the vm_page is empty, and has no data block present.
#define MARK_VM_PAGE_EMPTY(vm_page_ptr) \
    do {  \
    (vm_page_ptr -> meta_block.is_free = MM_TRUE);\
    (vm_page_ptr -> meta_block.next_block = NULL);\
    (vm_page_ptr -> meta_block.previous_block = NULL);\
    }while(0)\

