#include <cstdio>
#include <cstdlib>
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>
#include <atomic>
#include <thread>

#define MAX_THREAD_NUM 100
#define TEST_VOL 100

//std::atomic<int> toCheck;
std::atomic<int> poppedArray [MAX_THREAD_NUM*TEST_VOL];

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
		
		while (!head.compare_exchange_strong (newStack->next, newStack))
			; // the body of the loop is empty
	}
	
	LockFreeStack* pop ()
	{
		if (head == nullptr)
		{
			printf ("Stack if EMPTY!!! You can not pop from it!'n");
			return 0;
		}
		try
		{
			cds::gc::HP::Guard guard  = cds::gc::HP::Guard() ;
			
			LockFreeStack* newStack = guard.protect (head); //head вместо head->next;
			//LFStack* top = tmp->next.load ();
			
			LockFreeStack* poppedNode = new LockFreeStack (newStack->data);
			//*element = top->data;               //@@@@ по идее надо брать из tmp, косяки в push функции
			
			//head = top;
			while (!head.compare_exchange_strong (newStack, newStack->next)) //ВИДИМО ЗДЕСЬ НАДО НЕ head->next, А head.compare....
			{
				poppedNode->data = newStack->data;
			}
			
			guard.release (); //наверное, чтобы перестать защищать head
			return poppedNode;        //@@@@
		}
		catch (cds::gc::HP::not_enought_hazard_ptr_exception)
		{
			printf ("...!!!\n");
		}
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
			if (current.next == nullptr) break;
			current = *current.next;
		}
		printf ("\n");
	}
	
	bool isEmpty()
	{
		return (this->head == NULL);
		
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

void testPush (LFStack* toTest, int checkData)
{
	if (!cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::attachThread ();
	}
	
	//FastRandom* ran = new FastRandom (rand());
	
	for (int i = 0; i < TEST_VOL; i++)
	{
		//toTest = push (toTest, ran->rand()%MAX_THREAD_NUM);
		toTest->push (checkData+i);//ran->rand()%MAX_THREAD_NUM);
	}
	
	if (cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::detachThread ();
	}
}

void testPop (LFStack* toTest)
{
	if (!cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::attachThread ();
	}
	
	int value;
	for (int i = 0; i < TEST_VOL; i++)
	{
		LockFreeStack* resultNode = toTest->pop ();
		//int result = resultNode->data;
		int checkData = resultNode->data;
		//if (checkData == 0) printf ("HERE IS ZERO!!!!\n");
		poppedArray [checkData].store (1);// = true;
		//if (toCheck != resultNode->data) printf ("Test FAILED!!! Popped data is %d instead of %d\n", resultNode->data, toCheck.load());
	}
	
	if (cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::detachThread ();
	}
}

void testStack (LFStack* toTest)
{
	std::thread thr[MAX_THREAD_NUM];
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i] = std::thread (testPush, toTest, i*TEST_VOL);
		//myThreadEntryPoint (thr[i]);
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i].join ();
	}
	
	/*toTest->display ();
	toTest->display ();
	toTest->display ();*/
	
	for (int i = 0; i < MAX_THREAD_NUM*TEST_VOL; i++)
	{
		poppedArray[i].store (0);// = false;
	}
	
	
	/*toTest->display ();
	toTest->display ();*/
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i] = std::thread (testPop, toTest);
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i].join ();
	}
	
	bool failure = false;
	for (int i = 0; i < MAX_THREAD_NUM*TEST_VOL; i++)
	{
		if (!poppedArray[i].load())
		{
			printf ("Test FAILED!!! Popped data %d was not actually popped!\n", i);
			failure = true;
		}
		
	}
	
	if (!failure) printf ("Test passed successfully!\n");
	
}

int main ()
{
	
	//cds::gc::hp::GarbageCollector::Construct( stack_type::c_nHazardPtrCount, 1, 16 );
	
	// Инициализируем libcds
	cds::Initialize() ;
	
	{
		// Инициализируем Hazard Pointer синглтон
		cds::gc::HP hpGC (TEST_VOL*MAX_THREAD_NUM, MAX_THREAD_NUM); //num of hpointers
		//cds::gc::hp ::GarbageCollector::construct (1, 16 );//hpGC ;    @@@@@@
		
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