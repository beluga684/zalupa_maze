#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

struct Player // структура данных о клиенте
{
    std::string name;
    uint8_t C1, C2, C3;
};

std::map <std::string, Player> players; // база игроков

// функция обработки регистрации
void handleRegistration(const std::vector<uint8_t>& packet, int clientId) {
    size_t index = 0;

    // длина имени
    uint8_t nameSize = packet[index];
    index++;

    // имя
    std::string name(packet.begin() + index, packet.begin() + index + nameSize);
    index += nameSize;

    // хар-ки
    uint8_t C1 = packet[index++];
    uint8_t C2 = packet[index++];
    uint8_t C3 = packet[index++];

    // запись в базу
    Player player = {name, C1, C2, C3};
    players[name] = player;

    std::cout << "игрок зареган: " << name
        << " (C1=" << (int)C1
        << ", C2=" << (int)C2
        << ", C3=" << (int)C3 << ")\n";
}

// обработка входящего пакета
void handlePacket(const std::vector<uint8_t>& packet, int clientId) {
    if (packet.empty()) return;

    uint8_t packetType = packet[0];

    switch (packetType)
    {
    case 1:
        handleRegistration(packet, clientId);
        break;
    
    default:
        std::cout << "неизвестный тип пакета: " << (int)packetType << "\n";
    }
}

void initServer() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;

    // создание UDP сокета
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("не удалось создать сокет");
        exit(EXIT_FAILURE);
    }
    std::cout << "сокет создан\n";

    // настройка одреса сервера
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // связка сокета к порту
    if (bind(sockfd, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("не удалось привязать сокет к адресу");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "сервер запущен на порту " << PORT << "\n";

    // цикл обработки запросов
    while (true) {
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(clientAddr);

        // получение данных от клиента
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &len);
        if (n < 0) {
            perror("Ошибка при получении данных");
            continue;
        }
        buffer[n] = '\0';
        std::cout << "получено сообщение: " << buffer << "\n";

        // отправка ответа
        const char* response = "сообщение принято";
        sendto(sockfd, response, strlen(response), 0, (const struct sockaddr*)&clientAddr, len);
        std::cout << "ответ отправлен клиенту\n";
    }

    // закрытие сокета
    close(sockfd);
}

int main() {
    initServer();
    return 0;
}