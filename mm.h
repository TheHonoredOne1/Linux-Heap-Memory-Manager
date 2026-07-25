#define MM_MAX_STRUCT_NAME 32

typedef struct page_family_
{
    char struct_name[MM_MAX_STRUCT_NAME];
    __UINT32_TYPE__ struct_size;

} page_family_t;

typedef struct vm_page_for_families_
{
    struct vm_page_for_families_ *next;
    page_family_t page_family[0];

} vm_page_for_families_t;

#define MAX_FAMILIES_PER_VM_PAGE \
    (SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / sizeof(page_family_t)


     
#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)                 \
{                                                                                   \
    __uint32_t _count = 0;                                                            \
    for (curr = (page_family_t *)&(vm_page_for_families_ptr->page_family[0]); \
     _count < MAX_FAMILIES_PER_VM_PAGE && (curr->struct_size);                  \
     _count++, curr++)                                                          \
    {


#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) \
    }                                                             \
}