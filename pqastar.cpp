#include <vector>
#include "pqastar.h"

Queue::Queue()
{
	header=nullptr;
	number=0;
}

Queue::~Queue()
{
	/*	2 parts memory leak:
	1. number will decrease while i increase, 
	i will easily greater than number but queue node have not delete completely
	2. Astar BFS will not empty the priority queue when it found the target.
	there are a lot of step node: nodeptr remain in the priority queue and no one will delete these nodeptr

	for(int i=0;i<number;i++) {
		DeQueue();
	}*/
	
	while (number > 0) {
		delete DeQueue();
	}

}

void Queue::EnQueue(nodeptr l)
{
	qptr p,n,prev;
	bool dupflag=false;
	bool addflag=false;
	n=new qnode;
	n->lptr=l;
	n->next=nullptr;

	if(header==nullptr)
	{
		header=n;
		number++;
	}
	else
	{
		p=header;
		prev=nullptr;
		while(p!=nullptr)
		{	if(p->lptr->info.x==l->info.x && p->lptr->info.y==l->info.y)
				{
				dupflag=true;
				if(p->lptr->info.f>l->info.f)
					{
					delete p->lptr;
					p->lptr=NULL;
					p->lptr=l;
					}
				delete n;
				break;
				}
			if(n->lptr->info.f<p->lptr->info.f)
			{
				if(p!=header)
				{
				n->next=p;
				prev->next=n;
				}
				else
				{
				header=n;
				n->next=p;
				}
				number++;
				addflag=true;
				break;
			}
			prev=p;
			p=p->next;
		}

		if(dupflag==false&&addflag==false)
		{
			prev->next=n;
			number++;
		}
	}


}

nodeptr Queue::DeQueue()
{
	nodeptr r;
	if(header==nullptr)
	{
		//MessageBox(NULL,"The Queue is empty and it cannot be popped!","Dequeue Error",MB_ICONERROR|MB_OK);
		r=nullptr;
		return r; 
	}
	else
	{
		qptr p=header;
		header=p->next;
		r=p->lptr;
		p->next=nullptr;
		delete p;
		p=nullptr;
		number--;
		return r;
	}
}

bool Queue::IsEmpty()
{
	if(number>0)
		return false;
	else
		return true;
}



/*
2015-9-23 update: try to use fancy algorithm to reduce 2 year's ago build "naive" linked list priority queue when I took artificial inteligence class
but the fact is 2 year's ago's build is the optimal one, how smart at the time I was! (at that time the only data structure that I am good at is linked list)

The initial think is the original linked list priority queue is traverse the whole thing for O(n) time
The array heap is O(lgn) to add.

But when I implement the code as follows, I found:

array heap cannot remove duplicate at the adding process in O(lgn) time, at least O(n + lgn) time to add a element, 
n is for traverse the whole array for duplicate.

when found duplicate, if you replace the new one, you have to miniheapify float down for O(lgn) time, so add a duplicate O(n + lgn), if it is not duplicate, you also have to traveral the whole thing
that is O(n) then float up for normal adding, it's also O(n + lgn)
all in all ->worse than only O(n) linked list priority queue implement, bad move

performance has significant negative influnce: FPS reduce 30fps from 390+ to 350+ from linklist costmized no-duplicate priority queue to array based heap no-duplicate priority queue
When tour guide go to common's(very long walk), there is a 0.5s stuck for calculate path that button display white blank.

If you do not remove duplicate, the calculate became computational impossible when the map and route is large and long: 
in tour guide to go common's part, the game becomes freeze just before tour guide depart for large calculating. no remove dupliates only O(lgn) to add does nothing

Queue::Queue()
{
}

Queue::~Queue()
{
	for (auto member : pqdata) {
	delete member;
	member = nullptr;
	}
}

bool Queue::IsEmpty()
{
	return pqdata.empty();
}

void Queue::minheapify(size_t i) {
size_t l = left(i);
size_t r = right(i);
size_t smaller;
if (l < pqdata.size() && pqdata[i]->info.f > pqdata[l]->info.f) {
smaller = l;
} else {
smaller = i;
}
if (r < pqdata.size() && pqdata[smaller]->info.f > pqdata[r]->info.f) {
smaller = r;
}
if (smaller != i) {
std::swap(pqdata[i], pqdata[smaller]);
minheapify(smaller);
}
}
void Queue::EnQueue(nodeptr l) {
pqdata.push_back(l);
size_t i = static_cast<int>(pqdata.size()) - 1;//pqdata.size() at least is 1, so no negative situation

for (int j = 0; j < i; ++j) {
if (pqdata[j]->info.x == pqdata[i]->info.x && pqdata[j]->info.y == pqdata[i]->info.y) {
pqdata[j] = pqdata[i];
pqdata[i] = pqdata.back();
pqdata.pop_back();
minheapify(i);
return;
}
}
while (i > 0 && pqdata[parent(i)]->info.f > pqdata[i]->info.f) {
size_t p = parent(i);
std::swap(pqdata[p], pqdata[i]);
i = p;
}
}
nodeptr Queue::DeQueue() {
if (pqdata.empty()) {
return nullptr;
}
nodeptr ret = pqdata[0];
pqdata[0] = pqdata.back();
pqdata.pop_back();
minheapify(0);
return ret;
}

*/