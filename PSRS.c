#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MASTER 0        /* task id of master task */
#define MAXNUMBER 500   /* maximum number for random array generation */

// Function declarations
void swap(int* a, int* b);
int partition(int arr[], int low, int high);
void quickSort(int arr[], int low, int high);
void printArray(int arr[], int size, bool demo_mode);
int* create_array(int size);

// Main function
int main(int argc, char *argv[])
{
    int taskid, numtasks;
    bool demo_mode = false;
    
    int data_size = 150; // Size of data to sort
    
    // Generate the random array
    int* random_array_seq = create_array(data_size);
    
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
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);  
    return i + 1;
}

// The QuickSort function implementation
void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

// Function to print an array with demo/test mode toggle
void printArray(int arr[], int size, bool demo_mode)
{
    int numbers_per_line = 10;
    if (demo_mode) {
        for (int i = 0; i < size; i++) {
            printf("%d ", arr[i]);
            if ((i + 1) % numbers_per_line == 0) {
                printf("\n");
            }
        }
        if (size % numbers_per_line != 0) {
            printf("\n");
        }
    } else {
        for (int i = 0; i < size; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
}

// Function to create an array of random numbers
int* create_array(int size) {
    int* numbers = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        numbers[i] = rand() % (MAXNUMBER + 1);
    }
    return numbers;
}

