#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "userApi_mm.h"

typedef struct emp_{
	char name[32];
	uint32_t emp_id;
}emp_t;

typedef struct student_{
	char name[32];
	uint32_t rollno;
	uint32_t marks_phy;
	uint32_t marks_chem;
	uint32_t marks_math;
	struct student_* next;
}student_t;

// typedef struct manager_{
// 	char name[32];
// 	uint32_t emp_id;
// 	uint32_t team_size;
// }manager_t;

// typedef struct department_{
// 	char name[32];
// 	uint32_t dept_id;
// 	uint32_t head_emp_id;
// }department_t;

// typedef struct course_{
// 	char title[32];
// 	uint32_t course_id;
// 	uint32_t credits;
// 	struct course_* next;
// }course_t;

// typedef struct project_{
// 	char name[32];
// 	uint32_t project_id;
// 	uint32_t budget;
// }project_t;

// typedef struct client_{
// 	char name[32];
// 	uint32_t client_id;
// 	uint32_t contract_value;
// 	struct client_* next;
// }client_t;

int main()
{
    mm_initializer();
	// testApp first initialises.

    MM_REG_STRUCT(emp_t);
    MM_REG_STRUCT(student_t);
    // MM_REG_STRUCT(manager_t);
    // MM_REG_STRUCT(department_t);
    // MM_REG_STRUCT(course_t);
    // MM_REG_STRUCT(project_t);
    // MM_REG_STRUCT(client_t);
    mm_print_registered_page_families();// check if our project runs properly till now.

#if 0
	/* scenario 1 : allocate a mix of both families, then free them one at a
	 * time, dumping the layout after every free. good for watching individual
	 * merges happen. */
	emp_t* e1 = XCALLOC(1, emp_t);
	emp_t* e2 = XCALLOC(1, emp_t);
	emp_t* e3 = XCALLOC(1, emp_t);
	emp_t* e4 = XCALLOC(3, emp_t);

	student_t* s1 = XCALLOC(1, student_t);
	student_t* s2 = XCALLOC(2, student_t);
	student_t* s3 = XCALLOC(1, student_t);

	mm_print_memory_usage(NULL);
	XFREE(e1);  mm_print_memory_usage(NULL);
	XFREE(e2);  mm_print_memory_usage(NULL);
	XFREE(e3);  mm_print_memory_usage(NULL);
	XFREE(e4);  mm_print_memory_usage(NULL);
	XFREE(s1);  mm_print_memory_usage(NULL);
	XFREE(s2);  mm_print_memory_usage(NULL);
	XFREE(s3);  mm_print_memory_usage(NULL);
#endif

#if 1
	/* scenario 2 : 120 students chained into a linked list. at 56B struct +
	 * 48B meta = 104B per block, one page holds ~38, so this spans several
	 * pages. then walk the chain and free every node, which should merge
	 * everything back and hand whole pages back to the kernel. */
	int i = 0;
	student_t* stud = NULL;
	student_t* prev = NULL;
	student_t* first = NULL;

	for(i = 0; i < 120; i++){
		stud = XCALLOC(1, student_t);
		assert(stud);
		if(i == 0)
			first = stud;
		if(prev)
			prev->next = stud;
		prev = stud;
	}

	printf("\n============ AFTER 120 ALLOCATIONS ============\n");
	mm_print_memory_usage(NULL);
	mm_print_block_usage();

	student_t* next = NULL;
	for( ; first; first = next){
		next = first->next;
		XFREE(first);
	}

	printf("\n============ AFTER FREEING ALL ============\n");
	mm_print_memory_usage(NULL);
	mm_print_block_usage();
#endif

    return 0;
}
