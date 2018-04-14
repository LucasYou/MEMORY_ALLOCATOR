# MEMORY_ALLOCATOR

Supports:

        Malloc
        Realloc
        Free
        
The memory allocator use 4 category segregated free list to store the blocks.
The maximum size of the allocator is 4 Page. ie. 16384 bytes.
Coalescing backward when malloc.
Forward coalescing only when sf_bark(). (When run out of space, call sf_bark() to allocate a new page)

Built-in functions to check current state of the memory allocater:

        /**
        * Function which outputs the state of the free-list to stdout.
        * Performs checks on the placement of the header and footer,
        * and if the memory payload is correctly aligned.
        * @param verbose If true, snapshot will additionally print out
        */
        void sf_snapshot(bool verbose);
        
        /**
        * Function which prints human readable block format
        * readable format.
        * @param block Address of the block header in memory.
        */
        void sf_blockprint(void* block);
        
        /**
        * Prints human readable block format
        * from the address of the payload.
        * IE. subtracts header size from the data pointer to obtain the address
        * of the block header. Calls sf_blockprint internally to print.
        * @param data Pointer to payload data in memory (value returned by
        * sf_malloc).
        */
        void sf_varprint(void *data);
