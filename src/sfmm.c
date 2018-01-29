/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define WORD 2
#define DMEMORY_ROW 16

sf_free_header* remove_from_list(free_list *list);
sf_free_header* remove_element_from_list(sf_free_header *header, free_list *list);
void insert_to_list(sf_free_header *head, free_list *list);
size_t ajudst_size(size_t size);
/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX}, //0032, 0128
    {NULL, LIST_2_MIN, LIST_2_MAX}, //0129, 0512
    {NULL, LIST_3_MIN, LIST_3_MAX}, //0513, 2048
    {NULL, LIST_4_MIN, LIST_4_MAX}  //2049, -1
};

int sf_errno = 0;

int sbrk_counter = 0;

sf_free_header *starting_ptr = NULL;

sf_free_header *ending_ptr = NULL;

int space_avaiable = 0;

void *sf_malloc(size_t size)
{

size_t adjust_size; /*Adjusted block size*/
    int block_size;
    int new_block_size;
    int starting_array = 0;
    int current_array = 0;
    int found = 0;
    sf_free_header *removed_block = NULL;

    /*size is invalid (0 or greater than 4 pages)*/
    if(size <= 0 || size > (PAGE_SZ * 4)){
        sf_errno = EINVAL;
        return NULL;
    }

    /*Adjust size*/
    adjust_size = ajudst_size(size);

    /*Define block size*/
    block_size = adjust_size + 16;

    /*Check if there is 1 page exist*/
    if(get_heap_start() == get_heap_end()){

        /*Allocate 1st page*/
        sf_sbrk();

        /*Record the staring & ending*/
        starting_ptr = (void *)get_heap_start();
        ending_ptr = (void *)get_heap_end();
        space_avaiable = get_heap_end() - get_heap_start();

        /*Initialize the header*/
        sf_free_header *starting = (void *)get_heap_start();
        sf_header head;
        head.unused = 0;
        head.two_zeroes = 0;
        head.padded = 0;
        head.allocated = 0;
        (head).block_size = (space_avaiable >> 4);

        /*Initialize the footer*/
        sf_footer *ending = get_heap_end() - 8;
        (*ending).requested_size = 0;
        (*ending).block_size = (space_avaiable >> 4);
        (*ending).two_zeroes = 0;
        (*ending).padded = 0;
        (*ending).allocated = 0;

        /*Insert the new block to the free list*/
        (*starting).header = head;
        (*starting).next = NULL;
        (*starting).prev = NULL;

        insert_to_list(starting, &seg_free_list[3]);
        sbrk_counter++;
    }

    /*Check if there is space for the block*/
    while(!found){

        /*Determing which array to start from*/
        for(int i = 0; 0 < 4; i++){
            if(block_size >= seg_free_list[i].min && block_size <= seg_free_list[i].max){
                starting_array = i;
                break;
            }
        }

        /*Search in the free list for space*/
        for(int i = 0; i < 4; i++){
            if(seg_free_list[i].head != NULL)
            {
                //Have elements in the seg list
                if( (*seg_free_list[i].head).next != NULL ){

                    //Iterator = head
                    sf_free_header *iterator = seg_free_list[i].head;

                    if(((*iterator).header.block_size << 4) >=  block_size){
                        removed_block = remove_element_from_list(iterator, &seg_free_list[i]);
                        found = 1;
                        break;
                    }
                    while((*iterator).next != NULL){
                        iterator = (*iterator).next;
                        if(((*iterator).header.block_size << 4) >=  block_size){
                            removed_block = remove_element_from_list(iterator, &seg_free_list[i]);
                            found = 1;
                            break;
                        }
                    }//end while
                }
                else{
                    sf_free_header *iterator = seg_free_list[i].head;
                    if(((*iterator).header.block_size << 4) <  block_size){
                        found = 0;
                        current_array = i;
                    }
                    else{
                        removed_block = remove_from_list(&seg_free_list[i]);
                        found = 1;
                        current_array = i;
                        break;
                    }
                }
            }
            if(found)
                break;
        }
        /*Reach to the largest free list and yet not found space avaiable*/
        if(!found)
        {
            /*Can't allocate more space*/
            if (sbrk_counter >= 4){
                sf_errno = ENOMEM;
                return NULL;
            }
            else{
                sf_sbrk();
                sbrk_counter++;

                /*Accessing the footer of previous page*/
                sf_footer *previous_footer = ((void *)get_heap_end()) - PAGE_SZ - 8;
                if((*previous_footer).allocated == 1 && sbrk_counter > 1){
                    /*Previous page have no extra space*/
                    space_avaiable = PAGE_SZ;
                    sf_free_header *starting = (void *)get_heap_end() - PAGE_SZ;
                    sf_header head;
                    head.unused = 0;
                    head.two_zeroes = 0;
                    head.padded = 0;
                    head.allocated = 0;
                    (head).block_size = (space_avaiable >> 4);

                    /*Initialize the footer*/
                    sf_footer *ending = get_heap_end() - 8;
                    (*ending).requested_size = 0;
                    (*ending).block_size = (space_avaiable >> 4);
                    (*ending).two_zeroes = 0;
                    (*ending).padded = 0;
                    (*ending).allocated = 0;

                    /*Insert the new block to the free list*/
                    (*starting).next = NULL;
                    (*starting).prev = NULL;
                    (*starting).header = head;
                    insert_to_list(starting, &seg_free_list[3]);
                }
                else{
                    /*Coalesce with previous page*/
                    space_avaiable = PAGE_SZ + ((*previous_footer).block_size << 4);
                    remove_from_list(&seg_free_list[current_array]);

                    sf_free_header *starting = (void *)get_heap_end() - space_avaiable;
                    sf_header head;
                    head.unused = 0;
                    head.two_zeroes = 0;
                    head.padded = 0;
                    head.allocated = 0;
                    (head).block_size = (space_avaiable >> 4);

                    /*Initialize the footer*/
                    sf_footer *ending = get_heap_end() - 8;
                    (*ending).requested_size = 0;
                    (*ending).block_size = (head).block_size;
                    (*ending).two_zeroes = 0;
                    (*ending).padded = 0;
                    (*ending).allocated = 0;

                    /*Insert the new block to the free list*/
                    (*starting).next = NULL;
                    (*starting).prev = NULL;
                    (*starting).header = head;
                    insert_to_list(starting, &seg_free_list[3]);
                }
            }
        }
    }//end while

    /*Pull out the free block from approate list*/
    sf_free_header *free_block_from_list = removed_block;
    sf_free_header *new_allocated_header = free_block_from_list;

    /*Constructing head*/
    sf_header allocated_head;
    allocated_head.unused = 0;
    allocated_head.block_size = (block_size >> 4);
    allocated_head.two_zeroes = 0;
    allocated_head.allocated = 1;
    if(block_size-16 > size)
        allocated_head.padded = 1;
    else
        allocated_head.padded = 0;

    /*Check if the new free block will be splinter*/
    if(((*free_block_from_list).header.block_size << 4) - block_size < 32){
        if(((*free_block_from_list).header.block_size << 4) - block_size != 0){
            /*Merge the rest to avoid spinter*/
            block_size = block_size + (((*free_block_from_list).header.block_size << 4) - block_size);
            allocated_head.block_size = (block_size >> 4);
            allocated_head.padded = 1;
        }
    }

    if(((*free_block_from_list).header.block_size << 4) - block_size != 0){
        sf_free_header *new_free_block = (void *)free_block_from_list + block_size;
        sf_header new_free_head;

        new_free_head.unused = 0;
        new_block_size = ((*free_block_from_list).header.block_size << 4) - block_size;
        new_free_head.block_size = (new_block_size >> 4);
        new_free_head.two_zeroes = 0;
        new_free_head.padded = 0;
        new_free_head.allocated = 0;
        (*new_free_block).header = new_free_head;

        /*Update footer*/
        sf_footer *new_footer = (void *)new_free_block + new_block_size - 8;
        (*new_footer).block_size = new_block_size >> 4;

        /*Insert the new free block in to proper list*/
        for(int i = 0; 0 < 4; i++){
            if(new_block_size >= seg_free_list[i].min && new_block_size <= seg_free_list[i].max){
                starting_array = i;
                break;
            }
        }
        insert_to_list(new_free_block, &seg_free_list[starting_array]);
    }
    (*new_allocated_header).header = allocated_head;

    /*Constructing footer*/
    sf_footer *allocated_footer = (void *)new_allocated_header + block_size - 8;
    (*allocated_footer).requested_size = size;
    (*allocated_footer).block_size = block_size >> 4;
    (*allocated_footer).two_zeroes = 0;
    (*allocated_footer).padded = (*new_allocated_header).header.padded;
    (*allocated_footer).allocated = 1;
    //return get_heap_start();
	return (void *)new_allocated_header + 8;
}


