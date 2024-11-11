#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MASTER 0        /* task id of master task */

// Function declarations
void swap(int* a, int* b);
int partition(int arr[], int low, int high);
void quickSort(int arr[], int low, int high);
void printArray(int arr[], int size, bool demo_mode);

// Main function
int main(int argc, char *argv[])
{
    int taskid, numtasks;
    bool demo_mode = false;
    
    int data_size = 150; // Size of data to sort
    int random_array_seq[] = {189, 65, 204, 303, 330, 382, 295, 335, 499, 425, 67, 476, 419, 199, 153, 336, 131, 76, 197, 114, 342, 442, 88, 333, 442, 327, 17, 46, 482, 179, 109, 497, 479, 50, 127, 447, 424, 283, 249, 322, 476, 365, 93, 225, 441, 212, 293, 288, 480, 33, 357, 297, 100, 132, 130, 222, 356, 377, 153, 389, 179, 429, 449, 447, 108, 499, 317, 464, 241, 133, 47, 309, 366, 335, 426, 416, 128, 465, 104, 457, 218, 269, 114, 84, 441, 243, 431, 78, 345, 269, 424, 123, 321, 94, 309, 87, 355, 300, 121, 349, 347, 279, 300, 170, 255, 307, 70, 387, 17, 83, 117, 474, 427, 478, 213, 496, 50, 74, 166, 258, 298, 341, 75, 318, 328, 36, 77, 46, 20, 195, 455, 71, 230, 159, 208, 46, 250, 321, 297, 85, 410, 118, 102, 485, 352, 494, 270, 475, 137, 320};
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    
    // Local array for each process
    int local_size = (data_size + numtasks - 1) / numtasks; // Calculate ceiling of data_size / numtasks
    int *local_array = (int *)malloc(local_size * sizeof(int));
    
    // Scatter data across processes
    MPI_Scatter(random_array_seq, local_size, MPI_INT, local_array, local_size, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Step 1: Local sort
    int actual_local_size = (taskid == numtasks - 1) ? (data_size - local_size * (numtasks - 1)) : local_size;
    quickSort(local_array, 0, actual_local_size - 1);

    // Step 2: Sampling
    int *samples = (int *)malloc(numtasks * sizeof(int));
    for (int i = 0; i < numtasks; i++) {
        samples[i] = local_array[i * actual_local_size / numtasks];
    }

    // Gather samples on the master process
    int *all_samples = NULL;
    if (taskid == MASTER) {
        all_samples = (int *)malloc(numtasks * numtasks * sizeof(int));
    }
    MPI_Gather(samples, numtasks, MPI_INT, all_samples, numtasks, MPI_INT, MASTER, MPI_COMM_WORLD);
    
    int *pivots = (int *)malloc((numtasks - 1) * sizeof(int));
    if (taskid == MASTER) {
        quickSort(all_samples, 0, numtasks * numtasks - 1);
        for (int i = 1; i < numtasks; i++) {
            pivots[i - 1] = all_samples[i * numtasks + numtasks / 2];
        }
    }
    MPI_Bcast(pivots, numtasks - 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Step 4: Partition local array based on pivots
    int *partition_sizes = (int *)malloc(numtasks * sizeof(int));
    int **partitions = (int **)malloc(numtasks * sizeof(int *));
    for (int i = 0; i < numtasks; i++) {
        partitions[i] = (int *)malloc(local_size * sizeof(int));
        partition_sizes[i] = 0;
    }

    int current_partition = 0;
    for (int i = 0; i < actual_local_size; i++) {
        if (current_partition < numtasks - 1 && local_array[i] > pivots[current_partition]) {
            current_partition++;
        }
        partitions[current_partition][partition_sizes[current_partition]++] = local_array[i];
    }

    // Step 4b: All-to-all communication to redistribute partitions
    int *recv_counts = (int *)malloc(numtasks * sizeof(int));
    MPI_Alltoall(partition_sizes, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int *recv_offsets = (int *)malloc(numtasks * sizeof(int));
    int *send_offsets = (int *)malloc(numtasks * sizeof(int));
    int total_recv = 0;
    for (int i = 0; i < numtasks; i++) {
        send_offsets[i] = (i == 0) ? 0 : send_offsets[i - 1] + partition_sizes[i - 1];
        recv_offsets[i] = total_recv;
        total_recv += recv_counts[i];
    }

    int *recv_buffer = (int *)malloc(total_recv * sizeof(int));
    MPI_Alltoallv(&partitions[0][0], partition_sizes, send_offsets, MPI_INT, recv_buffer, recv_counts, recv_offsets, MPI_INT, MPI_COMM_WORLD);

    // Step 5: Local merging of received partitions
    quickSort(recv_buffer, 0, total_recv - 1);

    // Gather sorted data at the master process
    int *final_sorted = NULL;
    if (taskid == MASTER) {
        final_sorted = (int *)malloc(data_size * sizeof(int));
    }
    MPI_Gather(&total_recv, 1, MPI_INT, recv_counts, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Gatherv(recv_buffer, total_recv, MPI_INT, final_sorted, recv_counts, recv_offsets, MPI_INT, MASTER, MPI_COMM_WORLD);

    if (taskid == MASTER) {
        printf("\nFinal sorted array:\n");
        printArray(final_sorted, data_size, demo_mode);
    }

    MPI_Finalize();
    return 0;
}

/*
    Code below is from the website https://www.geeksforgeeks.org/quick-sort/
*/

// A utility function to swap two elements
void swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

// Partition function
int partition(int arr[], int low, int high) {
    
    // Choose the pivot
    int pivot = arr[high];
    
    // Index of smaller element and indicates 
    // the right position of pivot found so far
    int i = low - 1;

    // Traverse arr[low..high] and move all smaller
    // elements to the left side. Elements from low to 
    // i are smaller after every iteration
    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    
    // Move pivot after smaller elements and
    // return its position
    swap(&arr[i + 1], &arr[high]);  
    return i + 1;
}

// The QuickSort function implementation
void quickSort(int arr[], int low, int high) {
    if (low < high) {
        
        // pi is the partition return index of pivot
        int pi = partition(arr, low, high);

        // Recursion calls for smaller elements
        // and greater or equals elements
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

// Function to print an array with demo/test mode toggle
void printArray(int arr[], int size, bool demo_mode)
{
    int numbers_per_line = 10; // Controls numbers per line in demo mode
    
    if (demo_mode) {
        // Demo mode: Display with multiple numbers per line
        for (int i = 0; i < size; i++) {
            printf("%d ", arr[i]);
            // Print a newline after every 'numbers_per_line' elements
            if ((i + 1) % numbers_per_line == 0) {
                printf("\n");
            }
        }
        // Add a final newline if the last line wasn't complete
        if (size % numbers_per_line != 0) {
            printf("\n");
        }
    } else {
        // Test mode: Display all numbers on a single line
        for (int i = 0; i < size; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
}