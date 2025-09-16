# System Programming Projects  

This repository contains four projects from **Sogang University CSE4100: System Programming** course.  
It covers fundamental concepts in system programming, including **data structures, process management, concurrent programming, and memory allocation**.  

---

## 📚 Project #1: MyLib  

### Description  
Implemented three core data structures frequently used in the Linux kernel: **List**, **Hash Table**, and **Bitmap**.  
An interactive program was developed to verify the functionality of these implementations.  

### Key Concepts  

- **List**  
  - Implemented a non-standard doubly linked list where the list entry (list_item) splits the pointer and data.  
  - Core list functions operate using the `list_elem` structure, which contains pointers to the previous and next elements.  

- **Hash Table**  
  - Built a hash table where each bucket is a list (`struct list *buckets`) to handle collisions.  
  - Values are stored in `hash_elem` structures, which, like the list, separate the list element pointers from the data.  

- **Bitmap**  
  - Implemented a memory-efficient bit array.  
  - The bitmap structure uses an array of unsigned long elements to store bits (`elem_type *bits`), reducing memory overhead compared to a byte-per-value approach.  

---

## 🖥 Project #2: MyShell  

### Description  
Developed a simple, customized Linux shell in three incremental phases.  
The shell supports **basic command execution, I/O redirection, and background job control**.  

### Project Phases  

- **Phase I: Building and Testing Your Shell (30 Points)**  
  - Built a basic shell that can execute internal commands such as `cd`, `ls`, `mkdir`, `rmdir`, `touch`, `cat`, `echo`, and `exit`.  
  - Commands (except `exit` and `cd`) are executed by a child process created using `fork()`.  
  - The shell operates in a loop of printing a prompt, reading input, parsing the command line, and executing the command.  

- **Phase II: Redirection and Pipe (30 Points)**  
  - Extended the shell to support **pipes (|)**.  
  - Passed the output of one process as the input to another, enabling complex commands like `ls -al | grep filename`.  
  - Implemented recursive handling of multiple piped commands.  

- **Phase III: Run Processes in Background (40 Points)**  
  - Added **job control functionality** with `&` at the end of command lines.  
  - Implemented built-in commands: `jobs`, `bg`, `fg`, and `kill` to manage background jobs.  
  - Handled signals like **SIGINT (Ctrl-C)** and **SIGTSTP (Ctrl-Z)**.  

---

## 🌐 Project #3: Concurrent Stock Server  

### Description  
Built a **concurrent stock server** capable of handling multiple client requests simultaneously.  
Clients can **view, buy, and sell stocks**. Stock data is loaded from a `stock.txt` file into a binary tree in memory and saved back upon server termination.  

### Tasks  

- **Task 1: Event-driven Approach (30 Points)**  
  - Implemented the server using an **event-driven model** with the `select()` system call.  
  - Server monitors file descriptors triggered by clients and initiates actions based on events.  

- **Task 2: Thread-based Approach (30 Points)**  
  - Re-implemented the server using a **thread-based approach**.  
  - A master thread accepts new connections, while a pool of worker threads handles client requests.  

- **Task 3: Performance Evaluation and Analysis (30 Points)**  
  - Measured and analyzed the performance of both event-driven and thread-based approaches.  
  - Focused on concurrency throughput (requests per unit time) as the number of client processes increased.  
  - Compared performance under different workloads (read-heavy vs. write-heavy).  

---

## 📝 Project #4: Mallocator  

### Description  
Developed a **custom dynamic memory allocator** implementing `malloc`, `free`, and `realloc` for C programs.  
The objective was to create a **correct, efficient, and fast memory manager**.  

### Key Concepts  

- **Dynamic Memory Allocation**  
  - Implemented the core functions `mm_init`, `mm_malloc`, `mm_free`, and `mm_realloc` (declared in `mm.h` and defined in `mm.c`).  
  - Managed a heap region and ensured 8-byte aligned pointers.  

- **Heap Consistency Checker**  
  - Created a heap checker function `int mm_check(void)` to scan the heap and verify its consistency.  
  - Used as a crucial debugging tool (removed in production for performance).  

- **Performance Evaluation**  
  - Evaluated performance based on **space utilization** (minimizing fragmentation) and **throughput** (operations per second).  
  - Final score is a weighted sum, favoring space utilization over throughput.  

---


---
