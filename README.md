# Detective Pugs
<div align="center">
    <img src="https://github.com/z-beslaneev/Detective-Pugs/blob/main/assets/pug.png" alt="Puggy" style="display: block; margin: 0 auto;">
    <p align="center">
        A project with a modern approach to backend implementation in C++
    </p>
</div>

## About The Project

This project is a multiplayer game written in C++ and utilizes modern technologies for backend development. 

The goal of the game is to collect as many things as possible and take them back to the base.

### Built With
* ![](https://img.shields.io/badge/C%2B%2B20-%2320232A?style=for-the-badge&logo=cplusplus)
* ![](https://img.shields.io/badge/cmake-%2320232A?style=for-the-badge&logo=cmake)
* ![](https://img.shields.io/badge/conan-%2320232A?style=for-the-badge&logo=conan)
* ![](https://img.shields.io/badge/docker-%2320232A?style=for-the-badge&logo=docker)
* ![](https://img.shields.io/badge/postman-%2320232A?style=for-the-badge&logo=postman)
* ![](https://img.shields.io/badge/postgresql-%2320232A?style=for-the-badge&logo=postgresql)

## Getting Started

### Prerequisites

1. To build and run the server, you need to install [Docker](https://docs.docker.com/engine/install/ubuntu).
2. Deploy postgresql container:
   ```sh
   docker run -d --name postgres-container -p 30432:5432 -e TZ=UTC -e POSTGRES_PASSWORD=Mys3Cr3t ubuntu/postgres:14-22.04_beta
   ```
3. Set postgres URL to env `GAME_DB_URL`. For example `postgres://postgres:Mys3Cr3t@localhost:30432/`


### Build
Go to the root of repository and run the command:
```sh
docker build -t my_http_server .
```

### Run

1. Start the container by making the server available on port 80:
   ```sh
   docker run --rm -p 80:8080 my_http_server
   ```
2. Open local address in browser:
   ```
   127.0.0.1
   ```
<p align="center">
  <img src="https://github.com/z-beslaneev/Detective-Pugs/blob/main/assets/gameplay.gif"/>
</p>


### API Documentation

You can read the API documentation at the [link](https://documenter.getpostman.com/view/2539805/2sA3XTgMDq) or by importing the collection into postman

