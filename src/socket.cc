#include <nan.h>
extern "C" {
  #include <errno.h>
  #include <netinet/in.h>
  #include <stdio.h>
  #include <sys/socket.h>
  #include <sys/types.h>
}

/*
 * Helpers
 */

sockaddr_in sockaddr_from_host_and_port(char* host, int port) {
  struct sockaddr_in addr;
  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  if (!inet_pton(AF_INET, host, &(addr.sin_addr))) {
    Nan::ThrowError(Nan::ErrnoException(errno, "inet_pton", strerror(errno)));
  }
  addr.sin_port = htons(port);
  return addr;
}

/*
 * Exported
 */

// socket() => fd
NAN_METHOD(socket) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    Nan::ThrowError(Nan::ErrnoException(errno, "socket", strerror(errno)));
    return;
  }
  info.GetReturnValue().Set(fd);
}

// bind(fd, host, port)
NAN_METHOD(bind) {
  int fd = info[0]->ToInteger()->Value();
  Nan::Utf8String host(info[1]->ToString());
  int port = info[2]->ToInteger()->Value();

  sockaddr_in addr = sockaddr_from_host_and_port(*host, port);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    Nan::ThrowError(Nan::ErrnoException(errno, "bind", strerror(errno)));
  }
}

// listen(fd, backlog)
NAN_METHOD(listen) {
  int fd = info[0]->ToInteger()->Value();
  int backlog = info[1]->ToInteger()->Value();

  if (listen(fd, backlog) < 0) {
    Nan::ThrowError(Nan::ErrnoException(errno, "listen", strerror(errno)));
  }
}

// accept(fd, timeout) => fd
NAN_METHOD(accept) {
  int fd = info[0]->ToInteger()->Value();
  struct timeval tv;
  if (info[1]->IsNumber()) {
    tv.tv_sec = info[1]->ToNumber()->Value();
    tv.tv_usec = (info[1]->ToNumber()->Value() - (long)info[1]->ToNumber()->Value()) * 1000000;
  } else {
    tv.tv_sec = tv.tv_usec = 0;
  }

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  if ((!tv.tv_sec && !tv.tv_usec) || select(fd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv) > 0) {
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
      Nan::ThrowError(Nan::ErrnoException(errno, "accept", strerror(errno)));
      return;
    }
    info.GetReturnValue().Set(client_fd);
  } else {
    Nan::ThrowError("timeout");
  }
}

// connect(fd, host, port)
NAN_METHOD(connect) {
  int fd = info[0]->ToInteger()->Value();
  Nan::Utf8String host(info[1]->ToString());
  unsigned short port = info[2]->ToInteger()->Value();

  sockaddr_in addr = sockaddr_from_host_and_port(*host, port);
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    Nan::ThrowError(Nan::ErrnoException(errno, "connect", strerror(errno)));
  }
}

// recv(fd, buffer, timeout) => bytes received
NAN_METHOD(recv) {
  int fd = info[0]->ToInteger()->Value();
  char* buffer = node::Buffer::Data(info[1]->ToObject());
  ssize_t length = node::Buffer::Length(info[1]->ToObject());
  struct timeval tv;
  if (info[2]->IsNumber()) {
    tv.tv_sec = info[2]->ToNumber()->Value();
    tv.tv_usec = (info[2]->ToNumber()->Value() - (long)info[2]->ToNumber()->Value()) * 1000000;
  } else {
    tv.tv_sec = tv.tv_usec = 0;
  }

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  if ((!tv.tv_sec && !tv.tv_usec) || select(fd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv) > 0) {
    ssize_t bytes_received = recv(fd, buffer, length, 0);
    if (bytes_received < 0) {
      Nan::ThrowError(Nan::ErrnoException(errno, "recv", strerror(errno)));
      return;
    }
    info.GetReturnValue().Set((uint32_t)bytes_received);
  } else {
    Nan::ThrowError("timeout");
  }
}

// send(fd, buffer, timeout) => bytes sent
NAN_METHOD(send) {
  int fd = info[0]->ToInteger()->Value();
  char* buffer = node::Buffer::Data(info[1]->ToObject());
  ssize_t length = node::Buffer::Length(info[1]->ToObject());
  struct timeval tv;
  if (info[2]->IsNumber()) {
    tv.tv_sec = info[2]->ToNumber()->Value();
    tv.tv_usec = (info[2]->ToNumber()->Value() - (long)info[2]->ToNumber()->Value()) * 1000000;
  } else {
    tv.tv_sec = tv.tv_usec = 0;
  }

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  if ((!tv.tv_sec && !tv.tv_usec) || select(fd + 1, (fd_set *) 0, &rfds, (fd_set *) 0, &tv) > 0) {
    ssize_t bytes_sent = send(fd, buffer, length, 0);
    if (bytes_sent < 0) {
      Nan::ThrowError(Nan::ErrnoException(errno, "send", strerror(errno)));
      return;
    }
    info.GetReturnValue().Set((uint32_t)bytes_sent);
  } else {
    Nan::ThrowError("timeout");
  }
}

NAN_MODULE_INIT(Initialize) {
  NAN_EXPORT(target, socket);
  NAN_EXPORT(target, bind);
  NAN_EXPORT(target, listen);
  NAN_EXPORT(target, accept);
  NAN_EXPORT(target, connect);
  NAN_EXPORT(target, recv);
  NAN_EXPORT(target, send);
}

NODE_MODULE(socketfd, Initialize)
