#define MM_MAX_STRUCTNAME_SIZE 32
typedef struct vm_page_for_families_ vm_page_for_families_t;
typedef struct page_family_
{
    char struct_name[MM_MAX_STRUCTNAME_SIZE];
    __UINT32_TYPE__ struct_size;

} page_family_t;

typedef struct vm_page_for_families_
{
    vm_page_for_families_t *next;
    page_family_t page_family[0];

} vm_page_for_families_t;

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

typedef enum {
    MM_FALSE,
    MM_TRUE
} vm_bool_t;

typedef struct block_meta_data_  block_meta_data_t;
struct block_meta_data_{
    vm_bool_t is_free;
    __uint32_t data_block_size;
    __uint32_t offset;
    block_meta_data_t* previous_block;
    block_meta_data_t* next_block;
};

typedef struct vm_page_ vm_page_t;
struct vm_page_{
    vm_page_t * next;
    vm_page_t * prev;
};

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
    