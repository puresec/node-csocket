# csocket-linux

Port for C's `sys/socket.h` methods for synchronous usage of sockets as file descriptors.

This package uses a native node module that is precompiled for Linux.
To compile the module on installation, see [csocket](https://www.npmjs.com/package/csocket).

It is not recommended to make synchronous operations in NodeJS since there is only one thread
and blocking it prevents the event loop from going, and any asynchronous operations will not
be done. See [Switching to normal NodeJS operations](#switching-to-normal-nodejs-operations)
to minimize this risk.

## Usage

Install using:

```bash
npm install --save csocket-linux
```

Then in NodeJS:

```javascript
const csocket = require('csocket-linux');

const PORT = 1234;
const TIMEOUT = 5000; // in milliseconds (allows floating point), undefined/null/0 for no timeout

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
const net = require('net');
net.createServer().listen({ fd: listenerFd }).close();
new net.Socket({ fd: socketFd }).end();
```

### Switching to normal NodeJS operations

At any point you can use the file descriptor with NodeJS libraries that follow
the asynchronous way NodeJS is supposed to work with, like `fs` and `net`.

```javascript
const csocket = require('csocket-linux');
const net = require('net');

let socketfd = csocket.socket();

...

/* TCP Server */
let server = net.createServer().listen({ fd: socketFd });

/* TCP Client */
let socket = new net.Socket({ fd: socketFd });
```

#### Important note

Once you are done with your synchronous operations (receiving, writing, accepting, etc)
you should always create a `net.Socket`/`net.Server` and let NodeJS handle the file descriptor's lifecycle.

According to my experience, if you don't pass the file descriptor to NodeJS the socket may not
close properly, and may cause a port to stay bound or the process to hang (even if using `fs.close`).

Only do that after you finish using `csocket`, otherwise you may race condition with NodeJS
over accepts/reads/etc.

### Implementation notes

* Currently `csocket.socket()` only creates a socket for TCP/IPv4 (`AF_INET/SOCK_STREAM`). PRs are welcome.
* Not all `sys/socket.h` operations are implemented, only the bare minimum needed to communicate. PRs are welcome.
* Allowing a timeout is actually implemented using the `select`,
  I cannot fathom why other I/O implementations don't expose this functionallity,
  blocking I/O functions are unusuable without it.
