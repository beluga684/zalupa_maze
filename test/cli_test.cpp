#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

// формируем пакет
std::vector<uint8_t> createRegistrationPacket(const std::string& name, uint8_t C1, uint8_t C2, uint8_t C3) {
    std::vector<uint8_t> packet;

    // тип пакета
    packet.push_back(1);

    // размер имени
    packet.push_back(name.size());

    // имя 
    packet.insert(packet.end(), name.begin(), name.end());

    // хар-ки
    packet.push_back(C1);
    packet.push_back(C2);
    packet.push_back(C3);

    return packet;
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;

    // создание сокета
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("не удалось создать сокет");
        return -1;
    }
    std::cout << "сокет клиента создан\n";

    // настройка адреса сервера
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("неверный адрес сервера");
        close(sockfd);
        return -1;
    }

    // формируем пакет для регистрации
    std::string name = "Олег";
    uint8_t C1 = 5, C2 = 7, C3 = 10;
    std::vector<uint8_t> packet = createRegistrationPacket(name, C1, C2, C3);

    // отправка пакета
    if (sendto(sockfd, packet.data(), packet.size(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("ошибка при отправке данных");
        close(sockfd);
        return -1;
    }
    std::cout << "пакет отправлен\n";

    // ожидание ответа
    char buffer[1024];
    socklen_t len = sizeof(serverAddr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&serverAddr, &len);
    if (n < 0) {
        perror("ошибка при получении ответа");
        close(sockfd);
        return -1;
    }

    // вывод ответа
    buffer[n] = '\0';
    std::cout << "ответ от сервера: " << buffer << "\n";

    // закрытие сокета
    close(sockfd);
    return 0;
}