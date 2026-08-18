#include <stdint.h>
#include <stdio.h>
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

	emp_t* e1 = XCALLOC(1, emp_t);
	emp_t* e2 =  XCALLOC(1, emp_t);
	emp_t* e3 = XCALLOC(1, emp_t);

	student_t* s1 = XCALLOC(1, student_t);
	student_t* s2 = XCALLOC(1, student_t);



	mm_print_memory_usage("emp_t");
	mm_print_memory_usage("student_t");
	mm_print_block_usage();
	scanf("\n");// any key from user.

    return 0;
}