void *sf_realloc(void *ptr, size_t size) {
    int adjust_size = 0;
    int block_size = 0;

    //Null pointer
    if(ptr == NULL){
        abort();
    }
    //size = 0, free the block
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }

    /*Access to header & footer*/
    sf_free_header *header = (void *)ptr;
    sf_footer *footer = (void *)ptr;
    header = (void *)header - 8;
    footer = (void *)header + ((*header).header.block_size << 4) - 8;

    /*Invalid cases*/
    if(ptr < get_heap_start() || ptr > get_heap_end()){
        abort();
    }
    //Pointer is not allocated
    else if( (*header).header.allocated != 1 || (*footer).allocated != 1){
        abort();
    }
    //allocated bit in header & footer doesnt match
    else if( (*header).header.allocated != (*footer).allocated){
        abort();
    }
    //padded bit in header & footer doesnt match
    else if ((*header).header.padded != (*footer).padded){
        abort();
    }
    //block_size in header & footer doesnt match
    else if((*header).header.block_size != (*footer).block_size){
        abort();
    }
    else if( (*footer).requested_size + 16 != ((*footer).block_size << 4)&& (*footer).padded == 0){
        abort();
    }
    //invalid size
    else if((int)size < 0){
        abort();
    }


    adjust_size = ajudst_size(size);

    block_size = adjust_size + 16;
    //Reallocating to a Larger Size
    if(block_size > ((*header).header.block_size << 4)){
        //Malloc new space
        void* new_block = sf_malloc(size);

        //If there is error in malloc, return null
        if( new_block == NULL)
            return NULL;

        memcpy(new_block, ptr, (*header).header.block_size << 4);
        sf_free(ptr);
        return new_block;
    }
    //Reallocating to Smaller Size
    else if(size < (*header).header.block_size << 4){
        //There is splinter
        if(((*header).header.block_size << 4) - block_size < 32){
            //if(((*header).header.block_size << 4) - block_size != 0){
                /*Update header & footer*/
                block_size = block_size + (((*header).header.block_size << 4) - block_size);

                (*footer).requested_size = size;
                (*footer).block_size = block_size >> 4;
                (*footer).two_zeroes = 0;
                if((*footer).requested_size + 16 != (*footer).block_size << 4)
                    (*footer).padded = 1;
                else
                    (*footer).padded = 0;
                (*footer).allocated = 1;

                (*header).header.unused = 0;
                (*header).header.block_size = (*footer).block_size;
                (*header).header.two_zeroes = 0;
                (*header).header.padded = (*footer).padded;
                (*header).header.allocated = 1;
            //}
        }
        //No splinter
        else{

            sf_free_header *new_free_block = (void *)header + block_size;
            sf_footer *new_allocated_footer = (void *)new_free_block - 8;

            //header, new_allocated_footer, new_free_bloc, footer
            (*footer).requested_size -= block_size;
            (*footer).block_size = (((*header).header.block_size << 4) - block_size) >> 4;
            (*footer).two_zeroes = 0;
            if((*footer).requested_size + 16 != (*footer).block_size << 4)
                    (*footer).padded = 1;
                else
                    (*footer).padded = 0;
            (*footer).allocated = 1;

            (*new_allocated_footer).requested_size = size;
            (*new_allocated_footer).block_size = block_size >> 4;
            (*new_allocated_footer).two_zeroes = 0;
            (*new_allocated_footer).requested_size = size;
            if((*new_allocated_footer).requested_size + 16 != (*new_allocated_footer).block_size << 4)
                    (*new_allocated_footer).padded = 1;
                else
                    (*new_allocated_footer).padded = 0;
            (*new_allocated_footer).allocated = 1;

            (*new_free_block).header.unused = 0;
            (*new_free_block).header.block_size = (*footer).block_size;
            (*new_free_block).header.two_zeroes = 0;
            (*new_free_block).header.padded = 0;
            (*new_free_block).header.allocated = 1;

            (*header).header.unused = 0;
            (*header).header.block_size = (*new_allocated_footer).block_size;
            (*header).header.two_zeroes = 0;
            (*header).header.padded = (*new_allocated_footer).padded;
            (*header).header.allocated = 1;

            sf_free((void *)new_free_block + 8);
        }

    }
    //Size are the same
    else{
        (*footer).requested_size = size;
        if((*footer).requested_size + 16 != (*footer).block_size << 4)
            (*footer).padded = 1;
        else
            (*footer).padded = 0;
        return ptr;
    }

	return ptr;
}

