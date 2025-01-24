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
std::vector<uint8_t> createRegistrationPacket(const std::string& name, uint8_t C1, uint8_t C2, uint8_t C3) {
    std::vector<uint8_t> packet;
    packet.push_back(1); // Тип пакета
    packet.push_back(name.size()); // Длина имени
    packet.insert(packet.end(), name.begin(), name.end()); // Имя
    packet.push_back(C1);
    packet.push_back(C2);
    packet.push_back(C3);
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
    uint8_t C1 = 5, C2 = 7, C3 = 10;
    std::vector<uint8_t> regPacket = createRegistrationPacket(playerName, C1, C2, C3);
    std::string regResponse = sendPacketAndGetResponse(sockfd, regPacket, serverAddr);

    std::cout << "Тест 1: Регистрация игрока... ";
    assert(regResponse == "REGISTERED"); // Ожидаем ответ REGISTERED
    std::cout << "Пройден.\n";

    // Тест 2: движение (вниз)
    std::vector<uint8_t> moveDown = createMovementPacket(playerName, 2); // Вниз
    std::string moveDownResponse = sendPacketAndGetResponse(sockfd, moveDown, serverAddr);

    std::cout << "Тест 2: Столкновение... ";
    assert(moveDownResponse == "NOT OK"); // Ожидаем ответ NOT OK
    std::cout << "Пройден.\n";

    // Тест 3: Успешное движение (влево)
    std::vector<uint8_t> moveLeft = createMovementPacket(playerName, 4); // Вправо
    std::string moveLeftResponse = sendPacketAndGetResponse(sockfd, moveLeft, serverAddr);

    std::cout << "Тест 3: Успешное движение влево... ";
    assert(moveLeftResponse == "OK"); // Ожидаем ответ OK
    std::cout << "Пройден.\n";

    // Тест 4: Победа (движение к выходу)
    std::vector<uint8_t> moveRight = createMovementPacket(playerName, 2); // Вниз
    std::string moveRightResponse = sendPacketAndGetResponse(sockfd, moveRight, serverAddr);

    std::cout << "Тест 4: Достижение выхода... ";
    assert(moveRightResponse == "WIN"); // Ожидаем ответ WIN
    std::cout << "Пройден.\n";

    // Закрываем сокет
    close(sockfd);
    std::cout << "Все тесты пройдены успешно.\n";

    return 0;
}