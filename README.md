
# Multi-Threaded Concurrent Server

## Project Overview
This project involves implementing a multi-threaded, concurrent server in C. The server accepts incoming connections and queues them in a buffer according to the designated server scheduling policy. Worker threads handle the requests utilizing the provided `request.c` file. The project was developed as part of a course assignment.

## Features
- **Concurrency with Threads**:
  - Implemented using mutexes and condition variables to handle concurrent access to shared resources.
  - Two condition variables, `cond_nonempty` and `cond_nonfull`, are used to signal when the buffer has new requests or space is available.

- **Static Memory Allocation**:
  - Used static memory allocation for the buffer and thread array with `MAX_BUFFER_SIZE` set to 512 and `MAX_THREAD_AMOUNT` set to 128.
  - Static allocation was chosen to avoid issues with memory deallocation during server shutdown.

- **Request Handling**:
  - Utilized the `recv()` function from the `<sys/socket.h>` library to handle file descriptors without consuming them prematurely.
  - Implemented scheduling policies such as FIFO (First In, First Out) and SFF (Shortest File First) for request management.

## Implementation Details
1. **Concurrency Implementation**:
   - The server utilizes mutexes to protect shared resources and ensure safe access.
   - Condition variables manage the flow of requests into and out of the buffer, ensuring threads only access the buffer when it has requests.

2. **Memory Management**:
   - Initially attempted dynamic memory allocation but faced challenges with proper deallocation during server shutdown.
   - Transitioned to static memory allocation to ensure stable server behavior and avoid memory management issues.

3. **File Descriptor Handling**:
   - Encountered challenges with file descriptor consumption by the main thread, leading to partial handling by worker threads.
   - Resolved the issue by using the `recv()` function, which allows precise control over data reception without affecting the file descriptor.

4. **Scheduling Policies**:
   - **FIFO (First In, First Out)**: Requests are processed in the order they arrive.
   - **SFF (Shortest File First)**: Requests are processed based on file size, optimizing server performance for smaller tasks.

## Challenges and Solutions
- **Dynamic Memory Allocation**:
  - Faced difficulties in managing dynamic memory during server shutdown, leading to a switch to static memory allocation.
- **File Descriptor Handling**:
  - Struggled with file descriptor consumption by the main thread, which was resolved using the `recv()` function from `<sys/socket.h>`.

## Learning Outcomes
This project enhanced my understanding of concurrency in C, including the use of mutexes, condition variables, and thread synchronization. It also provided valuable experience in managing system-level programming challenges, such as memory allocation and file descriptor handling.

## How to Run
1. **Compile the Server**:
   ```bash
   gcc -o server server.c request.c -lpthread
   ```
2. **Run the Server**:
   ```bash
   ./server
   ```

## References
- `recv()` function from the `<sys/socket.h>` library for managing file descriptors.

