#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MASTER 0        /* tas id of master task */

// Function declarations
void swap(int* a, int* b);
int partition (int arr[], int low, int high);
void quickSort(int arr[], int low, int high);
void printArray(int arr[], int size, bool demo_mode);


// Main func
int main (int argc, char *argv[])
{
int	taskid;         /* task ID - used as seed number */
int numtasks;       /* number of tasks */

bool demo_mode = false; // Toggle between demo and test modes

int random_array_seq[] = {189, 65, 204, 303, 330, 382, 295, 335, 499, 425, 67, 476, 419, 199, 153, 336, 131, 76, 197, 114, 342, 442, 88, 333, 442, 327, 17, 46, 482, 179, 109, 497, 479, 50, 127, 447, 424, 283, 249, 322, 476, 365, 93, 225, 441, 212, 293, 288, 480, 33, 357, 297, 100, 132, 130, 222, 356, 377, 153, 389, 179, 429, 449, 447, 108, 499, 317, 464, 241, 133, 47, 309, 366, 335, 426, 416, 128, 465, 104, 457, 218, 269, 114, 84, 441, 243, 431, 78, 345, 269, 424, 123, 321, 94, 309, 87, 355, 300, 121, 349, 347, 279, 300, 170, 255, 307, 70, 387, 17, 83, 117, 474, 427, 478, 213, 496, 50, 74, 166, 258, 298, 341, 75, 318, 328, 36, 77, 46, 20, 195, 455, 71, 230, 159, 208, 46, 250, 321, 297, 85, 410, 118, 102, 485, 352, 494, 270, 475, 137, 320};

double start, end; 

/* Obtain number of tasks and task ID */
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
MPI_Comm_rank(MPI_COMM_WORLD,&taskid);

printf ("MPI task %d has started...\n", taskid);

MPI_Barrier(MPI_COMM_WORLD); 
if(taskid == MASTER)
    start = MPI_Wtime();

{
    int n = sizeof(random_array_seq)/sizeof(random_array_seq[0]); 
    printf("\nUnsorted array: ");
    printArray(random_array_seq, n, demo_mode);

    quickSort(random_array_seq, 0, n-1);

    printf("\nSorted array: ");
    printArray(random_array_seq, n, demo_mode);
}

MPI_Barrier(MPI_COMM_WORLD); 
if (taskid == MASTER) {
    end = MPI_Wtime();
    printf("Runtime = %f\n", end-start);
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