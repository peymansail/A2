#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "sequential_quicksort.h"
#include "mpi.h"

#define MAXNUMBER 1000 

int main(int argc, char *argv[]) { 
    int* numbers;
    int i,numbelem;
    double start_time, end_time;
    int	taskid;         
    int numtasks;  
    
    if ( argc != 2 )
      printf( "usage: %s <number of elements in array>\n", argv[0] );
    else
      numbelem = atoi( argv[1] );
    start_time =  MPI_Wtime();
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);

    srand(taskid);
    numbers=(int*) malloc(numbelem* sizeof(int));
    numbers=create_array_sequential(numbelem);
    
    //printf("The initial array is:\n");
    //for(i=0;i<numbelem;i++){
    //   printf("%d  ",numbers[i]);
    //}
    //printf("\n");

  
    quicksort(numbers, 0, numbelem-1); 
    //printf("Sorted array: \n"); 
    //for(i=0;i<numbelem;i++){
    //   printf("%d  ",numbers[i]);
    //}
    printf("calculate final result\n");

    end_time =  MPI_Wtime();
    printf("Time taken is %f seconds \n",end_time - start_time);
    return 0; 
} 


