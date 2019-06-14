#include <cstdio>
#include <cstdlib>
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <cds/gc/hp.h>
#include <atomic>
#include <thread>
#include <x86intrin.h>

#define INIT_PUSH 10000
#define MAX_THREAD_NUM 10
#define MAX_VOLUME 10000000

//std::atomic<int> toCheck;
std::atomic<int> poppedArray [MAX_THREAD_NUM*MAX_VOLUME];

class FastRandom {
private:
	unsigned long long rnd;
public:
	FastRandom(unsigned long long seed) { //time + threadnum
		rnd = seed;
	}
	unsigned long long rand() {
		rnd ^= rnd << 21;
		rnd ^= rnd >> 35;
		rnd ^= rnd << 4;
		return rnd;
	}
};

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
			printf ("Stack if EMPTY!!! You can not pop from it! \n");
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

/*
void testPush (LFStack* toTest, int checkData)
{
	if (!cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::attachThread ();
	}
	
	//FastRandom* ran = new FastRandom (rand());
	
	for (int i = 0; i < MAX_VOLUME; i++)
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
	for (int i = 0; i < MAX_VOLUME; i++)
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
		thr[i] = std::thread (testPush, toTest, i*MAX_VOLUME);
		//myThreadEntryPoint (thr[i]);
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i].join ();
	}
	
	
	for (int i = 0; i < MAX_THREAD_NUM*MAX_VOLUME; i++)
	{
		poppedArray[i].store (0);// = false;
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i] = std::thread (testPop, toTest);
	}
	
	for (int i = 0; i < MAX_THREAD_NUM; i++)
	{
		thr[i].join ();
	}
	
	bool failure = false;
	for (int i = 0; i < MAX_THREAD_NUM*MAX_VOLUME; i++)
	{
		if (!poppedArray[i].load())
		{
			printf ("Test FAILED!!! Popped data %d was not actually popped!\n", i);
			failure = true;
		}
		
	}
	
	if (!failure) printf ("Test passed successfully!\n");
}
 */

void testStack (LFStack* toTest, const int volume, int threadNum)
{
	if (!cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::attachThread ();
	}
	FastRandom* ran = new FastRandom (time(NULL) + threadNum);
	for (int i = 0; i < volume; i++)
	{
		int pushOrPop = i%2;//rand()%2;
		if (pushOrPop)
		{
			toTest->push (ran->rand()%volume);
		}
		else
		{
			toTest->pop ();
		}
	}
	
	if (cds::threading::Manager::isThreadAttached ())      //@@@@
	{
		cds::threading::Manager::detachThread ();
	}
}

int main (int argc, char** argv)
{
	int maxThreads = 0;
	
	if (argc > 1)
	{
		maxThreads = atoi(argv[1]);
	}
	else
	{
		maxThreads = 1;
		//printf ("no arguments :( \n");
		//return 0;
	}
	
	//cds::gc::hp::GarbageCollector::Construct( stack_type::c_nHazardPtrCount, 1, 16 );
	
	// Инициализируем libcds
	cds::Initialize() ;
	
	{
		// Инициализируем Hazard Pointer синглтон
		cds::gc::HP hpGC (INIT_PUSH + MAX_VOLUME, maxThreads); //num of hpointers
		//cds::gc::hp ::GarbageCollector::construct (1, 16 );//hpGC ;    @@@@@@
		
		// Если main thread использует lock-free контейнеры
		// main thread должен быть подключен
		// к инфраструктуре libcds
		cds::threading::Manager::attachThread() ;
		
		// Всё, libcds готова к использованию
		// Далее располагается ваш код
		//...
	
		LFStack s;
		
		std::thread thr[maxThreads];
		
		FastRandom* ran = new FastRandom (__rdtsc ()/1000000000);
		
		for (int i = 0; i < INIT_PUSH; i++)
		{
			s.push (ran->rand() % INIT_PUSH);
		}
		
		uint64_t tick = __rdtsc ()/100000;
		for (int i = 0; i < maxThreads; i++)
		{
			thr[i] = std::thread (testStack, &s, MAX_VOLUME/maxThreads, i);
		}
		
		for (int i = 0; i < maxThreads; i++)
		{
			thr[i].join ();
		}
		
		uint64_t tick2 = __rdtsc ()/100000;
		printf ("%d, %llu, \n", maxThreads, tick2 - tick);
		
	
	}
	
	// Завершаем libcds
	cds::Terminate() ;
	
	
}