# Parallel Image Processing

This project explores the use of parallelism to optimize the performance of image processing tasks by leveraging multiple cores. It implements three different parallel strategies—using processes (with pipes and shared memory) and threads (with atomic operations)—to execute image transformations through three stages: smoothening, detail enhancement, and sharpening.

## 🚀 Objective

To improve the efficiency of image processing through parallelism and compare:
- Sequential execution (baseline)
- Process-based parallelism (pipes & shared memory)
- Thread-based parallelism (atomic operations)

## 🧩 Project Structure

1. **Baseline Version**  
   Processes the image sequentially through stages S1, S2, and S3.

2. **Process-based Parallelism**  
   - **Pipes:** Parallel processing of rows with unnamed pipes between three processes.
   - **Shared Memory + Semaphores:** Data is shared via memory with synchronization through semaphores.

3. **Thread-based Parallelism**  
   - Threads communicate using atomic variables with careful synchronization to avoid race conditions.

## 🧪 Results Summary

| Method                  | Total Time (s) | Speed-up |
|------------------------|----------------|----------|
| Baseline               | 11.57          | 1.00x    |
| Process + Pipes        | 11.10          | 1.04x    |
| Shared Memory + Semaphores | 11.50      | 1.006x   |
| Thread + Atomic Ops    | **10.97**      | **1.05x**|

## ✅ Data Integrity

A checksum validation is used between stages to ensure correct data transmission. Each row’s checksum is appended and verified before processing.

## 🔧 Challenges Faced

- **Pipes:** Avoiding deadlocks while sending row-wise data.
- **Shared Memory:** Ensuring proper synchronization between stages.
- **Atomic Operations:** Maintaining correct execution order across threads.

## 📌 Conclusion

Thread-based parallelism using atomic operations achieved the best performance, balancing ease of implementation with processing speed. Process-based methods also showed benefits but added complexity due to inter-process communication.

## 📷 Stages Overview

- **S1:** Smoothen the image
- **S2:** Find details
- **S3:** Sharpen the image

---

## 🛠️ How to Run (Assuming Linux)


# Compile and run your selected implementation
g++ -o main main.cpp -lpthread
./main input_image.bmp output_image.bmp
