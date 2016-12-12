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

#include <boost/program_options.hpp>
namespace po = boost::program_options;

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
      // std::cout << "invalid sock fd: " << newsockfd << std::endl;
      break;
    }
    std::cout << "new connection: " << newsockfd << std::endl;

    std::unique_lock<std::mutex> lock(connection_mutex);
    connections.insert(newsockfd);
    lock.unlock();
  }
}

void handle_interrupt(int sig) {
  close(0);
}

int main(int argc, char * argv[]) {
  signal(SIGINT, handle_interrupt);

  int portno;
  size_t buffer_size;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("port,p", po::value(&portno)->default_value(2222), "port to listen for connections")
    ("buffer-size,b", po::value(&buffer_size)->default_value(2048), "size of temporary buffer")
    ("help,h", "print help")
  ;

  po::positional_options_description p;
  p.add("port", -1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 0;
  }

  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

  char * buffer = new char[buffer_size];

  int n;
  while ((n = read(0, buffer, buffer_size)) > 0) {
    std::vector<int> toremove;

    std::unique_lock<std::mutex> lock(connection_mutex);
    for (auto i = connections.begin(); i != connections.end(); i++) {
      int written = write(*i, buffer, n);
      if (written < n) {
        toremove.push_back(*i);
      }
    }
    for (auto j = toremove.begin(); j != toremove.end(); j++) {
      std::cout << "connection closing: " << *j << std::endl;
      close(*j);
      connections.erase(*j);
    }
    lock.unlock();
  }

  std::cout << "closing socket" << std::endl;
  close(sockfd);
  server_thread.join();

  delete [] buffer;

  return 0;
}
