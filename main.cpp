#include <cstdio>
#include <cstdlib>
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>
#include <atomic>


typedef struct LockFreeStack
{
	int data;
	std::atomic<struct LockFreeStack*> next;
} LFStack;

/*void init (LFStack* head)
{
	head = nullptr;
}*/

LFStack* create_stack ()
{
	auto* newStack = new LFStack;
	
	newStack->data = 0;
	newStack->next = nullptr;
	
	return newStack;
}

LFStack* push (LFStack* head, int data)
{
	auto* tmp = new LFStack;            //@@@@
	if (!tmp) exit(0);                  //@@@@
	
	LFStack* top = head->next.load ();
	
	tmp->data = data;
	tmp->next.store (top);
	//tmp->next = head;
	//head = tmp;
	while (!head->next.compare_exchange_weak (top, tmp)) //ВИДИМО ЗДЕСЬ НАДО НЕ head->next, А head.compare....
	{
		tmp->next.store(top);
	}
	
	free (tmp);
	return head;
}

LFStack* pop (LFStack *head, int *element)
{
	typename cds::gc::HP::Guard guard;
	
	LFStack* tmp = guard.protect (head->next); //head вместо head->next;
	LFStack* top = tmp->next.load ();
	
	*element = top->data;               //@@@@ по идее надо брать из tmp, косяки в push функции
	
	//head = top;
	while (!head->next.compare_exchange_weak(tmp, top)) //ВИДИМО ЗДЕСЬ НАДО НЕ head->next, А head.compare....
	{
		top = tmp->next.load();
	}
	
	guard.release (); //наверное, чтобы перестать защищать head
	return head;
}

void display (LFStack* head)
{
	LFStack *current;
	current = head;
	if (current)
	{
		printf("Stack: ");
		do
		{
			printf("%d ",current->data);
			current = current->next;
		}
		while (current!= nullptr);
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
	LFStack *s = create_stack ();
	
	for (i = 0; i < 3; i++)
	{
		s = push(s, i);
		display (s);
	}
	for (i = 0; i < 3; i++)
	{
		int returnData;
		s = pop (s, &returnData);
		display (s);
		printf("%d \n", returnData);
	}
	
}