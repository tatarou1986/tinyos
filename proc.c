
/* b’è“I‚É */
static task_struct		rdy_head[2];

static int	task_sentinel = 0;
volatile int rdy_tail = 0;

static int pickup_process(task_struct **t){
	
	++task_sentinel;
	
	*t = &rdy_head[task_sentinel % 2];
	
	return 1;
	
}

int start_thread(void (*f)(void)){
	
	task_struct		*task;
	
	task = &rdy_head[rdy_tail];
	
	task->eip = f;
	task->esp = 
	
}

