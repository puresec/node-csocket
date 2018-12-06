const { expect } = require('chai');

const csocket = require('../../src/index');
const fs = require('fs');
const net = require('net');
const getPort = require('get-port');

describe('csocket', function() {
  // can't use this because afterEach doesn't have access to variables
  // set inside the tests
  let closed;

  beforeEach(async function() {
    closed = false;
    this.fd = csocket.socket();
    this.port = await getPort();
  });

  afterEach(function(done) {
    if (closed) return done();
    fs.close(this.fd, done);
  });

  describe('#bind', function() {
    beforeEach(function() {
      csocket.bind(this.fd, '127.0.0.1', this.port);
    });

    it("throws an error on failure", function(done) {
      let secondFd = csocket.socket();
      try {
        expect(() => csocket.bind(secondFd, '127.0.0.1', this.port))
          .to.throw("EADDRINUSE, Address already in use");
      } finally {
        fs.close(secondFd, done);
      }
    });
  });

  describe('#listen', function() {
    beforeEach(function() {
      csocket.bind(this.fd, '127.0.0.1', this.port);
    });

    it("receives connections", function(done) {
      csocket.listen(this.fd, 1);
      net.createConnection(this.port, done)
        .on('error', () => {}); // otherwise net.js crashes for unaccepted closed socket
    });

    it("throws an error on failure", function(done) {
      closed = true;
      fs.close(this.fd, () => {
        expect(() => csocket.listen(this.fd, 1))
          .to.throw("EBADF, Bad file descriptor");
        done();
      });
    });
  });

  describe('#accept', function() {
    beforeEach(function() {
      csocket.bind(this.fd, '127.0.0.1', this.port);
      csocket.listen(this.fd, 1);
    });

    it("returns a closeable file descriptor without timeout", function(done) {
      net.createConnection(this.port, () => {
        let socketFd = csocket.accept(this.fd);
        fs.close(socketFd);
      }).on('end', done);
    });

    it("returns a closeable file descriptor with timeout", function(done) {
      net.createConnection(this.port, () => {
        let socketFd = csocket.accept(this.fd, 0.01);
        fs.close(socketFd);
      }).on('end', done);
    });

    it("times out with no connection", function() {
      expect(() => csocket.accept(this.fd, 0.01))
        .to.throw("timeout");
    });

    it("throws an error on failure", function(done) {
      closed = true;
      fs.close(this.fd, () => {
        expect(() => csocket.accept(this.fd))
          .to.throw("EBADF, Bad file descriptor");
        done();
      });
    });
  });

  describe('#connect', function() {
    it("connects", function(done) {
      let server = net.createServer()
        .listen(this.port, () => {
          csocket.connect(this.fd, '127.0.0.1', this.port);
        })
        .on('connection', (socket) => {
          server.close();
          fs.close(this.fd);
          closed = true;
          socket.on('end', done); // ensuring they are connected by identifying close
        });
    });

    it("throws an error on failure", function() {
      expect(() => csocket.connect(this.fd, '127.0.0.1', this.port))
        .to.throw("ECONNREFUSED, Connection refused");
    });
  });

  describe('#recv', function() {
    beforeEach(function(done) {
      let server = net.createServer()
        .listen(this.port, () => {
          csocket.connect(this.fd, '127.0.0.1', this.port);
        })
        .on('connection', (socket) => {
          server.close();
          this.socket = socket;
          done();
        });
    });

    it("receives without timeout", function(done) {
      this.socket.write(new Buffer([1, 2, 3]), () => {
        let buffer = new Buffer(4);
        expect(csocket.recv(this.fd, buffer))
          .to.equal(3);
        expect(buffer)
          .to.deep.equal(new Buffer([1, 2, 3, 0 /* extra byte */]));
        done();
      });
    });

    it("receives with timeout", function(done) {
      this.socket.write(new Buffer([1, 2, 3]), () => {
        let buffer = new Buffer(4);
        expect(csocket.recv(this.fd, buffer, 0.01))
          .to.equal(3);
        expect(buffer)
          .to.deep.equal(new Buffer([1, 2, 3, 0 /* extra byte */]));
        done();
      });
    });

    it("times out with no data", function() {
      let buffer = new Buffer(4);
      expect(() => csocket.recv(this.fd, buffer, 0.01))
        .to.throw("timeout");
    });

    it("doesn't overflow", function(done) {
      this.socket.write(new Buffer([1, 2, 3]), () => {
        let buffer = new Buffer(2);

        expect(csocket.recv(this.fd, buffer))
          .to.equal(2);
        expect(buffer)
          .to.deep.equal(new Buffer([1, 2]));

        expect(csocket.recv(this.fd, buffer))
          .to.equal(1);
        expect(buffer)
          .to.deep.equal(new Buffer([3, 2]));
        done();
      });
    });

    it("throws an error on failure", function(done) {
      closed = true;
      fs.close(this.fd, () => {
        expect(() => csocket.recv(this.fd, new Buffer(4)))
          .to.throw("EBADF, Bad file descriptor");
        done();
      });
    });
  });

  describe('#send', function() {
    beforeEach(function(done) {
      let server = net.createServer()
        .listen(this.port, () => {
          csocket.connect(this.fd, '127.0.0.1', this.port);
        })
        .on('connection', (socket) => {
          server.close();
          this.socket = socket;
          done();
        });
    });

    it("sends with no timeout", function(done) {
      expect(csocket.send(this.fd, new Buffer([1, 2, 3])))
        .to.equal(3);
      this.socket
        .on('data', function(data) {
          expect(data)
            .to.deep.equal(new Buffer([1, 2, 3]));
          done();
        });
    });

    it("sends with timeout", function(done) {
      expect(csocket.send(this.fd, new Buffer([1, 2, 3]), 0.01))
        .to.equal(3);
      this.socket
        .on('data', function(data) {
          expect(data)
            .to.deep.equal(new Buffer([1, 2, 3]));
          done();
        });
    });

    it("throws an error on failure", function(done) {
      closed = true;
      fs.close(this.fd, () => {
        expect(() => csocket.send(this.fd, new Buffer([1, 2, 3])))
          .to.throw("EBADF, Bad file descriptor");
        done();
      });
    });
  });
});