void sf_free(void *ptr) {
    int block_size = 0;
    int array_index = 0;

    /*Invalid cases*/

    //Null pointer
    if(ptr == NULL){
        abort();
    }

    /*Access to header & footer*/
    sf_free_header *header = (void *)ptr;
    sf_footer *footer = (void *)ptr;
    header = (void *)header - 8;
    footer = (void *)header + ((*header).header.block_size << 4) - 8;

    /*Invalid cases*/
    //Pointer range out of bounds
    if(ptr < get_heap_start() || ptr > get_heap_end()){
        abort();
    }
    //Pointer is not allocated
    else if( (*header).header.allocated != 1 || (*footer).allocated != 1){
        abort();
    }
    //allocated bit in header & footer doesnt match
    else if( (*header).header.allocated != (*footer).allocated){
        abort();
    }
    //padded bit in header & footer doesnt match
    else if ((*header).header.padded != (*footer).padded){
        abort();
    }
    //block_size in header & footer doesnt match
    else if((*header).header.block_size != (*footer).block_size){
        abort();
    }
    else if( (*footer).requested_size + 16 != ((*footer).block_size << 4) && (*footer).padded == 0){
        abort();
    }
    else if((*footer).requested_size + 16 == ((*footer).block_size << 4) && (*footer).padded == 1){
        abort();
    }

    /*Check if the new block is free*/
    sf_free_header *next_block_header = (void *)header + ((*header).header.block_size << 4);

    if( (*next_block_header).header.allocated == 0 ){
        block_size = (*next_block_header).header.block_size << 4;
        for(int i = 0; 0 < 4; i++){
            if(block_size >= seg_free_list[i].min && block_size <= seg_free_list[i].max){
                array_index = i;
                break;
            }
        }
        if(seg_free_list[array_index].head != NULL)
            remove_element_from_list(next_block_header, &seg_free_list[array_index]);

        /*Coalescing*/
        sf_footer *new_footer = (void *)next_block_header + ((*next_block_header).header.block_size << 4) - 8;
        block_size = ((*header).header.block_size << 4) + ((*next_block_header).header.block_size << 4);

        /*Setup for new header & footer*/
        (*header).header.block_size = (block_size >> 4);

        (*header).header.allocated = 0;
        (*header).header.padded = 0;
        (*header).header.two_zeroes = 0;
        (*header).header.unused = 0;
        (*new_footer).allocated = 0;
        (*new_footer).padded = 0;
        (*new_footer).two_zeroes = 0;
        (*new_footer).requested_size = 0;
        (*new_footer).block_size = (block_size >> 4);


        /*Determing which array to insert*/
        block_size = (*header).header.block_size << 4;
        for(int i = 0; 0 < 4; i++){
            if(block_size >= seg_free_list[i].min && block_size <= seg_free_list[i].max){
                array_index = i;
                break;
            }
        }
        insert_to_list(header, &seg_free_list[array_index]);
    }
    else{
        /*Just free the block*/
        (*header).header.allocated = 0;
        (*header).header.padded = 0;
        (*header).header.two_zeroes = 0;
        (*header).header.unused = 0;
        (*footer).allocated = 0;
        (*footer).padded = 0;
        (*footer).two_zeroes = 0;
        (*footer).requested_size = 0;

        /*Determing which array to insert*/
        block_size = (*header).header.block_size << 4;
        for(int i = 0; 0 < 4; i++){
            if(block_size >= seg_free_list[i].min && block_size <= seg_free_list[i].max){
                array_index = i;
                break;
            }
        }
        insert_to_list(header, &seg_free_list[array_index]);
    }
	return;
}


