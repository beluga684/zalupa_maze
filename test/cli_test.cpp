#include <iostream>
#include <vector>
#include <string>
#include <cstring>    // memset
#include <arpa/inet.h> // для работы с сокетами (Linux/Unix)
#include <unistd.h>    // для close()
#include <cassert>     // для assert()

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

// Функция для формирования пакета регистрации
std::vector<uint8_t> createRegistrationPacket(const std::string& name) {
    std::vector<uint8_t> packet;
    packet.push_back(1); // Тип пакета
    packet.push_back(name.size()); // Длина имени
    packet.insert(packet.end(), name.begin(), name.end()); // Имя
    return packet;
}

// Функция для формирования пакета движения
std::vector<uint8_t> createMovementPacket(const std::string& name, uint8_t direction) {
    std::vector<uint8_t> packet;
    packet.push_back(2); // Тип пакета
    packet.push_back(name.size()); // Длина имени
    packet.insert(packet.end(), name.begin(), name.end()); // Имя
    packet.push_back(direction); // Направление
    return packet;
}

// Функция для формирования пакета обзора
std::vector<uint8_t> createViewPacket(const std::string& name) {
    std::vector<uint8_t> packet;
    packet.push_back(3); // Тип пакета
    packet.push_back(name.size()); // Длина имени
    packet.insert(packet.end(), name.begin(), name.end()); // Имя
    return packet;
}

// Функция отправки пакета и получения ответа
std::string sendPacketAndGetResponse(int sockfd, const std::vector<uint8_t>& packet, struct sockaddr_in& serverAddr) {
    socklen_t len = sizeof(serverAddr);
    char buffer[1024];

    // Отправляем пакет
    if (sendto(sockfd, packet.data(), packet.size(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Ошибка при отправке пакета");
        exit(EXIT_FAILURE);
    }

    // Получаем ответ
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&serverAddr, &len);
    if (n < 0) {
        perror("Ошибка при получении ответа");
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';
    return std::string(buffer);
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;

    // Создаем сокет
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Не удалось создать сокет");
        return -1;
    }

    // Настраиваем адрес сервера
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Неверный адрес сервера");
        close(sockfd);
        return -1;
    }

    // Регистрация двух игроков
    std::string player1 = "Player1";
    std::string player2 = "Player2";

    std::cout << "Регистрируем игроков...\n";
    std::vector<uint8_t> regPacket1 = createRegistrationPacket(player1);
    std::vector<uint8_t> regPacket2 = createRegistrationPacket(player2);

    std::string regResponse1 = sendPacketAndGetResponse(sockfd, regPacket1, serverAddr);
    std::string regResponse2 = sendPacketAndGetResponse(sockfd, regPacket2, serverAddr);

    assert(regResponse1 == "REGISTERED");
    assert(regResponse2 == "REGISTERED");
    std::cout << "Игроки зарегистрированы успешно!\n";

    // Игрок 1: движение вверх
    std::cout << "Игрок 1 движется вверх...\n";
    std::vector<uint8_t> moveUp1 = createMovementPacket(player1, 0); // Вверх
    std::string moveResponse1 = sendPacketAndGetResponse(sockfd, moveUp1, serverAddr);
    std::cout << "Ответ сервера: " << moveResponse1 << "\n";

    // Игрок 2: движение вправо
    std::cout << "Игрок 2 движется вправо...\n";
    std::vector<uint8_t> moveRight2 = createMovementPacket(player2, 3); // Вправо
    std::string moveResponse2 = sendPacketAndGetResponse(sockfd, moveRight2, serverAddr);
    std::cout << "Ответ сервера: " << moveResponse2 << "\n";

    // Игрок 1: запрос обзора
    std::cout << "Игрок 1 запрашивает обзор...\n";
    std::vector<uint8_t> viewPacket1 = createViewPacket(player1);
    std::string viewResponse1 = sendPacketAndGetResponse(sockfd, viewPacket1, serverAddr);
    std::cout << "Обзор для игрока 1:\n" << viewResponse1 << "\n";

    // Игрок 2: запрос обзора
    std::cout << "Игрок 2 запрашивает обзор...\n";
    std::vector<uint8_t> viewPacket2 = createViewPacket(player2);
    std::string viewResponse2 = sendPacketAndGetResponse(sockfd, viewPacket2, serverAddr);
    std::cout << "Обзор для игрока 2:\n" << viewResponse2 << "\n";

    // Закрываем сокет
    close(sockfd);
    std::cout << "Тестирование завершено успешно.\n";

    return 0;
}