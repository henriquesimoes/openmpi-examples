#include <stdio.h>
#include <mpi.h>
#include <queue>

using namespace std;

enum commands {
  REQUEST,
  RELEASE,
  OK,
};


class Node {
  public:
    Node () {}
    Node(int rank): rank(rank) {}
    virtual void start() = 0;

  protected:
    int rank;
};

class Coordinator: public Node {
  public:
    Coordinator(int rank): Node(rank) {}

    void start() {
      printf("Starting coordinator (rank %d)...\n", rank);

      listen();
    }

  private:
    void listen() {
      int process = poll();
      send_ok(process);
    }

    int poll() {
      int seconds;
      MPI_Status status;

      MPI_Recv(&seconds, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);

      printf("%d asked for the resource...\n", status.MPI_SOURCE);

      return status.MPI_SOURCE;
    }

    void send_ok(int process) {
      MPI_Send(NULL, 0, MPI_BYTE, process, OK, MPI_COMM_WORLD);
    }
};


class Worker: public Node {
  public:
    Worker(int rank, int coord): Node(rank), coord(coord) {}
    
    void start() {
      printf("Starting worker %d...\n", rank);

      request();
    }

  private:
    int coord;

    void request() {
      int seconds = 3;

      MPI_Send(&seconds, 1, MPI_INT, coord, REQUEST, MPI_COMM_WORLD);

      MPI_Recv(NULL, 0, MPI_BYTE, coord, OK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
};


int main() {
  MPI_Init(NULL, NULL);

  const int coord = 0;
  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  Node* node;

  if (rank == coord)
    node = new Coordinator(rank);
  else
    node = new Worker(rank, coord);

  node->start();

  MPI_Finalize();

  return EXIT_SUCCESS;
}
