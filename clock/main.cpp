#include <mpi.h>
#include <stdio.h>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

class LamportClock
{
private:
    int time = 0;

public:
    LamportClock(){};

    int setTime(int time)
    {
        this->time = time;

        return this->time;
    }

    int getTime()
    {
        return this->time;
    }

    int tick()
    {
        this->time += 1;

        return this->time;
    };

    void sync(int rcv_clock_time)
    {
        this->time = std::max(this->time, rcv_clock_time) + 1;
    }
};

void log(int rank, const char *fmt, ...)
{
    char *msg;
    va_list args;

    va_start(args, fmt);
    vasprintf(&msg, fmt, args);

    time_t timer = time(NULL);

    printf("node %d @ %.24s: %s\n", rank, ctime(&timer), msg);

    va_end(args);
    free(msg);
}

void tickClock(int range, int sleepSeconds, LamportClock &clock)
{
    for (int i = 0; i < range; i++)
    {
        sleep(sleepSeconds);

        clock.tick();
    }
}

int main()
{
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    LamportClock clock;

    srand(time(NULL) * world_rank);

    int initialClockTime = rand() % 20;
    int clockTickFrequency = rand() % 2 + 1;

    clock.setTime(initialClockTime);

    int currentClockTime = clock.getTime();

    log(world_rank, "\nSending clock time of %d\n", currentClockTime, world_rank);

    // We are considering that the world has 6 nodes
    int sendTo[] = {3, 4, 5, 0, 1, 2};
    int partner_rank = sendTo[world_rank];

    MPI_Send(&currentClockTime, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);

    tickClock(2, clockTickFrequency, clock);

    int timeSync;
    MPI_Status status;
    MPI_Recv(&timeSync, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

    int timeBeforeSync = clock.getTime();

    clock.sync(timeSync);

    log(world_rank, "Received clock time %d from node %d.\nClock time before sync: %d\nClock time after sync: %d\n\n",
        timeSync,
        status.MPI_SOURCE,
        timeBeforeSync,
        clock.getTime());

    MPI_Finalize();
};
