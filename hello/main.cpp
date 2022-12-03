#include <mpi.h>
#include <stdio.h>

int main() {
  MPI_Init(NULL, NULL);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  char proc_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(proc_name, &name_len);

  printf("Hello from processor %s (rank %d of %d)\n", proc_name, world_rank, world_size);

  MPI_Finalize();

  return 0;
}
