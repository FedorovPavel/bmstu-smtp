#include "../common/header.h"
#include "smtp.h"
#include "process.h"

int run_process(struct process *pr) {
	struct mesg_buffer message;

	printf("SMTP run process start");

	int received_bytes_count;				// число прочитанных байт
	char buffer_output[SERVER_BUFFER_SIZE];	// выходной буфер для записи ответа

	char smtp_stub[SERVER_BUFFER_SIZE] = "Hi, you've come to smtp server";

	while (1) {
		printf("PROCESS_RUNS\n");
		sleep(50);
	}

	/*while (1) {
    
    	printf("asd\n");
    	printf("%d\n",pr->sock_list->c_sock.fd);
		struct timeval tv; // timeval используется внутри select для выхода из ожидания по таймауту
		tv.tv_sec = 15;
		tv.tv_usec = 0;

		int fd = pr->sock_list->c_sock.fd;
		printf("%d\n", fd);
		printf("%d\n", pr->max_fd);


		if (pr->sock_list != NULL) {
			struct client_socket_list *p;
    		for (p = pr->sock_list; p != NULL; p = p->next) {
    			FD_SET(p->c_sock.fd, &(pr->socket_set));
    			//printf("%d\n", p->c_sock.fd);
    		}

    		// ожидаем select-ом возможности писать в сокет сервером в течение 15с
			int sock_count = select(pr->max_fd + 1,  &(pr->socket_set), NULL, NULL, &tv);

			printf("sock_count = %d\n", sock_count);
			if (sock_count > 1) {
				return 1;
			}
			
			p = pr->sock_list;
			while (p != NULL) {
				// если так и не дождались возможности писать - ошибка
				if (!FD_ISSET(p->c_sock.fd,  &(pr->socket_set))) {
					sprintf(buffer_output, "socket is not available for 15s\n");
					printf("socket is not available for 15s\n");
					send(p->c_sock.fd, buffer_output, strlen(buffer_output), 0);
					struct client_socket_list *temp = p;
					p->next = temp->next;
					free(temp);
					//FD_CLR(p->c_sock.fd, &(pr->socket_set));
					//close(p->c_sock.fd);
				} else {

					switch (p->c_sock.state) {
						case SOCKET_STATE_SEND_STUB: {
							sprintf(buffer_output, "%s\n", smtp_stub);
							if (send(p->c_sock.fd, buffer_output, strlen(buffer_output), 0) < 1) {
								printf("send_stub");
								return 0;
							} else {
								printf("send_stub_ok");
								p->c_sock.state = SOCKET_STATE_RECEIVE_DATA;
							}
						}
						case SOCKET_STATE_RECEIVE_DATA: {
							// читаем, что прислал клиент, во входной буфер
							received_bytes_count = recv(p->c_sock.fd, p->c_sock.buffer, SERVER_BUFFER_SIZE, 0);
							// считали 0 байт - клиент перестал отправлять данные
							if (received_bytes_count == 0) {
								printf("remote host closed socket %d\n", p->c_sock.fd);
								struct client_socket_list *temp = p;
								p->next = temp->next;
								free(temp);
								return 0;
							}
							if (received_bytes_count < 0) {
								printf("problems with socket %d\n", p->c_sock.fd);
								return 0;
							}
							if (received_bytes_count > 0) {
								printf("%s\n",p->c_sock.buffer);
							}
						
						}
					}
					
				}
				p = p->next;
			}
		}
	}*/
}

