#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <mutex>
#include <thread>
#include <set>
#include <vector>
#include <string>

std::mutex connection_mutex;
std::set<int> connections;

void error(std::string msg) {
  std::cerr << msg << std::endl;
  exit(1);
}

void server(int sockfd) {
  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  int newsockfd;

  while (true) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) {
      std::cout << "invalid sock fd: " << newsockfd << std::endl;
      break;
    }
    std::cout << "new connection: " << newsockfd << std::endl;

    std::unique_lock<std::mutex> lock(connection_mutex);
    connections.insert(newsockfd);
    lock.unlock();
  }
}

int main(int argc, char * argv[]) {
  std::cout << "Hello!\n";

  if (argc < 2) {
    error("invalid port number");
  }

  int sockfd, portno = atoi(argv[1]);
  struct sockaddr_in serv_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  listen(sockfd,5);

  std::thread server_thread(server, sockfd);

  char buffer[16];
  int n;
  while ((n = read(0, buffer, 16)) > 0) {
    std::cout << "got data." << std::endl;
    std::vector<int> toremove;

    std::unique_lock<std::mutex> lock(connection_mutex);
    for (auto i = connections.begin(); i != connections.end(); i++) {
      int written = write(*i, buffer, n);
      if (written < n) {
        toremove.push_back(*i);
      }
    }
    for (auto j = toremove.begin(); j != toremove.end(); j++) {
      close(*j);
      connections.erase(*j);
    }
    lock.unlock();
  }

  close(sockfd);
  server_thread.join();

  return 0;
}
