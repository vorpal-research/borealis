#include "defines.h"
#include <stdio.h>

typedef struct {
	int dev;
	void* obj;
	int status;
} necla_dev_t;

#define dev_t necla_dev_t

// @ensures \is_valid_ptr(\result)
// @ensures \result->dev == dev
dev_t* dev_alloc(int dev) {
	dev_t* x = (dev_t*)malloc(sizeof(dev_t));
    ASSUME(x != 0);
	x->dev    = dev;
	x->obj    = NULL;
	x->status = 0;
	return x;
}

// @requires \is_valid_ptr(dev)
void dev_free(dev_t* dev) {
	switch(dev->dev) {
	case 0:
		dev_free((dev_t*)dev->obj);
		ASSERT(((dev_t*)dev->obj)->status == 2);
		ASSERT(dev->status == 1);
		break;
	case 1:
		free(dev->obj);
		ASSERT(dev->status > 0);
		break;
	default:
		ASSERT(dev->status == -1);
		break;
	}
	free(dev);
}
						 

int main(int devNr, int proc) {
	dev_t* dev = dev_alloc(devNr);
	switch(devNr) {
	case 0:
		dev->obj = (void*)dev_alloc(proc);
		dev->status = 1;
		break;
	case 1:
		dev->obj = (int*)malloc(proc * sizeof(int));
		ASSUME(dev->obj);
		dev->status = proc;
		break;
	default:
		dev->status = -1;
		break;
	}

	switch(dev->dev) {
	case 0:
		((dev_t*)dev->obj)->status = 2;
		break;
	case 1:
		{
			int* x = (int*)dev->obj;
			int i; 
			for(i = 0; i < dev->status; ++i) {
				x[i] = i * 335;
			}
		}
	  break;
	default:
		ASSERT(dev->obj == NULL);
		break;
	}

	dev_free(dev);
	return 0;	
}
