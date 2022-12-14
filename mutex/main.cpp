#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <queue>

#include <mpi.h>

#define MAX_SECONDS_WITH_RESOURCE 10
#define MAX_RESOURCE_TAKE 3


using namespace std;

enum commands {
  REQUEST,
  RELEASE,
  OK,
};

class Resource {
  public:
    const int FREE = -1;
    queue<int> waiting;
    int holding = FREE;

  public:
    Resource() {}

    bool is_free() {
      return holding == FREE;
    }

    void enqueue(int process) {
      waiting.push(process);
    }

    bool grant() {
      bool granted = false;

      if (!waiting.empty()) {
        assert(holding == FREE);

        holding = waiting.front();
        waiting.pop();

        granted = true;
      }

      return granted;
    }

    int get_process() {
      assert(holding != FREE);

      return holding;
    }

    void release() {
      holding = FREE;
    }
};


class Node {
  public:
    Node(int rank): rank(rank) {
      srand(1LL * rank * time(NULL));
    }

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
    Resource resource;

    void listen() {
      signal(SIGTERM, [](int signal) {
        fprintf(stderr, "Quitting...\n");

        MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);

        exit(EXIT_SUCCESS);
      });

      while (true) {
        process_incoming();

        if (resource.is_free()) {
          bool granted = resource.grant();

          if (granted)
            send_ok(resource.get_process());
        } else {
          int process = resource.get_process();

          process_release(process);
        }
      }
    }

    void process_incoming() {
      int available;

      MPI_Iprobe(MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &available, MPI_STATUS_IGNORE);

      if (available) {
        int process = read_request();

        resource.enqueue(process);
      }
    }

    int read_request() {
      int seconds;
      MPI_Status status;

      MPI_Recv(&seconds, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);

      log("Node %d asked for the resource for %d seconds...", status.MPI_SOURCE, seconds);

      return status.MPI_SOURCE;
    }

    void send_ok(int process) {
      MPI_Send(NULL, 0, MPI_BYTE, process, OK, MPI_COMM_WORLD);

      log("Allowed node %d to use resource.", process);
    }

    void process_release(int process) {
      int available;
      MPI_Iprobe(process, RELEASE, MPI_COMM_WORLD, &available, MPI_STATUS_IGNORE);

      if (available) {
        MPI_Recv(NULL, 0, MPI_BYTE, process, RELEASE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        resource.release();
      }
    }
};


class Worker: public Node {
  public:
    Worker(int rank, int coord): Node(rank), coord(coord) {}

    void start() {
      log("Starting worker (rank %d)...", rank);

      int n = 1 + (random() % (MAX_RESOURCE_TAKE - 1));

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
      log("Request resource for %d seconds...", seconds);

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
