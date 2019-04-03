#include "comm.h"

typedef struct sockfd_node{
  int fd;
//插入和删除
  struct sockfd_node* _prev;
  struct sockfd_node* _next;
}sockfd_node;

sockfd_node* phead = NULL;

sockfd_node* buy_node(int client_fd)
{
  sockfd_node* new_node = (sockfd_node*)malloc(sizeof(sockfd_node));
  assert(new_node != NULL);

  new_node->fd = client_fd;
  new_node->_prev = NULL;
  new_node->_next = NULL;
  return new_node;
}

void push_back(sockfd_node* new_node)
{
  if(phead->_next == NULL){
    phead->_next = new_node;
    phead->_prev = new_node;
    new_node->_next = phead;
    new_node->_prev = phead;
  }else{
    new_node->_prev = phead->_prev;
    new_node->_next = phead;
    phead->_prev->_next = new_node;
    phead->_prev = new_node;
  }
}

// 删除节点
void erase(int sock)
{
  sockfd_node* del = phead->_next;
  while(del != NULL){
    if(del->fd == sock){
      del->_prev->_next = del->_next;
      del->_next->_prev = del->_prev;
      free(del);
      break;
    }
    del = del->_next;
  }
}

// 接收到 Ctrl+C 产生的 信息后，执行此函数
void handler(int n)
{
  sockfd_node* cur = phead->_next;
  while(cur != phead){
    sockfd_node* next = cur->_next;
    free(cur);
    cur = next;
  }
  free(phead);
  printf("服务器退出\n");
  exit(0);
}

void thread_work(int epfd, int sock)
{
  // 储存客户端发来的消息以及客户端的姓名
  Msg msg;
  memset(&msg, 0, sizeof(msg));

   // 读取客户端信息
   int read_size = recv(sock, &msg, sizeof(msg), 0);
   // 读到的字节个数为0，代表客户端断开连接
   if(read_size == 0){
     printf("< < < < < %s 已下线 < < < < <\n", msg.name);
     epoll_ctl(epfd, EPOLL_CTL_DEL, sock,  NULL);
     erase(sock);
     return;
   }else if(read_size < 0){
       perror("recv");
       return;
   }

   printf("%s say : %s\n", msg.name, msg.data);

   // 把收到消息给所有用户发送
   sockfd_node* p = phead->_next;
   while(p != phead){
     if(p->fd != sock){
       send(p->fd, &msg, sizeof(msg), 0);
     }
     p = p->_next;
   }
}

void handlerReadyEvents(int epfd, struct epoll_event revs[], int num, int listen_sock)
{
  int i = 0;
  struct epoll_event ev;

  for(; i < num; i++){
    int fd = revs[i].data.fd;
    uint64_t events = revs[i].events;

    if(listen_sock == fd && (events & EPOLLIN)){
      int sock = accept(fd, NULL, NULL);
      if(sock < 0){
        perror("accept");
        continue;
      }
      printf("get a new client!\n");
      sockfd_node* newNode = buy_node(fd);
      push_back(newNode);
      ev.data.fd = sock;
      ev.events = EPOLLIN;
      epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);
    }
    else if(events & EPOLLIN){
      //read event ready!
      thread_work(epfd, fd);
    }
    else{

    }
  }
}

void server_work(int argc, char* argv[])
{
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0){
    perror("socket");
  }

  // 允许创建多个端口号相同的套接字
  // 解决：如果服务器主动断开连接会进入 TIME_WAIT 状态的问题
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in server_socket;
  server_socket.sin_family = AF_INET;
  server_socket.sin_addr.s_addr = inet_addr(argv[1]);
  server_socket.sin_port = htons(atoi(argv[2]));

  int ret = bind(server_fd, (struct sockaddr*)&server_socket, sizeof(server_socket));
  if(ret < 0){
    perror("bind()");
    close(server_fd);
  }

  if(listen(server_fd, 5) < 0){
    perror("listen");
    close(server_fd);
  }

  printf("bind and listen success, wait accept...\n");
  int epfd = epoll_create(256);
  if(epfd < 0){
    perror("epoll_create");
    return;
  }

  struct epoll_event ev;
  ev.data.fd = server_fd;
  ev.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

  struct epoll_event revs[128];
  for( ; ; ){
    int timeout = -1;
    int num = epoll_wait(epfd, revs, 128, timeout);
    printf("num %d\n", num);
    switch(num){
      case -1:
        perror("epoll_wait");
        break;
      case 0:
        printf("timeout...\n");
        break;
      default:
          handlerReadyEvents(epfd, revs, num, server_fd);
          break;
    }
  }
  close(epfd);
  close(server_fd);
}

int main(int argc, char* argv[])
{
  // 把服务器设置成守护进程
  //daemon(0, 0);
  phead = buy_node(-1);
  if(argc != 3){
    printf("Usage ./server [ip] [port]\n");
    return 1;
  }

  // 当按下 Ctrl+C 时，先销毁链表释放空间，然后服务器退出
  signal(SIGINT, handler);
  server_work(argc, argv);

  return 0;
}
