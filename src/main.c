#include "../header/traceroute.h"

#define MAX_HOPE    32          // Максимальное количество узлов

int main(int argc, void *argv[])
{
	int                 sock;
	struct sockaddr_in  addr;
	struct icmp_echo    packet;
	u_short             ttl;        // Текущий TTL
	struct timeval      time;       // Время ожидания ответа
	char                ipfrom[16]; // IP промежуточного узла
	int                 reply;      // Тип ответа на посланый запрос
	
	// Проверяем параметры на валидность
	if (argc != 2) {
		printf("Неверно указаны параметры\n");
		exit(-1);
	}
	if(inet_addr(argv[1]) == INADDR_NONE) {
		printf("Некорректно задан адрес\n");
		exit(-1);
	}

	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock == -1) {
		perror("socket");
		exit(-1);
	}
	/*
	 * Заполняем IP назначения, поскольку система сама добавит на наш ICMP 
	 * необходимые заголовки. И задаем таймаут для получения
	 */
	time.tv_sec          = 2;
	time.tv_usec         = 0;
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&time,sizeof(time)) == -1)
		perror("Не удалось настроить таймер. Возможна блокировка");

	//Отправляем эхо запросы, постепенно увеличивая TTL
	for(int ttl = 1; ttl < MAX_HOPE; ttl++) {
		setsockopt(sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
		icmp_build(&packet, 8, 0);
		sendto(sock, (void *)&packet, sizeof(packet), 0,
		       (struct sockaddr *)&addr, sizeof(addr));
		reply = icmp_recv(sock, (char *)argv[1], ipfrom);
		switch(reply) {
			case -1:
				printf("%2d  %s\n", ttl, "* * *");
			break;

			case 0:
				printf("%2d  %s\n", ttl, (char *)argv[1]);
				// Досрочно "выпрыгиваем" из цикла
				ttl = MAX_HOPE;
			break;

			case 1:
				printf("%2d  %s\n", ttl, ipfrom);
			break;
		}
	}
	close(sock);
}