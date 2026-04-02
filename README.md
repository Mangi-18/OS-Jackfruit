# Multi-Container Runtime (Mini Docker in C)

## Overview

This project implements a lightweight container runtime in C.
It supports process isolation using Linux namespaces, chroot-based filesystem isolation, and basic resource control.

---

## Features Implemented

### 1. Container Execution

* Run containers using `fork` and `exec`
* Execute commands inside isolated environments

### 2. Filesystem Isolation

* Implemented using `chroot`
* Each container uses its own root filesystem (Alpine Linux)

### 3. UTS Namespace

* Each container has its own hostname
* Verified using `hostname` command

### 4. PID Namespace

* Each container has isolated process space
* Verified using `ps` command

### 5. Mount Namespace

* `/proc` mounted inside container

### 6. Process Priority

* Implemented using `nice`
* Allows custom scheduling priority

### 7. Multiple Containers

* Multiple containers executed sequentially
* Each container has independent environment

---

## Screenshots

### Screenshot 1: Basic Execution
### Screenshot 1: Basic Execution
![Basic Execution](image.png)

### Screenshot 2: Fork + Exec
![alt text](image-1.png)

### Screenshot 3: Filesystem Isolation (chroot)

![alt text](image-2.png)

### Screenshot 4: UTS Namespace (hostname)

![alt text](image-3.png)

### Screenshot 5: PID Namespace (ps output)

![alt text](image-4.png)

### Screenshot 6: Nice Priority

![alt text](image-5.png)

### Screenshot 7: Container c1

![alt text](image-6.png)

### Screenshot 8: Container c2

![alt text](image-7.png)

---

## How to Run

```bash
make
sudo ./engine run <id> <rootfs-path> /bin/sh
```

---

## Conclusion

This project demonstrates core containerization concepts such as namespaces, process isolation, and filesystem separation, similar to Docker at a simplified level.
