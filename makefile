.PYONY: all build run setup

SHELL=/bin/bash
IMAGE_NAME=node
ALGORITHM=hello

all: build setup run

build: Dockerfile
	docker build -t $(IMAGE_NAME) .

setup:
	docker-compose down -v && docker-compose up --scale worker=2 -d
	docker exec -it master setup/config-ssh.sh

run:
	docker exec -it master mpirun -v -n 6 --hostfile /hosts/names ./$(ALGORITHM)/main
