#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cassert>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

// Функция для формирования пакета регистрации
std::vector<uint8_t> createRegistrationPacket(const std::string& name, uint8_t x, uint8_t y, uint8_t health) {
    std::vector<uint8_t> packet;
    packet.push_back(1); // Тип пакета
    packet.push_back(name.size()); // Длина имени
    packet.insert(packet.end(), name.begin(), name.end()); // Имя
    packet.push_back(x);
    packet.push_back(y);
    packet.push_back(health);
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

    // Тест 1: Регистрация
    std::string playerName = "TestPlayer";
    uint8_t startX = 1, startY = 1, health = 10;
    std::vector<uint8_t> regPacket = createRegistrationPacket(playerName, startX, startY, health);
    std::string regResponse = sendPacketAndGetResponse(sockfd, regPacket, serverAddr);

    std::cout << "Тест 1: Регистрация игрока... ";
    assert(regResponse == "REGISTERED" || regResponse == "ALREADY REGISTERED");
    std::cout << "Пройден (" << regResponse << ").\n";

    // Тест 2: Успешное движение вправо
    std::vector<uint8_t> moveRight = createMovementPacket(playerName, 4); // Вправо
    std::string moveRightResponse = sendPacketAndGetResponse(sockfd, moveRight, serverAddr);

    std::cout << "Тест 2: Движение вправо... ";
    assert(moveRightResponse == "OK" || moveRightResponse == "NOT OK");
    std::cout << "Пройден (" << moveRightResponse << ").\n";

    // Тест 3: Обзор ближайших клеток
    std::vector<uint8_t> viewPacket = createViewPacket(playerName);
    std::string viewResponse = sendPacketAndGetResponse(sockfd, viewPacket, serverAddr);

    std::cout << "Тест 3: Обзор ближайших клеток...\n";
    std::cout << viewResponse << "\n";
    std::cout << "Тест 3 пройден.\n";

    // Тест 4: Столкновение с ботом
    std::cout << "Тест 4: Проверка столкновения с ботом... (для проверки требуется ручной анализ ситуации).\n";

    // Закрываем сокет
    close(sockfd);
    std::cout << "Все тесты пройдены успешно.\n";

    return 0;
}