void smtp_handler(int *socket_fd, const int pid) {
	printf("SMTP handler start");

	int client_socket_fd = *socket_fd;		// клентский сокет, полученный после select()
	int received_bytes_count;				// число прочитанных байт
	// int i, j;
	char buffer[SERVER_BUFFER_SIZE];		// буфер, в который считываем
	char buffer_output[SERVER_BUFFER_SIZE];	// выходной буфер для записи ответа

	char smtp_stub[SERVER_BUFFER_SIZE] = "Hi, you've come to smtp server";

	sprintf(buffer_output, "%s\n", smtp_stub);
	printf("%s\n", smtp_stub);
	send(client_socket_fd, buffer_output, strlen(buffer_output), 0);

	while (1) {
		fd_set socket_set; // fd_set используется для select - выбор свободного сокета для записи
		struct timeval tv; // timeval используется внутри select для выхода из ожидания по таймауту

		// начальная инициализация socket_set единственным сокетом socket_fd
		FD_ZERO(&socket_set);
		FD_SET(client_socket_fd, &socket_set);
		tv.tv_sec = 15;
		tv.tv_usec = 0;

		// ожидаем select-ом возможности писать в сокет сервером в течение 15с
		int sock_count = select(client_socket_fd + 1, &socket_set, NULL, NULL, &tv);

		printf("%d\n", sock_count);

		// если так и не дождались возможности писать - ошибка
		if (!FD_ISSET(client_socket_fd, &socket_set)) {
			sprintf(buffer_output, "socket is not available for 15s\n");
			printf("socket is not available for 15s\n");
			send(client_socket_fd, buffer_output, strlen(buffer_output), 0);
			break;
		}

		// читаем, что прислал клиент, во входной буфер
		received_bytes_count = recv(client_socket_fd, buffer, SERVER_BUFFER_SIZE, 0);
		// считали 0 байт - клиент перестал отправлять данные
		if (received_bytes_count == 0) {
			printf("remote host closed socket %d\n", client_socket_fd);
			break;
		}
		if (received_bytes_count == -1) {
			printf("problems with socket %d\n", client_socket_fd);
			break;
		}

		char *eol; // признак конца линии обрабатываемой команды
		while (strstr(buffer, "\r\n")) {
			eol = strstr(buffer, "\r\n");
			eol[0] = '\0'; 

			// обработка ключевых слов
			printf("Client: %d, message: %s\n", client_socket_fd, buffer);

			// конец строки для сравнения (потом переделать под разделение на данные и команды)
			buffer[4] = '\0';

			if (STR_EQUAL(buffer, "HELO")) { 
				// начальное приветствие
				sprintf(buffer_output, HEADER_250_OK);
			} else if (STR_EQUAL(buffer, "EHLO")) { 
				// улучшенное начальное приветствие
				sprintf(buffer_output, HEADER_250_OK);
			} else if (STR_EQUAL(buffer, "MAIL")) { 
				// получено новое письмо от
				sprintf(buffer_output, HEADER_250_OK);
			} else if (STR_EQUAL(buffer, "RCPT")) { 
				// письмо направлено ... 
				sprintf(buffer_output, HEADER_250_OK_RECIPIENT);
			} else if (STR_EQUAL(buffer, "DATA")) { 
				// содержимое письма
				sprintf(buffer_output, HEADER_354_CONTINUE);
			} else if (STR_EQUAL(buffer, "RSET")) { 
				// сброс соединения
				sprintf(buffer_output, HEADER_250_OK_RESET);
			} else if (STR_EQUAL(buffer, "NOOP")) { 
				// ничего не делать
				sprintf(buffer_output, HEADER_250_OK_NOOP);
			} else if (STR_EQUAL(buffer, "QUIT")) { 
				// закрыть соединение
				sprintf(buffer_output, HEADER_221_OK);
				printf("Server: %d, message: %s", client_socket_fd, buffer_output);
				send(client_socket_fd, buffer_output, strlen(buffer_output), 0);
				close(client_socket_fd);
				kill(pid, SIGTERM);
			} else { 
				// метод не был определен
				sprintf(buffer_output, HEADER_502_NOT_IMPLEMENTED);
			}
			printf("Server: %d, message: %s", client_socket_fd, buffer_output);
			send(client_socket_fd, buffer_output, strlen(buffer_output), 0);


			// смещаем буфер для обработки следующей команды
			memmove(buffer, eol + 2, SERVER_BUFFER_SIZE - (eol + 2 - buffer));

		}
	}
	// обработка команды кончилась - закрываем сокет
	close(client_socket_fd);
	kill(pid, SIGTERM);
}