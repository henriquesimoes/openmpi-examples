#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <queue>
#include <thread>
#include <mutex>

#include <mpi.h>

#define ELECTION_STARTER 0
#define RESPONSE_TIMEOUT 5

using namespace std;

enum commands {
  ELECTION,
  OK,
  RESULT,
};


class Node {
  private:
    int rank, size;
    bool electing = false;
    mutex ballot_box_lock;

    thread *listen_job, *election_job = nullptr;

  public:
    Node(int rank, int size): rank(rank), size(size) {}

    void start() {
      if (rank == size - 1)
        sleep(100);

      listen_job = new thread([this]() {
        this->listen_for_election();
      });

      if (rank == ELECTION_STARTER)
        start_election();
    }

    void finish() {
      listen_job->join();
      election_job->join();
    }

    void log(const char *fmt, ...) {
      char* msg;
      va_list args;

      va_start(args, fmt);
      vasprintf(&msg, fmt, args);

      time_t timer = time(NULL);

      printf("node %d @ %.24s: %s", rank, ctime(&timer), msg);

      va_end(args);
      free(msg);
    }

  private:
    void set_electing(bool electing) {
      lock_guard<mutex> guard(ballot_box_lock);

      this->electing = electing;
    }

    void start_election() {
      lock_guard<mutex> guard(ballot_box_lock);

      if (!electing) {
        electing = true;

        this->election_job = new thread([this]() {
          this->make_election();
        });
      }
    }

    void make_election() {
      log("Starting election...\n");
      set_electing(true);

      for (int process = rank + 1; process < size; process++)
        send_election(process);

      bool answered = wait_for_response();

      if (!answered)
        broadcast_victory();
      else
        wait_for_result();

      set_electing(false);
    }

    void listen_for_election() {
      MPI_Status status;

      while (true) {
        MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, ELECTION, MPI_COMM_WORLD, &status);
        MPI_Send(NULL, 0, MPI_BYTE, status.MPI_SOURCE, OK, MPI_COMM_WORLD);

        log("Received election request from %d (on election: %s)\n",
            status.MPI_SOURCE, electing ? "true" : "false");

        start_election();
      }
    }

    void send_election(int process) {
      MPI_Request request;
      MPI_Isend(NULL, 0, MPI_BYTE, process, ELECTION, MPI_COMM_WORLD, &request);
    }

    bool wait_for_response() {
      const int wait_time = 1;
      int answered = 0;
      int waited = 0;

      log("Waiting for a response from higher nodes...\n");

      while (!answered) {
        MPI_Iprobe(MPI_ANY_SOURCE, OK, MPI_COMM_WORLD, &answered, MPI_STATUS_IGNORE);  

        sleep(wait_time);

        waited += wait_time;

        if (waited >= RESPONSE_TIMEOUT)
          break;
      }

      if (answered) {
        MPI_Status status;
        
        MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, OK, MPI_COMM_WORLD, &status);
        
        log("Got an OK answer from node %d\n", status.MPI_SOURCE);
      }

      return answered;
    }

    void broadcast_victory() {
      log("I'm the winner of this election! Broadcasting result...\n");

      for (int process = 0; process < size; process++)
        MPI_Send(NULL, 0, MPI_BYTE, process, RESULT, MPI_COMM_WORLD);
    }

    void wait_for_result() {
      log("Waiting for election result...\n");
      MPI_Status status;

      MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);

      log("I acknowledge that %d is the winner!\n", status.MPI_SOURCE);
    }
};


int main() {
  int required = MPI_THREAD_MULTIPLE;
  int provided;

  MPI_Init_thread(NULL, NULL, required, &provided);

  if (provided < required) {
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

    exit(EXIT_FAILURE);
  }

  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  Node* node = new Node(rank, size);

  node->start();
  node->finish();

  MPI_Finalize();

  return EXIT_SUCCESS;
}
