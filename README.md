# ECEWO MODULES

A collection of the modules written for [ecewo](https://github.com/savashn/ecewo).

## Available Modules

| Module                            | Description                                |
|-----------------------------------|--------------------------------------------|
| [ecewo-cluster](./src/cluster/)   | Multi-process clustering                   |
| [ecewo-cookie](./src/cookie)      | Cookie parsing and management              |
| [ecewo-cors](./src/cors)          | Cross-Origin Resource Sharing (CORS)       |
| [ecewo-fs](./src/fs)              | Async file system operations               |
| [ecewo-helmet](./src/helmet)      | Security headers middleware                |
| [ecewo-postgres](./src/postgres/) | Async PostgreSQL integration               |
| [ecewo-session](./src/session)    | Session management with in-memory storage  |
| [ecewo-static](./src/static)      | Static file serving with security features |

## Running Test

1. Clone the repo:

```shell
git clone https://github.com/savashn/ecewo-modules.git
```

2. Create a build directory:

```shell
mkdir build && cd build
```

3. Compile the program:

```shell
cmake ..
cmake --build .
```

4. Run tests:

```shell
./modules_test
```
