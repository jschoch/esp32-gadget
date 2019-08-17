#include <stdint.h>

#ifdef __cplusplus  
extern "C" { 
#endif 

#define OUTPUT_TASK_STACK_SIZE 4096

void output_task(void *pvParameter);
void set_output_value(int new_value);

#ifdef __cplusplus 
} 
#endif
