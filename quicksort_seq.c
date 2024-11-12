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
    int taskid;         /* task ID - used as seed number */
    int numtasks;       /* number of tasks */
    bool demo_mode = false; // Toggle between demo and test modes

    int data_size = 1000;  // Size of the random array
    double start, end;

    /* Obtain number of tasks and task ID */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    printf("MPI task %d has started...\n", taskid);

    MPI_Barrier(MPI_COMM_WORLD);
    if (taskid == MASTER)
        start = MPI_Wtime();

    // Generate a random array instead of using a hard-coded array
    int* random_array_seq = create_array(data_size);

    // Main sorting section
    if (taskid == MASTER) {
        printf("\nUnsorted array: ");
        printArray(random_array_seq, data_size, demo_mode);

        quickSort(random_array_seq, 0, data_size - 1);

        printf("\nSorted array: ");
        printArray(random_array_seq, data_size, demo_mode);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (taskid == MASTER) {
        end = MPI_Wtime();
        printf("Runtime = %f\n", end - start);
    }

    MPI_Finalize();
    free(random_array_seq);  // Free dynamically allocated memory
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

