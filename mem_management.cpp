#include <unistd.h>
#include <iostream>
#include <iomanip>

/**
 * MEM_BUFFER determines size of RAM
 * METADATA_SIZE is the fixed size of metadata block
 * ALIGNMENT_FACTOR determines the smallest chunk of memory in bytes.
 * MAGIC_NUMBER is used to check if the pointer to be freed is valid.
 */
#define MEM_BUFFER (1024*1024)
#define METADATA_SIZE (sizeof(meta_data))
#define ALIGNMENT_FACTOR 4
#define MAGIC_NUMBER 0123

using namespace std;

/**
 * This structure contains the metadata.
 * Size determines the size of data excuding the size of metadata
 * next block is the pointer to next slot of memory in heap.
 */
typedef struct meta_data{
	unsigned int size;
	unsigned int available;
	meta_data* next_block;
	unsigned int magic_number;
} meta_data;

/**
 * Determines the start of heap.
 */ 
void* heap_start=NULL;

/**
 * keeps track of the last block used when extending heap.
 */
void* heap_last_block=NULL;

/**
 * Adjusts the requested size so that the allocated space is always a multiple of alighment factor
 */ 
unsigned int align_size(unsigned int size){
	return (size % ALIGNMENT_FACTOR) ? size+ALIGNMENT_FACTOR-(size%ALIGNMENT_FACTOR) : size;
}

/**
 * Goes through the whole heap to find an empty slot.
 */ 
meta_data* find_slot(unsigned int size){
	meta_data* iter = (meta_data*)heap_start;
	while(iter){
		if (iter->available && iter->size>=size){
			iter->available=false;
			return iter;
		}
		iter=iter->next_block;
	}
	return NULL;
}

/**
 * If a free slot can accommodate atleast 1 more (METADATA_SIZE+ALIGNMENT FACTOR)
 * apart from the requested size, then the slot is divided to save space.
 */ 
void divide_slot(void* slot,unsigned int  size){
	meta_data* slot_to_divide = (meta_data*)slot;
	meta_data* new_slot=(meta_data*)((char*)slot_to_divide+METADATA_SIZE+size);
	
	new_slot->size=slot_to_divide->size - size - METADATA_SIZE;
	new_slot->available=true;
	new_slot->next_block=slot_to_divide->next_block;
	new_slot->magic_number=MAGIC_NUMBER;
	
	slot_to_divide->size=size;
	slot_to_divide->next_block=new_slot;
}

/**
 * Extends the heap using sbrk syscall. 
 */
void* extend(unsigned int  size){
	meta_data* new_block=(meta_data*)sbrk(0);
	if ((char*)new_block-(char*)heap_start>MEM_BUFFER) return NULL;
	int* flag = (int*)sbrk(size+METADATA_SIZE);
	if (*flag==-1) return NULL;
	new_block->size=size;
	new_block->available=false;
	new_block->next_block=NULL;
	new_block->magic_number=MAGIC_NUMBER;
	
	if (heap_last_block){
		((meta_data*)heap_last_block)->next_block=new_block;	
	}
	heap_last_block=new_block;
	return new_block;
}

/**
 * Returns the metadata from heap corresponding to a data pointer.
 */ 
meta_data* get_metadata(void* ptr){
	
	return (meta_data*)((char*)ptr - METADATA_SIZE);
}

/**
* Search for big enough free space on heap.
* Split the free space slot if it is too big, else space will be wasted.
* Return the pointer to this slot.
* If no adequately large free slot is available, extend the heap and return the pointer.
*/
void* xyzmalloc(unsigned int  size){
	size = align_size(size);
	void* slot;
	if (heap_start){
		cout << "Heap starts at: " << heap_start << endl;
		slot = find_slot(size);
		if (slot){
			if(((meta_data*)slot)->size > size + METADATA_SIZE){
				divide_slot(slot,size);
			}
		}else{
			slot = extend(size);
		}
	}else{
		heap_start=sbrk(0);
		cout << "Heap starts at: " << heap_start << endl;
		slot = extend(size);
	}
	
	if (!slot){return slot;}
	
	cout << "Memory assigned from " << slot << " to 0x" <<hex<< long((char*)slot + METADATA_SIZE +((meta_data*)slot)->size) << endl;
	cout << "Memory ends at: " << sbrk(0) << endl;
	cout << "Size of heap so far: " << (char*)sbrk(0) - (char*)heap_start << endl;

	return ((char*)slot)+METADATA_SIZE;
}

