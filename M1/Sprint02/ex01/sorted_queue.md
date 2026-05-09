// Global Variables
Maximum Buffer Size = 100
Queue Item Structure: contains a string buffer and a Red-Black Tree node
Red-Black Tree Root (represents the entire sorted tree, initially empty)
Pointer 'first' (points to the node with the highest priority, initially NULL)

Function pop(destination_buffer):
    if the 'first' pointer is NULL (the tree is empty):
        return 0 (FAILURE)
        
    copy the string from the 'first' node's buffer into 'destination_buffer'
    
    temporarily set the 'first' pointer to NULL
    
    find the next node in the Red-Black Tree (the in-order successor)
    if a next node exists:
        update the 'first' pointer to this next node
        
    erase the original 'first' node from the Red-Black Tree
    free the memory allocated for that node
    
    return 1 (SUCCESS)

Function push(source_buffer):
    allocate memory for a new queue node
    if memory allocation fails:
        return 0 (FAILURE)
        
    copy the string from 'source_buffer' into the new node's buffer
    
    start at the root of the Red-Black Tree to find the insertion point
    set a flag 'is_first' to true
    
    while traversing down the tree to find an empty spot:
        compare the first character of the new string with the current node's string
        if the new character is alphabetically smaller:
            move down to the left branch
        else:
            move down to the right branch
            set the 'is_first' flag to false
            
    if the 'is_first' flag is still true (meaning it's the smallest element so far):
        update the global 'first' pointer to this new node
        
    link the new node into the tree at the found position
    rebalance the Red-Black Tree colors
    
    return 1 (SUCCESS)

Function proc_open(inode, file):
    print "LKM: sorted_queue open" with current process ID
    return 0

Function proc_read(file, user_buffer, size, file_position):
    if file_position > 0:
        return 0 // Indicates End Of File (EOF) to the 'cat' command
        
    if the call to pop(local_buffer) returns 0 (failure):
        return 0 // Tree is empty, no data to read
        
    calculate the length of the string in local_buffer
    
    if the length is 0 or less, or if the user buffer size is too small:
        return ERROR
        
    copy data from 'local_buffer' to 'user_buffer'
    if copy fails:
        return ERROR
        
    add the string length to file_position
    return the amount of bytes read

Function proc_write(file, user_buffer, size, file_position):
    if the received size is greater than the Maximum Buffer Size:
        return ERROR
        
    copy data from 'user_buffer' to 'local_buffer'
    if copy fails:
        return ERROR
        
    ensure that the 'local_buffer' string is null-terminated
    
    if the call to push(local_buffer) returns 0 (failure):
        return ERROR // Memory allocation failed
        
    return the amount of bytes written

Function proc_close(inode, file):
    print "LKM: sorted_queue release" with current process ID
    return 0

Function proc_init():
    create proc entry "sorted_queue" with default permissions
    if creation fails:
        return memory ERROR
        
    print "LKM: /proc/sorted_queue created"
    
    return 0

Function proc_exit():
    remove proc entry "sorted_queue"
    print "LKM: /proc/sorted_queue removed"
    
    find the very first node in the Red-Black Tree
    while a node exists:
        erase the node from the Red-Black Tree
        free the memory allocated for the node
        find the next first node in the tree