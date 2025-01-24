#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

const int MAZE_WIDTH = 10;
const int MAZE_HEIGHT = 10;

// карта
int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

struct Player // структура данных о клиенте
{
    std::string name;
    uint8_t C1, C2, C3;
    int x, y;
};

std::map <std::string, Player> players; // база игроков

// проверка ячейки
bool canMove(int x, int y) {
    return x >= 0 and x < MAZE_WIDTH and y>= 0 and y < MAZE_HEIGHT and maze[x][y] == 0;
}

// функция обработки регистрации
void handleRegistration(const std::vector<uint8_t>& packet) {
    size_t index = 1;

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

    // стартовая позиция
    int startX = 1, startY = 1;

    if (players.find(name) != players.end()) {
        std::cout << "имя " << name << "занято \n";
        return;
    }

    // запись в базу
    Player player = {name, C1, C2, C3, startX, startY};
    players[name] = player;

    std::cout << "игрок зареган: " << name
        << " (C1=" << (int)C1
        << ", C2=" << (int)C2
        << ", C3=" << (int)C3
        << " (x=" << startX
        << ", y=" << startY
        << ")\n";
}

// обработка движения
void handleMovement(const std::vector<uint8_t>& packet) {
    size_t index = 1;

    // имя
    uint8_t nameSize = packet[index];
    index++;
    std::string name(packet.begin() + index, packet.begin() + index + nameSize);
    index += nameSize;

    // направление
    uint8_t direction = packet[index];

    // поиск игрока
    auto it = players.find(name);
    if (it == players.end()) {
        std::cout << "игрок " << name << " не найден!\n";
        return;
    }

    Player& player = it->second;

    // вычисление новой позиции
    int newX = player.x, newY = player.y;
    switch (direction) {
        case 1: newY--; break; // вверх
        case 2: newY++; break; // вниз
        case 3: newX--; break; // влево
        case 4: newX++; break; // вправо
        default: std::cout << "неверное направление: " << (int)direction << "\n"; return;
    }

    // проверка возможности движения
    if (canMove(newX, newY)) {
        player.x = newX;
        player.y = newY;
        std::cout << "игрок " << name << " переместился на (" << newX << ", " << newY << ")\n";
    } else {
        std::cout << "игрок " << name << " не может двигаться в (" << newX << ", " << newY << ")\n";
    }
}

// обработка входящего пакета
void handlePacket(const std::vector<uint8_t>& packet) {
    if (packet.empty()) return;

    uint8_t packetType = packet[0];

    switch (packetType)
    {
    case 1: // регистрация
        handleRegistration(packet);
        break;
    case 2:
        handleMovement(packet);
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
            perror("ошибка при получении данных");
            continue;
        }

        std::vector<uint8_t> packet(buffer, buffer + n);
        handlePacket(packet);

        // отправка ответа
        const char* response = "пакет обработан!";
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&clientAddr, len);
    }

    // закрытие сокета
    close(sockfd);
}

int main() {
    initServer();
    return 0;
}