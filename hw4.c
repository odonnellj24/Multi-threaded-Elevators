#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

//how many ptrs into the heap we have
#define INDEX_SIZE 1000
void* heapindex[INDEX_SIZE];


//grabbing the address and size of the global memory region from proc 
void init_global_range(){
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;

  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    if (strstr(line, "hw4")!=NULL){
      ++counter;
      if (counter==3){
        sscanf(line, "%lx-%lx", &start, &end);
        global_mem.start=(size_t*)start;
        // with a regular address space, our globals spill over into the heap
        global_mem.end=malloc(256);
        free(global_mem.end);
      }
    }
    else if (read_bytes > 0 && counter==3) {
      if(strstr(line,"heap")==NULL) {
        // with a randomized address space, our globals spill over into an unnamed segment directly following the globals
        sscanf(line, "%lx-%lx", &start, &end);
        printf("found an extra segment, ending at %zx\n",end);						
        global_mem.end=(size_t*)end;
      }
      break;
    }
  }
  fclose(mapfile);
}


//marking related operations

int is_marked(size_t* chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(size_t* chunk) {
  (*chunk)|=0x2;
}

void clear_mark(size_t* chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((size_t*)c))& ~(size_t)3 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    printf("Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < sbrk(0))
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}
int in_use(void *c) { 
  return (next_chunk(c) && ((*(size_t*)next_chunk(c)) & 1));
}


// index related operations

#define IND_INTERVAL ((sbrk(0) - (void*)(heap_mem.start - 1)) / INDEX_SIZE)
void build_heap_index()
{
  // TODO
  
}

// the actual collection code
void sweep()
{
  
  size_t* currChunk = heap_mem.start - 1;
  //size_t* endChunk = heap_mem.end;
  size_t* nextChunk = NULL;
  while( (currChunk < ((size_t*)sbrk(0))) && (currChunk != NULL) )
  {
        nextChunk = next_chunk(currChunk);
  	if(is_marked(currChunk))
  	{
  		clear_mark(currChunk);
	}
	else if (in_use(currChunk)){
		free(currChunk + 1);
	}
	currChunk = nextChunk;
        
        heap_mem.end = sbrk(0);
  }
  
  
}

//determine if what "looks" like a pointer actually points to a block in the heap
size_t * is_pointer(size_t * ptr)
{
	// TODO
	
	//helper function to help with marking 
	//give a number, it should either return 0
		//if the number is not an address on the heap
	//or return the pointer to the chunk that holds the address
	//that the pointer passed in as a parameter points to
	
	//size_t* ptr currChunk = heap_mem.start;
	size_t* startChunk = heap_mem.start;
	//size_t* endChunk = ((size_t*)sbrk(0));
	size_t* currChunk = heap_mem.start - 1;
        size_t* nextChunk = NULL;
	if ( ptr == NULL){
		return NULL;
	}
	else if ( ptr < startChunk){
		return NULL;
	}
	else if ( ptr >=  (size_t*)sbrk(0)){
		return NULL;
	}
	while( currChunk < (size_t*)sbrk(0))
        {
  	nextChunk = next_chunk(currChunk);
  	
  	if ( (ptr <= nextChunk) && (ptr >= currChunk+1) ){
        
  	return currChunk;
    
        }
  	
	
        currChunk = nextChunk;
        } 
  return NULL;
	
}

void walk_region_and_mark(void* start, void* end)
{
	// TODO
	
	//this function will go through the data and stack sections,
  	//finding pointers and marking the blocks they point to
  
  size_t* curr = ((size_t*)start);
 // size_t* endr = ((size_t*)end);

   
  while( curr < ((size_t*)end))
  {
  	size_t* ptr = is_pointer((size_t*)*curr);
  	if ( ptr == NULL) {
  		curr++;
  	        continue;
	  }
  	else if (is_marked(ptr)){
  		curr++;
	        continue;
  	  }
	else{
  	mark(ptr);
	//setMarkBit(currChunk);
  	
  	walk_region_and_mark( ptr + 1 , next_chunk( ptr ) - 2);
  	curr++;
        }
  }
  
}

// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  heap_mem.start=malloc(512);
  //since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  heap_mem.end=sbrk(0);
  //grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;

  // build the index that makes determining valid ptrs easier
  // implementing this smart function for collecting garbage can get bonus;
  // if you can't figure it out, just comment out this function.
  // walk_region_and_mark and sweep are enough for this project.
  build_heap_index();

  //walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep(stack_mem.start, heap_mem.end);
}

