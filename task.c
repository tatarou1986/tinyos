
#include <task.h>
#include <idt.h>

static void (*task_func)()	= NULL;
static int volatile task_flag = 0;

int in_task_que(void (*f)())
{	
	cli();
	
	if(task_flag){
		sti();
		return 0;
	}
	
	task_func = f;
	task_flag = 1;
	
	sti();
	
	return 1;
}


int exist_task(void)
{
	return task_flag;
}


void delete_task(void)
{
	
	cli();
	
	task_flag = 0;
	task_func = NULL;
	
	sti();
	
}


int exec_task(void)
{
	
	cli();
	
	if (task_flag == 0){
		sti();
		return;
	}
	
	task_flag = 0;
	sti();
	task_func();
	
	return 1;
	
}