/**
 * Frees the allocated memory. If first checks if the pointer falls
 * between the allocated heap range. It also checks if the pointer
 * to be deleted is actually allocated. this is done by using the
 * magic number. Due to lack of time i haven't worked on fragmentation.
 */ 
void xyzfree(void* ptr){

	if (!heap_start) return;
	if (ptr >= heap_start && ptr < sbrk(0)){
		meta_data* ptr_metadata = get_metadata(ptr);
		if (ptr_metadata->magic_number==MAGIC_NUMBER){
			ptr_metadata->available=true;
			cout << "Memory freed at: "<< ptr_metadata << endl;
		}
	}
}

/**
 * trivial case: allocates 2 integers and then frees them.
 */
void test_case_1(){
	cout << "TC1\n";
	heap_start=NULL;
	int* x = (int*)xyzmalloc(sizeof(int));
	cout << endl;
	int* y = (int*)xyzmalloc(sizeof(int));
	cout << endl;
	xyzfree(x);
	cout << endl;
	xyzfree(y);
	cout << endl;
}

/**
 * Allocates, frees, allocates and then frees again.
 */ 
void test_case_2(){
	cout << "TC2\n";
	heap_start=NULL;
	int* x = (int*)xyzmalloc(sizeof(int));
	cout << endl;
	xyzfree(x);
	cout << endl;
	int* y = (int*)xyzmalloc(sizeof(int));
	cout << endl;
	xyzfree(y);
	cout << endl;
	
}

/**
 * allocates a long, frees it. and then allocates some chars
 * to see if the already freed space is utilised.
 */
void test_case_3(){
	cout << "TC3\n";
	heap_start=NULL;
	long* x = (long*)xyzmalloc(sizeof(long));
	cout << endl;
	xyzfree(x);
	cout << endl;
	char* c1 = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	char* c2 = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	char* c3 = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	char* c4 = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	char* c5 = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	xyzfree(c1);
	cout << endl;
	xyzfree(c2);
	cout << endl;
	xyzfree(c3);
	cout << endl;
	xyzfree(c4);
	cout << endl;
	xyzfree(c5);
	cout << endl;
}

/**
 * allocates once and frees thrice.
 */
void test_case_4(){
	cout << "TC4\n";
	heap_start=NULL;
	long* x = (long*)xyzmalloc(sizeof(long));
	cout << endl;
	xyzfree(x);
	cout << endl;
	xyzfree(x);
	cout << endl;
	xyzfree(x);
	cout << endl;
}

/**
 * Allocate 3 spaces, free the middle one and allocate 4th space to 
 * see if the middle empty slot is utilized.
 */
void test_case_5(){
	cout << "TC5\n";
	heap_start=NULL;
	long* l1 = (long*)xyzmalloc(sizeof(long));
	cout << endl;
	long* l2 = (long*)xyzmalloc(sizeof(long));
	cout << endl;
	long* l3 = (long*)xyzmalloc(sizeof(long));
	cout << endl;
	xyzfree(l2);
	cout << endl;
	char* c = (char*)xyzmalloc(sizeof(char));
	cout << endl;
	xyzfree(c);
	cout << endl;
}

/**
 * Tests the limits of memory by creating many objects.
 */
void test_case_6(){
	cout << "TC6\n";
	heap_start=NULL;
	for (int i=0;i<MEM_BUFFER;i++){
		if (!xyzmalloc(sizeof(long))){
			cout << "Ran out of memory!" << endl;
			break;
		}
		cout <<endl;
	}
}

int main(){
	test_case_1();
	test_case_2();
	test_case_3();
	test_case_4();
	test_case_5();
	//test_case_6();
	return 0;
}
