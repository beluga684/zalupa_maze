#include <iostream>
#include <vector>
#include <string>
#include <cstring>  
#include <arpa/inet.h>
#include <unistd.h> 

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

// функция для формирования пакета регистрации
std::vector<uint8_t> createRegistrationPacket(const std::string& name, uint8_t C1, uint8_t C2, uint8_t C3) {
    std::vector<uint8_t> packet;

    packet.push_back(1);

    packet.push_back(name.size());

    packet.insert(packet.end(), name.begin(), name.end());

    packet.push_back(C1);
    packet.push_back(C2);
    packet.push_back(C3);

    return packet;
}

// функция для формирования пакета движения
std::vector<uint8_t> createMovementPacket(const std::string& name, uint8_t direction) {
    std::vector<uint8_t> packet;

    packet.push_back(2);

    packet.push_back(name.size());

    packet.insert(packet.end(), name.begin(), name.end());

    packet.push_back(direction);

    return packet;
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Не удалось создать сокет");
        return -1;
    }
    std::cout << "Сокет клиента создан.\n";

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Неверный адрес сервера");
        close(sockfd);
        return -1;
    }

    std::string playerName = "Player1";
    uint8_t C1 = 5, C2 = 7, C3 = 10;
    std::vector<uint8_t> regPacket = createRegistrationPacket(playerName, C1, C2, C3);

    if (sendto(sockfd, regPacket.data(), regPacket.size(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Ошибка при отправке пакета регистрации");
        close(sockfd);
        return -1;
    }
    std::cout << "Пакет регистрации отправлен серверу.\n";

    char buffer[1024];
    socklen_t len = sizeof(serverAddr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&serverAddr, &len);
    if (n < 0) {
        perror("Ошибка при получении ответа на регистрацию");
        close(sockfd);
        return -1;
    }
    buffer[n] = '\0';
    std::cout << "Ответ сервера на регистрацию: " << buffer << "\n";

    std::vector<uint8_t> directions = {1, 2, 3, 4}; // Вверх, вниз, влево, вправо
    for (uint8_t direction : directions) {
        std::vector<uint8_t> movePacket = createMovementPacket(playerName, direction);

        if (sendto(sockfd, movePacket.data(), movePacket.size(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Ошибка при отправке пакета движения");
            close(sockfd);
            return -1;
        }
        std::cout << "Пакет движения (" << (int)direction << ") отправлен серверу.\n";

        n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&serverAddr, &len);
        if (n < 0) {
            perror("Ошибка при получении ответа на движение");
            close(sockfd);
            return -1;
        }
        buffer[n] = '\0';
        std::cout << "Ответ сервера на движение: " << buffer << "\n";
    }

    close(sockfd);
    return 0;
}