sf_free_header* remove_from_list(free_list *list){

    sf_free_header *removed = (*list).head;;  //removed element
    (*list).head = (*(*list).head).next;  //set the head to next element

    return removed;
}

sf_free_header* remove_element_from_list(sf_free_header *header, free_list *list){
    sf_free_header *removed = header;

    if((*header).next != NULL){
        if((*header).prev == NULL){
            //The next become new head
            (*list).head = (*header).next;
        }
        else{
            (*(*header).next).prev = (*header).prev;
            (*(*header).prev).next = (*header).next;
        }
    }
    else{
        //Only 1 element, remove & set head to NULL
        (*list).head =  NULL;
    }

    return removed;
}

void insert_to_list(sf_free_header *head, free_list *list){
    /*Head of the list is NULL*/
    if((*list).head == NULL){
        /*Sprintf will override the next, thus manually set it to NULL*/
        (*head).next = NULL;
        (*head).prev = NULL;
        (*list).head = head;

    }
    else
    {
        (*(*list).head).prev = head;  //set the old head prev to new head
        (*head).next = (*list).head;  //set the new head next to old head
        (*list).head = head;          //insert the new head
    }
}

size_t ajudst_size(size_t size){

    size_t adjust_size;
    /*Adjust the block size*/
    if(size < DMEMORY_ROW){
        adjust_size = DMEMORY_ROW;
    }
    else if(size % DMEMORY_ROW != 0){
        adjust_size = DMEMORY_ROW * ((size + (DMEMORY_ROW - 1))/DMEMORY_ROW);
    }
    else
        adjust_size = size;

    return adjust_size;
}