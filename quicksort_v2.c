#include "mpi.h"
#include "sequential_quicksort.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int* create_array(int size);
int get_partner(int d, int task, int* taskelem, int numtasks);
void print_array(int* arr, int size);
void merge_sorted_arrays(int* result, int* sub_array, int size1, int size2);

#define MAXNUMBER 1000      /* maximum number in the array */
#define MASTER 0            /* task ID of master task */

int main(int argc, char *argv[]) {
    int* taskselem;
    int* numbers;
    int* sub_numbers;
    int* final_result;
    int* rec_array;
    int taskid, numtasks, numbelem;
    int i, j, size_subarray, size_rec, position_final;
    double start_time, end_time;
    MPI_Status status;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    printf("MPI task %d has started...\n", taskid);

    /* Check command-line arguments */
    if (argc != 2) {
        if (taskid == MASTER) {
            printf("usage: %s <numbers in array>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    numbelem = atoi(argv[1]);

    /* Set seed for random number generator equal to task ID */
    srand(taskid);
    numbers = NULL;
    if (taskid == MASTER) {
        numbers = create_array(numbelem);
        printf("Unsorted array:\n");
        print_array(numbers, numbelem);
    }

    /* Allocate memory for task elements and sub-array */
    taskselem = (int *)malloc(sizeof(int) * numtasks);
    for (j = 0; j < numtasks; j++) {
        taskselem[j] = j;
    }
    sub_numbers = (int *)malloc(sizeof(int) * ((int) ceil((double)numbelem / numtasks)));

    /* Scatter the random numbers from the root process to all processes */
    MPI_Scatter(numbers, (numbelem / numtasks), MPI_INT, sub_numbers, (numbelem / numtasks), MPI_INT, MASTER, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    if (taskid == MASTER) {
        start_time = MPI_Wtime();
    }

    /* Sort the subarray after partitioning */
    size_subarray = (numbelem / numtasks);
    quicksort(sub_numbers, 0, size_subarray - 1);

    /* Master gathers the results from all processes, others send result to master */
    if (taskid == MASTER) {
        final_result = (int *)malloc(numbelem * sizeof(int));
        for (i = 0; i < size_subarray; i++) {
            final_result[i] = sub_numbers[i];
        }
        position_final = size_subarray;

        /* Get each process result */
        for (i = 1; i < numtasks; i++) {
            rec_array = NULL;
            MPI_Recv(&size_rec, 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (size_rec == 0)
                continue;

            rec_array = (int *)malloc(sizeof(int) * size_rec);
            MPI_Recv(rec_array, size_rec, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (j = 0; j < size_rec; j++) {
                final_result[position_final] = rec_array[j];
                position_final++;
            }
            free(rec_array);
        }

        /* Perform final merge sort on gathered results */
        quicksort(final_result, 0, numbelem - 1);

        /* Print the final sorted array */
        printf("The sorted array is:\n");
        print_array(final_result, numbelem);

        end_time = MPI_Wtime();
        printf("Time taken is %f seconds\n", end_time - start_time);

        free(final_result);
    } else {
        MPI_Send(&size_subarray, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
        if (size_subarray != 0)
            MPI_Send(sub_numbers, size_subarray, MPI_INT, MASTER, 2, MPI_COMM_WORLD);
    }

    /* Finalize MPI */
    MPI_Finalize();
    return 0;
}

/* Returns the partner for a task */
int get_partner(int d, int task, int* taskelem, int numtasks) {
    int i;
    int dimenum = (int)pow(2, d - 1);
    
    for (i = 0; i < numtasks; i++) {
        if ((task ^ taskelem[i]) == dimenum)
            return (taskelem[i]);
    }
    return (-1);
}

/* Creates an array of random numbers */
int* create_array(int size) {
    int* numbers = (int *)malloc(size * sizeof(int));
    int i;
    for (i = 0; i < size; i++) {
        numbers[i] = rand() % (MAXNUMBER + 1);
    }
    return numbers;
}

/* Function to print an array */
void print_array(int* arr, int size) {
    int i;
    for (i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

