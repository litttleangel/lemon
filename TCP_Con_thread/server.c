#include "comm.h"

typedef struct sockfd_node {
	int fd;
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
	if (phead->_next == NULL) {
		phead->_next = new_node;
		phead->_prev = new_node;
		new_node->_next = phead;
		new_node->_prev = phead;
	}
	else {
		new_node->_prev = phead->_prev;
		new_node->_next = phead;
		phead->_prev->_next = new_node;
		phead->_prev = new_node;
	}
}

// ɾ���ڵ�
void erase(sockfd_node* del)
{
	del->_prev->_next = del->_next;
	del->_next->_prev = del->_prev;
	free(del);
}

// ���յ� Ctrl+C ������ ��Ϣ��ִ�д˺���
void handler(int n)
{
	sockfd_node* cur = phead->_next;
	while (cur != phead) {
		sockfd_node* next = cur->_next;
		free(cur);
		cur = next;
	}
	free(phead);
	printf("�������˳�\n");
	exit(0);
}

void* thread_work(void* attr)
{
	sockfd_node* cur = (sockfd_node*)attr;

	// ����ͻ��˷�������Ϣ�Լ��ͻ��˵�����
	Msg msg;
	memset(&msg, 0, sizeof(msg));

	for (;;) {
		// ��ȡ�ͻ�����Ϣ
		int read_size = recv(cur->fd, &msg, sizeof(msg), 0);
		// �������ֽڸ���Ϊ0������ͻ��˶Ͽ�����
		if (read_size == 0) {
			printf("< < < < < %s ������ < < < < <\n", msg.name);
			erase(cur);
			break;
		}
		else {
			if (read_size < 0) {
				perror("recv");
				break;
			}
		}

		printf("%s say : %s\n", msg.name, msg.data);

		// ���յ���Ϣ�������û�����
		sockfd_node* p = phead->_next;
		while (p != phead) {
			if (p != cur) {
				send(p->fd, &msg, sizeof(msg), 0);
			}
			p = p->_next;
		}
	}
}

void server_work(int argc, char* argv[])
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
	}

	// ����������˿ں���ͬ���׽���
	// �������������������Ͽ����ӻ���� TIME_WAIT ״̬������
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in server_socket;
	server_socket.sin_family = AF_INET;
	server_socket.sin_addr.s_addr = inet_addr(argv[1]);
	server_socket.sin_port = htons(atoi(argv[2]));

	int ret = bind(server_fd, (struct sockaddr*)&server_socket, sizeof(server_socket));
	if (ret < 0) {
		perror("bind()");
		close(server_fd);
	}

	if (listen(server_fd, 5) < 0) {
		perror("listen");
		close(server_fd);
	}


	printf("bind and listen success, wait accept...\n");

	for (;;) {
		struct sockaddr_in client_socket;
		socklen_t len = sizeof(client_socket);

		// ��ͻ��˽�������
		int client_fd = accept(server_fd, (struct sockaddr*)&client_socket, &len);
		if (client_fd < 0) {
			continue;
		}
		char* ip = inet_ntoa(client_socket.sin_addr);
		int port = ntohs(client_socket.sin_port);
		printf("get accept, ip : %s, port : %d\n", ip, port);

		// �����ӵĿͻ��˵���Ϣ����������ͳһ����
		sockfd_node* new_node = buy_node(client_fd);
		push_back(new_node);

		pthread_t tid = 0;
		pthread_create(&tid, NULL, thread_work, (void*)new_node);

		// ���߳����óɷ���״̬��ʹ������󱻲���ϵͳ�Զ����գ������������̣߳�
		pthread_detach(tid);
	}
	close(server_fd);
}

int main(int argc, char* argv[])
{
	// �ѷ��������ó��ػ�����
	//daemon(0, 0);
	phead = buy_node(0);
	if (argc != 3) {
		printf("Usage ./server [ip] [port]\n");
		return 1;
	}

	// ������ Ctrl+C ʱ�������������ͷſռ䣬Ȼ��������˳�
	signal(SIGINT, handler);
	server_work(argc, argv);

	return 0;
}
