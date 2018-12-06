# csocket

Port for C's `sys/socket.h` methods for synchronous usage of sockets as file descriptors.

This package uses a native node module so it is compiled on your machine.
For using the module prebuilt for Linux, see [csocket-linux](https://www.npmjs.com/package/csocket-linux).

It is not recommended to make synchronous operations in NodeJS since there is only one thread
and blocking it prevents the event loop from going, and any asynchronous operations will not
be done. See [Switching to normal NodeJS operations](#switching-to-normal-nodejs-operations)
to minimize this risk.

## Usage

Install using:

```bash
npm install --save csocket
```

Then in NodeJS:

```javascript
const csocket = require('csocket');

const PORT = 1234;
const TIMEOUT = 5; // in seconds (allows floating point), undefined/null/0 for no timeout

/* TCP Server */
const BIND_HOST = '127.0.0.1'; // use 0.0.0.0 for all interfaces
const BACKLOG = 5;
let listenerFd = csocket.socket();
csocket.bind(listenerFd, BIND_HOST, PORT);
csocket.listen(listenerFd, BACKLOG);
let socketFd = csocket.accept(listenerFd, TIMEOUT);

/* TCP Client */
const HOST = '127.0.0.1';
let socketFd = csocket.socket();
csocket.connect(socketFd, HOST, PORT);

/* Both */
let bytesSent = csocket.send(socketFd, new Buffer([1, 2, 3]), TIMEOUT);
// bytesSent <= 3
let buffer = new Buffer(4096);
let bytesReceived = csocket.read(socketFd, buffer, TIMEOUT);
// bytesReceived <= 3
// buffer == <Buffer 01 02 03 00 00 00 ... >

/*
 * Always close the file descriptor (not from this library, no "close" in sys/socket.h).
 * The kernel might not immediately close the socket when the process exits,
 *   and NodeJS won't close it during garbage collection.
 */
const fs = require('fs');
fs.close(socketFd);
```

### Switching to normal NodeJS operations

At any point you can use the file descriptor with NodeJS libraries that follow
the asynchronous way NodeJS is supposed to work with, like `fs` and `net`.

```javascript
const csocket = require('csocket');

let socketfd = csocket.socket();

...

const fs = require('fs');
fs.read(socketFd, ...);

const net = require('net');
let socket = new net.Socket({ fd: socketFd });
```
