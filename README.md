# Distributed System Algorithms

In this repository, we show the implementation of three algorithms commonly
known in the field of distributed systems, which aims at

- defining a logical clock;
- implementing mutual exclusion;
- electing a leader.

## Executing the algorithms

In order to execute the algorithms, you should have [Docker][docker], [Docker
Compose][docker-compose] and [Make][make] installed in your machine. Please,
refer to their documentation for the installation guide.

After that, all you need to do is define the algorithm you are willing to run by
the environment variable `ALGORITHM` and do a `make`, for instance

```bash
export ALGORITHM=clock
make
```

You should see the docker image `node` being created and a docker composer set
of instances getting up. Afterwards, a `mpirun` execution will take place for
the specified algorithm.

[docker]: https://docs.docker.com/get-docker/
[docker-compose]: https://docs.docker.com/compose/install/
[make]: https://www.gnu.org/software/make/
