// Global Variables
Maximum Buffer Size = 100
Queue Size = 5
Circular Queue (array) with read (read_item) and write (write_item) indices initialized to 0

Function increment(index_pointer):
    save the current value of the index
    update the index to: (index + 1) MOD Queue Size
    return the saved value

Function is_empty(read, write):
    if read is equal to write:
        return TRUE
    else:
        return FALSE

Function is_full(read, write):
    next_write = (write + 1) MOD Queue Size
    if next_write is equal to read:
        return TRUE
    else:
        return FALSE

Function dequeue(destination_buffer):
    if the queue is NOT empty (is_empty is false):
        copy the message from the queue at position 'read_item' to 'destination_buffer'
        call increment(read_item)
        return 1 (SUCCESS)
    
    return 0 (FAILURE)

Function enqueue(source_buffer):
    if the queue is full (is_full is true):
        call increment(read_item) // Discards the oldest item to make room
        
    copy the message from 'source_buffer' to the queue at position 'write_item'
    call increment(write_item)
    
    return 1 (SUCCESS)

Function proc_read(file, user_buffer, size, file_position):
    if file_position > 0:
        return 0 // Indicates End Of File (EOF) to the 'cat' command
        
    if the call to dequeue(local_buffer) returns 0 (failure):
        return 0 // No data to read
        
    if the message size is invalid or the user buffer is too small:
        return ERROR
        
    copy data from 'local_buffer' to 'user_buffer'
    
    add the message size to file_position
    return the amount of bytes read

Function proc_write(file, user_buffer, size, file_position):
    if the received size is greater than the buffer limit:
        return ERROR
        
    copy data from 'user_buffer' to 'local_buffer'
    ensure that the 'local_buffer' string is null-terminated
    
    call enqueue(local_buffer)
    
    return the amount of bytes written