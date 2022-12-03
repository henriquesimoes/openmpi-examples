#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <queue>

#define MAX_SECONDS_WITH_RESOURCE 25
#define MAX_RESOURCE_TAKE 5


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
    void log(const char *fmt, ...) {
      char* msg;
      va_list args;

      va_start(args, fmt);
      vasprintf(&msg, fmt, args);

      time_t timer = time(NULL);

      printf("node %d @ %.24s: %s\n", rank, ctime(&timer), msg);

      va_end(args);
      free(msg);
    }

  protected:
    int rank;
};

class Coordinator: public Node {
  public:
    Coordinator(int rank): Node(rank) {}

    void start() {
      log("Starting coordinator (rank %d)...", rank);

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

      log("Node %d asked for the resource for %d seconds...", status.MPI_SOURCE, seconds);

      return status.MPI_SOURCE;
    }

    void send_ok(int process) {
      log("Allowing node %d to use resource.", process);

      MPI_Send(NULL, 0, MPI_BYTE, process, OK, MPI_COMM_WORLD);
    }
};


class Worker: public Node {
  public:
    Worker(int rank, int coord): Node(rank), coord(coord) {}
    
    void start() {
      log("Starting worker (rank %d)...", rank);

      int n = random() % MAX_RESOURCE_TAKE;

      while (n--) {
        int seconds = random() % MAX_SECONDS_WITH_RESOURCE;
        
        request(seconds);
        use(seconds);
        release();
      }
    }

  private:
    int coord;

    void request(int seconds) {
      MPI_Send(&seconds, 1, MPI_INT, coord, REQUEST, MPI_COMM_WORLD);

      MPI_Recv(NULL, 0, MPI_BYTE, coord, OK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    void use(int seconds) {
      log("Using resource for %d seconds...", seconds);

      sleep(seconds);

      log("Finished using resource.");
    }

    void release() {
      MPI_Send(NULL, 0, MPI_BYTE, coord, RELEASE, MPI_COMM_WORLD);
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
