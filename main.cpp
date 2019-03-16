#include <cstdio>
#include <cstdlib>
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>
#include <atomic>
#include <thread>

#define MAX_THREAD_NUM 100
#define TEST_VOL 100000

class FastRandom {
private:
	unsigned long long rnd;
public:
	FastRandom(unsigned long long seed) {
		rnd = seed;
	}
	unsigned long long rand() {
		rnd ^= rnd << 21;
		rnd ^= rnd >> 35;
		rnd ^= rnd << 4;
		return rnd;
	}
};

/*struct LockFreeStack
{
	int data;
	std::atomic<struct LockFreeStack*> next;
};*/

struct LockFreeStack
{
	int data;
	LockFreeStack* next;
	LockFreeStack(const int& data) : data(data), next(nullptr) {}
};

class LFStack
{
	std::atomic<LockFreeStack*> head;
	
public:
	void push (const int& data)
	{
		LockFreeStack* newStack = new LockFreeStack (data);
		newStack->next = head.load ();
		
		while (!head.compare_exchange_weak (newStack->next, newStack))
			; // the body of the loop is empty
	}
	
	LockFreeStack* pop ()
	{
		typename cds::gc::HP::Guard guard;
		
		LockFreeStack* newStack = guard.protect (head); //head вместо head->next;
		//LFStack* top = tmp->next.load ();
		
		LockFreeStack* poppedNode = new LockFreeStack (0);
		poppedNode->data = newStack->data;
		poppedNode->next = NULL;
		//*element = top->data;               //@@@@ по идее надо брать из tmp, косяки в push функции
		
		//head = top;
		while (!head.compare_exchange_weak (newStack, newStack->next)) //ВИДИМО ЗДЕСЬ НАДО НЕ head->next, А head.compare....
		{}
		
		guard.release (); //наверное, чтобы перестать защищать head
		return poppedNode;        //@@@@
	}
	
	void display ()
	{
		//нужна проверка this на empty!!!
		if (this->head == NULL)
		{
			printf ("Stack: EMPTY!\n");
			return;
		}
		LockFreeStack current = *this->head.load();
		printf ("Stack: ");
		while (true)
		{
			printf ("%d ", current.data);
			if (current.next == NULL) break;
			current = *current.next;
		}
		printf ("\n");
	}
};

/*LFStack* create_stack ()
{
	auto* newStack = new LFStack;
	
	newStack->data = 0;
	newStack->next = nullptr;
	
	return newStack;
}*/

/*LFStack* push (LFStack* head, int data)
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
}*/

/*LFStack* pop (LFStack *head, int *element)
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
	return head;        //@@@@
}*/

int myThreadEntryPoint (void *)
{
	// Attach the thread to libcds infrastructure
	cds::threading::Manager::attachThread();
	// Now you can use HP-based containers in the thread
	//...
	// Detach thread when terminating
	cds::threading::Manager::detachThread();
}

bool empty();//

/*void display (LFStack* head)
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
	
}*/

void testPush (LFStack* toTest)
{
	FastRandom* ran = new FastRandom (rand());
	
	for (int i = 0; i < TEST_VOL; i++)
	{
		//toTest = push (toTest, ran->rand()%MAX_THREAD_NUM);
		toTest->push (ran->rand()%MAX_THREAD_NUM);
	}
}

void testPop (LFStack* toTest)
{
	int value;
	for (int i = 0; i < TEST_VOL; i++)
	{
		toTest->pop ();// = pop (toTest, &value);
	}
}

void testStack (LFStack* toTest)
{
	std::thread thr[MAX_THREAD_NUM];
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i] = std::thread (testPush, toTest);
		//myThreadEntryPoint (thr[i]);
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i].join ();
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i] = std::thread (testPop, toTest);
	}
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
	
		int i;
		//LFStack *s = create_stack ();
		LFStack s;
		
		testStack (&s);
		/*for (i = 0; i < 3; i++)
		{
			s.push (i);// = push(s, i);
			s.display ();//display (s);
		}
		for (i = 0; i < 3; i++)
		{
			LockFreeStack* returnData = s.pop ();
			s.display ();//display (s);
			printf("%d \n", returnData->data);
		}*/
		
		
		
	}
	
	// Завершаем libcds
	cds::Terminate() ;
	
	
}