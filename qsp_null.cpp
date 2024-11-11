#include <mpi.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#define MASTER 0

void hypercube_quicksort(std::vector<int>& B, int d, int id);

std::vector<int> parse_input_file(const std::string& filename);

int main(int argc, char* argv[]) {
    int taskid, numtasks;
    int d;  // Dimension of the hypercube

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    double start_time = MPI_Wtime();  // Start timing the main execution

    // Determine the hypercube dimension
    d = (int)log2(numtasks);
    if ((1 << d) != numtasks) {
        if (taskid == MASTER) {
            std::cerr << "Number of processes must be a power of 2 for hypercube topology.\n";
        }
        MPI_Finalize();
        return 1;
    }

    std::vector<int> B;
    int num_elements;

    if (taskid == MASTER) {
        // Read and parse the input file on the MASTER process
        B = parse_input_file("input.txt");

        num_elements = B.size();
        if (num_elements % numtasks != 0) {
            std::cerr << "The number of elements must be divisible by the number of tasks.\n";
            MPI_Finalize();
            return 1;
        }

        std::cout << "Using explicitly provided list: ";
        for (int val : B) std::cout << val << " ";
        std::cout << std::endl;
    }

    // Broadcast the number of elements to all processes
    MPI_Bcast(&num_elements, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    int local_size = num_elements / numtasks;
    std::vector<int> local_B(local_size);  // Local data buffer

    // Scatter the data from the MASTER process to all processes
    MPI_Scatter(B.data(), local_size, MPI_INT, local_B.data(), local_size, MPI_INT, MASTER, MPI_COMM_WORLD);

    std::cout << "Process " << taskid << " initial array: ";
    for (int val : local_B) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    double sort_start_time = MPI_Wtime();  // Start timing the sorting
    hypercube_quicksort(local_B, d, taskid);  // Perform hypercube quicksort
    double sort_end_time = MPI_Wtime();    // End timing the sorting

    std::cout << "Process " << taskid << " sorted array: ";
    for (int val : local_B) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    // Gather the sizes of local_B from all processes
    int local_B_size = local_B.size();
    std::vector<int> recv_counts(numtasks);
    MPI_Gather(&local_B_size, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Compute displacements for MPI_Gatherv
    std::vector<int> displs(numtasks);
    if (taskid == MASTER) {
        displs[0] = 0;
        for (int i = 1; i < numtasks; ++i) {
            displs[i] = displs[i - 1] + recv_counts[i - 1];
        }
        num_elements = displs[numtasks - 1] + recv_counts[numtasks - 1];
        B.resize(num_elements);  // Resize B to hold the final sorted data
    }

    // Gather all sorted segments at the MASTER process
    MPI_Gatherv(local_B.data(), local_B_size, MPI_INT, B.data(), recv_counts.data(),
                displs.data(), MPI_INT, MASTER, MPI_COMM_WORLD);

    if (taskid == MASTER) {
        // The data in B is already sorted across processes
        std::cout << "Final sorted list: ";
        for (int val : B) std::cout << val << " ";
        std::cout << std::endl;

        double end_time = MPI_Wtime();  // End timing the entire execution

        std::cout << "Total execution time: " << end_time - start_time << " seconds\n";
        std::cout << "Total sorting time: " << sort_end_time - sort_start_time << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

void hypercube_quicksort(std::vector<int>& B, int d, int id) {
    MPI_Comm old_comm = MPI_COMM_WORLD;

    for (int i = d - 1; i >= 0; --i) {
        int color = (id >> i) & 1;

        // Split the communicator into two groups
        MPI_Comm new_comm;
        MPI_Comm_split(old_comm, color, id, &new_comm);

        // Get rank in the new communicator
        int new_rank;
        MPI_Comm_rank(new_comm, &new_rank);

        // Group leader selects pivot
        int pivot;
        if (new_rank == 0) {
            // For simplicity, select median of local data
            pivot = B[B.size() / 2];
        }

        // Broadcast pivot within the new communicator
        MPI_Bcast(&pivot, 1, MPI_INT, 0, new_comm);

        // Partition data based on pivot
        std::vector<int> B1, B2;
        for (size_t j = 0; j < B.size(); ++j) {
            if (B[j] <= pivot)
                B1.push_back(B[j]);
            else
                B2.push_back(B[j]);
        }

        // Determine partner process in the other group
        int partner = id ^ (1 << i);

        // Exchange data with partner
        std::vector<int> send_data, recv_data;
        int send_size, recv_size;

        if (color == 0) {
            // Keep B1, send B2
            send_data = B2;
        } else {
            // Keep B2, send B1
            send_data = B1;
        }
        send_size = send_data.size();

        // Exchange sizes first
        MPI_Sendrecv(&send_size, 1, MPI_INT, partner, 0,
                     &recv_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Prepare receive buffer
        recv_data.resize(recv_size);

        // Exchange data
        MPI_Sendrecv(send_data.data(), send_size, MPI_INT, partner, 0,
                     recv_data.data(), recv_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Merge received data
        if (color == 0) {
            B = B1;
        } else {
            B = B2;
        }
        B.insert(B.end(), recv_data.begin(), recv_data.end());

        // Free the communicator
        MPI_Comm_free(&new_comm);
    }

    // Now, each process sorts its local B
    std::sort(B.begin(), B.end());
}

// Function to parse input file of format "{ number, number, ... }"
std::vector<int> parse_input_file(const std::string& filename) {
    std::vector<int> numbers;
    std::ifstream file(filename);
    std::string line;

    if (file.is_open() && std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '{'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '}'), line.end());

        std::stringstream ss(line);
        int number;
        char comma;
        while (ss >> number) {
            numbers.push_back(number);
            ss >> comma; // Skip commas
        }
    } else {
        std::cerr << "Error reading file: " << filename << std::endl;
    }

    return numbers;
}
