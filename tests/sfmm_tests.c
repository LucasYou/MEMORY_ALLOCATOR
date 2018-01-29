#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "sfmm.h"

int find_list_index_from_size(int sz) {
  if (sz >= LIST_1_MIN && sz <= LIST_1_MAX) return 0;
  else if (sz >= LIST_2_MIN && sz <= LIST_2_MAX) return 1;
  else if (sz >= LIST_3_MIN && sz <= LIST_3_MAX) return 2;
  else return 3;
}

Test(sf_memsuite_student, Malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
  sf_errno = 0;
  int *x = sf_malloc(sizeof(int));

  cr_assert_not_null(x);

  *x = 4;

  cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

  sf_header *header = (sf_header*)((char*)x - 8);

  /* There should be one block of size 4064 in list 3 */
  free_list *fl = &seg_free_list[find_list_index_from_size(PAGE_SZ - (header->block_size << 4))];

  cr_assert_not_null(fl, "Free list is null");

  cr_assert_not_null(fl->head, "No block in expected free list!");
  cr_assert_null(fl->head->next, "Found more blocks than expected!");
  cr_assert(fl->head->header.block_size << 4 == 4064);
  cr_assert(fl->head->header.allocated == 0);
  cr_assert(sf_errno == 0, "sf_errno is not zero!");
  cr_assert(get_heap_start() + PAGE_SZ == get_heap_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, Malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
  sf_errno = 0;
  void *x = sf_malloc(PAGE_SZ << 2);

  cr_assert_null(x, "x is not NULL!");
  cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_double_free, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = 2, .signal = SIGABRT) {
  sf_errno = 0;
  void *x = sf_malloc(sizeof(int));
  sf_free(x);
  sf_free(x);
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
  sf_errno = 0;
  /* void *x = */ sf_malloc(sizeof(long));
  void *y = sf_malloc(sizeof(double) * 10);
  /* void *z = */ sf_malloc(sizeof(char));

  sf_free(y);

  free_list *fl = &seg_free_list[find_list_index_from_size(96)];

  cr_assert_not_null(fl->head, "No block in expected free list");
  cr_assert_null(fl->head->next, "Found more blocks than expected!");
  cr_assert(fl->head->header.block_size << 4 == 96);
  cr_assert(fl->head->header.allocated == 0);
  cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
  sf_errno = 0;
  /* void *w = */ sf_malloc(sizeof(long));
  void *x = sf_malloc(sizeof(double) * 11);
  void *y = sf_malloc(sizeof(char));
  /* void *z = */ sf_malloc(sizeof(int));

  sf_free(y);
  sf_free(x);

  free_list *fl_y = &seg_free_list[find_list_index_from_size(32)];
  free_list *fl_x = &seg_free_list[find_list_index_from_size(144)];

  cr_assert_null(fl_y->head, "Unexpected block in list!");
  cr_assert_not_null(fl_x->head, "No block in expected free list");
  cr_assert_null(fl_x->head->next, "Found more blocks than expected!");
  cr_assert(fl_x->head->header.block_size << 4 == 144);
  cr_assert(fl_x->head->header.allocated == 0);
  cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
  /* void *u = */ sf_malloc(1);          //32
  void *v = sf_malloc(LIST_1_MIN); //48
  void *w = sf_malloc(LIST_2_MIN); //160
  void *x = sf_malloc(LIST_3_MIN); //544
  void *y = sf_malloc(LIST_4_MIN); //2080
  /* void *z = */ sf_malloc(1); // 32

  int allocated_block_size[4] = {48, 160, 544, 2080};

  sf_free(v);
  sf_free(w);
  sf_free(x);
  sf_free(y);

  // First block in each list should be the most recently freed block
  for (int i = 0; i < FREE_LIST_COUNT; i++) {
    sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
    cr_assert_not_null(fh, "list %d is NULL!", i);
    cr_assert(fh->header.block_size << 4 == allocated_block_size[i], "Unexpected free block size!");
    cr_assert(fh->header.allocated == 0, "Allocated bit is set!");
  }

  // There should be one free block in each list, 2 blocks in list 3 of size 544 and 1232
  for (int i = 0; i < FREE_LIST_COUNT; i++) {
    sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
    if (i != 2)
        cr_assert_null(fh->next, "More than 1 block in freelist [%d]!", i);
    else {
        cr_assert_not_null(fh->next, "Less than 2 blocks in freelist [%d]!", i);
        cr_assert_null(fh->next->next, "More than 2 blocks in freelist [%d]!", i);
    }
  }
}



//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sf_memsuite_student, invalid_free, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT){
    /*This should expect abort*/
    void* a = sf_malloc(16);
    sf_header *header = (void *)a - 8;
    sf_footer *footer = (void *)header + ((*header).block_size << 4 )- 8;
    header->padded = 1;
    footer->padded = 1;
    sf_free(a);
}

