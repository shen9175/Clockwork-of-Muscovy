#ifndef PQASTAR_H
#define PQASTAR_H

typedef struct
{
	int x;
	int y;
	double cost;
	double f;
}cell;

typedef struct node
{
	cell info;
	struct node* next;
}* nodeptr;


typedef struct qnode
{
	nodeptr lptr;//this is the information the queue stores
	struct qnode* next;
}* qptr;

class Queue
{
	private:
		qptr header;
		int number;
		std::vector<nodeptr> pqdata;
		void minheapify(size_t i);
		size_t left(size_t i) { return 2 * i + 1; }
		size_t right(size_t i) { return 2 * i + 2; }
		size_t parent(size_t i) { return (i - 1) / 2; }//0 base index's left,right and parent is different from 1 base index
	public:
		Queue();
		~Queue();
		void EnQueue(nodeptr n);
		nodeptr DeQueue();
		bool IsEmpty();
};

#endif

