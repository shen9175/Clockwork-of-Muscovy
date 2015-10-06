#include <vector>
#include <cmath>
#include <string>
#include "pqastar.h"
#include "astar.h"

using namespace std;



double h1(cell info,int GoalX, int GoalY)
{
	double x,y;
	x=info.x-GoalX;
	y=info.y-GoalY;

	double r=info.cost+sqrt(x*x+y*y);
	return r;
}

nodeptr reverse(nodeptr l)
{
	//update with new high efficiency reverse linked list algorithm with no extra memory space needed O(1) memory and O(n) time
	nodeptr prev = nullptr;
	nodeptr p = l;
	while (p != nullptr) {
		nodeptr temp = p;
		p = p->next;
		temp->next = prev;
		prev = temp;
	}
	return prev;
}

nodeptr astar1_search(char** screen, int height, int width, int row,int col,int srow,int scol)
	//map, map-rows, map-cols,goal-row-y,goal-col-x,start-row-y,start-col-x
{
	Queue q;
	cell info;
	info.x=scol;
	info.y=srow;
	info.cost=0;
	nodeptr path=new node;
	path->info=info;
	path->next=nullptr;
	q.EnQueue(path);
	char** map=new char*[height];

	for(int i=0;i<height;i++)
	{
		map[i]=new char[width];
	}

	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			map[i][j]=screen[i][j];
		}
	}


	if(map[row][col]=='w' || map[row][col]=='#'|| map[row][col]=='b'|| map[row][col]=='o')
	{
		for(int i=0;i<height;i++)
		{
			delete []map[i];
		}
			delete []map;
		return nullptr;
	}
	else
		map[row][col]='G';
	while(1)
	{

		if(q.IsEmpty())
		{
				for(int i=0;i<height;i++)
				{
				delete []map[i];
				}
				delete []map;
			return nullptr;
		}
		nodeptr f=q.DeQueue();
		cell frontier=f->info;

		if(map[f->info.y][f->info.x]=='G')
		{
			for(int i=0;i<height;i++)
				{
				delete []map[i];
				}
				delete []map;
				return reverse(f);
			
		}

		map[frontier.y][frontier.x]='V';

		if(frontier.y-1>=0)  //top
		{
			if(map[frontier.y-1][frontier.x]!='V' && map[frontier.y-1][frontier.x]!='w'&& map[frontier.y-1][frontier.x]!='#'&& map[frontier.y-1][frontier.x]!='o'&& map[frontier.y-1][frontier.x]!='b')
			{
			cell info;
			info.x=frontier.x;
			info.y=frontier.y-1;
			info.cost=frontier.cost+1;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}
		if(frontier.y-1>=0&&frontier.x-1>=0)  //top-left
		{
			if(map[frontier.y-1][frontier.x-1]!='V' && map[frontier.y-1][frontier.x-1]!='w'&& map[frontier.y-1][frontier.x-1]!='#'&& map[frontier.y-1][frontier.x-1]!='o'&& map[frontier.y-1][frontier.x-1]!='b')
			{
			cell info;
			info.x=frontier.x-1;
			info.y=frontier.y-1;
			info.cost=frontier.cost+1.414;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}
		if(frontier.x-1>=0) //left
		{
			if(map[frontier.y][frontier.x-1]!='V' && map[frontier.y][frontier.x-1]!='w'&& map[frontier.y][frontier.x-1]!='#'&& map[frontier.y][frontier.x-1]!='o'&& map[frontier.y][frontier.x-1]!='b')
			{
			cell info;
			info.x=frontier.x-1;
			info.y=frontier.y;
			info.cost=frontier.cost+1;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}

		if(frontier.x-1>=0&&frontier.y+1<height) //left-bottom
		{
			if(map[frontier.y+1][frontier.x-1]!='V' && map[frontier.y+1][frontier.x-1]!='w'&& map[frontier.y+1][frontier.x-1]!='#'&& map[frontier.y+1][frontier.x-1]!='o'&& map[frontier.y+1][frontier.x-1]!='b')
			{
			cell info;
			info.x=frontier.x-1;
			info.y=frontier.y+1;
			info.cost=frontier.cost+1.414;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}

		if(frontier.y+1<height) //down
		{
			if(map[frontier.y+1][frontier.x]!='V' && map[frontier.y+1][frontier.x]!='w'&& map[frontier.y+1][frontier.x]!='#'&& map[frontier.y+1][frontier.x]!='o'&& map[frontier.y+1][frontier.x]!='b')
			{
			cell info;
			info.x=frontier.x;
			info.y=frontier.y+1;
			info.cost=frontier.cost+1;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}
		if(frontier.y+1<height&&frontier.x+1<width) //bottom-right
		{
			if(map[frontier.y+1][frontier.x+1]!='V' && map[frontier.y+1][frontier.x+1]!='w'&& map[frontier.y+1][frontier.x+1]!='#'&& map[frontier.y+1][frontier.x+1]!='o'&& map[frontier.y+1][frontier.x+1]!='b')
			{
			cell info;
			info.x=frontier.x+1;
			info.y=frontier.y+1;
			info.cost=frontier.cost+1.414;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}
		if(frontier.x+1<width) //right
		{
			if(map[frontier.y][frontier.x+1]!='V' && map[frontier.y][frontier.x+1]!='w'&& map[frontier.y][frontier.x+1]!='#'&& map[frontier.y][frontier.x+1]!='o'&& map[frontier.y][frontier.x+1]!='b')
			{
			cell info;
			info.x=frontier.x+1;
			info.y=frontier.y;
			info.cost=frontier.cost+1;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}
		if(frontier.x+1<width&&frontier.y-1>=0) //right-top
		{
			if(map[frontier.y-1][frontier.x+1]!='V' && map[frontier.y-1][frontier.x+1]!='w'&& map[frontier.y-1][frontier.x+1]!='#'&& map[frontier.y-1][frontier.x+1]!='o'&& map[frontier.y-1][frontier.x+1]!='b')
			{
			cell info;
			info.x=frontier.x+1;
			info.y=frontier.y-1;
			info.cost=frontier.cost+1.414;

			nodeptr n=new node;
			n->info=info;
			n->next=f;
			n->info.f=h1(info,col,row);
			q.EnQueue(n);
			}
		}	


	}
}