Test(sf_memsuite_student, sf_realloc_invalid_padded, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = 2, .signal = SIGABRT) {
    void* a = sf_malloc(32);
    sf_header *header = (void *)a - 8;
    sf_footer *footer = (void *)header + ((*header).block_size << 4 )- 8;
    header->padded = 1;
    footer->padded = 1;
    sf_realloc(a, 48);
}

Test(sf_memsuite_student, sf_realloc_same_size, .init = sf_mem_init, .fini = sf_mem_fini){
    /*Relloc to same size, different requested_size*/
    void* a = sf_malloc(1);
    void* b = sf_realloc(a, 16);

    /*Expect request size to 16 & padding to 0*/
    sf_header *header = (void *)a - 8;
    sf_footer *footer = (void *)header + ((*header).block_size << 4 )- 8;
    cr_assert(footer->requested_size == 16, "footer requested_size is not 16");
    cr_assert(footer->padded == 0, "footer padded is not 0");
    cr_assert(a == b, "return address is different");
}

Test(sf_memsuite_student, sf_realloc_to_smaller_splinter, .init = sf_mem_init, .fini = sf_mem_fini){
    /*Relloc to smaller size with spinlter*/
    void* a = sf_malloc(32);
    sf_realloc(a, 15);

    sf_header *header = (void *)a - 8;
    sf_footer *footer = (void *)header + ((*header).block_size << 4 )- 8;
    cr_assert(header->padded == 1, "header padded is not 1");
    cr_assert(footer->block_size == (48 >> 4), "footer block_size is not 48");
    cr_assert(footer->requested_size == 15, "footer block_size is not 48");
    cr_assert(footer->padded == 1, "footer padded is not 1");

    cr_assert(seg_free_list[0].head == NULL, "list 1 is not NULL");
    cr_assert(seg_free_list[1].head == NULL, "list 2 is not NULL");
    cr_assert(seg_free_list[2].head == NULL, "list 3 is not NULL");
    cr_assert(seg_free_list[3].head != NULL, "list 4 is NULL");
}

Test(sf_memsuite_student, sf_realloc_to_smaller, .init = sf_mem_init, .fini = sf_mem_fini){
    /*Relloc to smaller size*/
    void* a = sf_malloc(128);
    sf_malloc(1);
    sf_realloc(a, 48);

    sf_header *header = (void *)a - 8;
    sf_footer *footer = (void *)header + ((*header).block_size << 4 )- 8;
    cr_assert(header->padded == 0, "header padded is not 0");
    cr_assert(footer->block_size == (64 >> 4), "footer block_size is not 64");
    cr_assert(footer->requested_size == 48, "footer block_size is not 48");
    cr_assert(footer->padded == 0, "footer padded is not 1");

    cr_assert(seg_free_list[0].head != NULL, "list 1 is NULL");
    cr_assert(seg_free_list[1].head == NULL, "list 2 is not NULL");
    cr_assert(seg_free_list[2].head == NULL, "list 3 is not NULL");
    cr_assert(seg_free_list[3].head != NULL, "list 4 is NULL");
}


