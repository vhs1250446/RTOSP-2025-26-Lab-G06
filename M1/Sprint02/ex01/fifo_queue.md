// Global Variables
Maximum Buffer Size = 100
Queue Item Structure: contains a string buffer and a linked list node
Linked List Head (represents the start of the queue)

Function pop(destination_buffer):
    if the linked list is empty:
        return 0 (FAILURE)
        
    get the first node from the head of the linked list
    copy the string from the node's buffer into 'destination_buffer'
    
    remove the node from the linked list
    free the memory allocated for the node
    
    return 1 (SUCCESS)

Function push(source_buffer):
    allocate memory for a new queue node
    if memory allocation fails:
        return 0 (FAILURE)
        
    copy the string from 'source_buffer' into the new node's buffer
    add the new node to the tail of the linked list
    
    return 1 (SUCCESS)

Function proc_open(inode, file):
    print "LKM: fifo_queue open" with current process ID
    return 0

Function proc_read(file, user_buffer, size, file_position):
    if file_position > 0:
        return 0 // Indicates End Of File (EOF) to the 'cat' command
        
    if the call to pop(local_buffer) returns 0 (failure):
        return 0 // Queue is empty, no data to read
        
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
    print "LKM: fifo_queue close" with current process ID
    return 0

Function proc_init():
    create proc entry "fifo_queue" with 0666 permissions
    if creation fails:
        return memory ERROR
        
    print "LKM: /proc/fifo_queue created"
    initialize the Linked List Head (set it up as an empty list)
    
    return 0

Function proc_exit():
    remove proc entry "fifo_queue"
    print "LKM: /proc/fifo_queue removed"
    
    for each node currently remaining in the linked list:
        remove the node from the list
        free the memory allocated for the node