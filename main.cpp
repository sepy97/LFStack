#include <stdio.h>
#include <stdlib.h>
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>
#include <atomic>


typedef struct concurrentStack
{
	int data;
	struct concurrentStack* next;
} cStack;


void init (cStack* head)
{
	head = NULL;
}
cStack* create_stack ()
{ //initializes the stack
	
	cStack* newStack = (cStack*)malloc(sizeof(cStack));
	
	newStack->data = 0;
	newStack->next = NULL;
	
	return newStack;
}


cStack* push (cStack* head, int data)
{
	cStack* tmp = (cStack*)malloc(sizeof(cStack));
	if(tmp == NULL)
	{
		exit(0);
	}
	tmp->data = data;
	tmp->next = head;
	head = tmp;
	return head;
}

cStack* pop (cStack *head, int *element)
{
	cStack* tmp = head;
	*element = head->data;
	head = head->next;
	free(tmp);
	return head;
}

void display (cStack* head)
{
	cStack *current;
	current = head;
	if(current!= NULL)
	{
		printf("Stack: ");
		do
		{
			printf("%d ",current->data);
			current = current->next;
		}
		while (current!= NULL);
		printf("\n");
	}
	else
	{
		printf("The Stack is empty\n");
	}
	
}

int myThreadEntryPoint(void *)
{
	// Attach the thread to libcds infrastructure
	cds::threading::Manager::attachThread();
	// Now you can use HP-based containers in the thread
	//...
	// Detach thread when terminating
	cds::threading::Manager::detachThread();
}

int main ()
{
	
	// Инициализируем libcds
	cds::Initialize() ;
	
	{
		// Инициализируем Hazard Pointer синглтон
		cds::gc::HP hpGC ;
		
		// Если main thread использует lock-free контейнеры
		// main thread должен быть подключен
		// к инфраструктуре libcds
		cds::threading::Manager::attachThread() ;
		
		// Всё, libcds готова к использованию
		// Далее располагается ваш код
		//...
	}
	
	// Завершаем libcds
	cds::Terminate() ;
	
	
	int i;
	cStack *s = create_stack ();
	
	for (i = 0; i < 300; i++)
	{
		s = push(s, i);
		//display (s);
	}
	for (i = 0; i < 300; i++)
	{
		int returnData;
		s = pop (s, &returnData);
		printf("%d ", returnData);
	}
	
}