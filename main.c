#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROXY "127.0.0.1"
#define PROXYPORT 9050
#define USERNAME "sensei1"

typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;

// Packed structs to prevent padding
struct ProxyRequest {
  int8 vn;
  int8 cd;
  int16 dstport;
  int32 dstip;
  unsigned char userid[8];
} __attribute__((packed));

struct ProxyResponse {
  int8 vn;
  int8 cd;
  int16 _;
  int32 __;
} __attribute__((packed));

typedef struct ProxyRequest Req;
typedef struct ProxyResponse Res;

Req *request(char *host, int port) {
  Req *req = (Req *)malloc(sizeof(Req));
  req->vn = 4;
  req->cd = 1;
  req->dstport = htons(port);
  req->dstip = inet_addr(host);
  strncpy((char *)req->userid, USERNAME, 7);
  req->userid[7] = '\0'; // Null-terminate userid
  return req;
}

char buffer[1024];

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    return -1;
  }

  char *host = argv[1];
  int port = atoi(argv[2]);

  int s = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in sock;
  sock.sin_family = AF_INET;
  sock.sin_addr.s_addr = inet_addr(PROXY);
  sock.sin_port = htons(PROXYPORT);

  if (connect(s, (struct sockaddr *)&sock, sizeof(sock)) < 0) {
    perror("connect");
    close(s);
    return -1;
  }

  Req *req = request(host, port);
  Res *res;

  printf("Connected to Proxy\n");
  if (write(s, req, sizeof(Req)) < 0) { // Use sizeof(Req)
    perror("write");
    free(req);
    close(s);
    return -1;
  }

  printf("Sent request\n");
  memset(buffer, 0, sizeof(buffer));
  int total_read = 0, bytes;
  while (total_read < sizeof(Res)) {
    bytes = read(s, buffer + total_read, sizeof(Res) - total_read);
    if (bytes <= 0) {
      perror("read");
      free(req);
      close(s);
      return -1;
    }
    total_read += bytes;
  }

  res = (Res *)buffer;
  printf("Response: vn=%d, cd=%d\n", res->vn, res->cd);
  if (res->cd != 90) {
    fprintf(stderr, "Proxy error: %d\n", res->cd);
    free(req);
    close(s);
    return -1;
  }

  printf("Connected via proxy\n");

  // Send HTTP request
  const char *http_req = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
  write(s, http_req, strlen(http_req));

  // Read response
  while ((bytes = read(s, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes] = '\0';
    printf("%s", buffer);
  }

  free(req);
  close(s);
  return 0;
}
