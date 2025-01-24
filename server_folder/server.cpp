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
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 2},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

struct Player // структура данных о клиенте
{
    std::string name;
    uint8_t C1, C2, C3;
    int x, y;
};

// база игроков
std::map <std::string, Player> players;

// проверка ячейки
bool canMove(int x, int y) {
    return x >= 0 and x < MAZE_WIDTH and y>= 0 and y < MAZE_HEIGHT and maze[x][y] != 1;
}

// функция обработки обзора
std::string handleView(const std::vector<uint8_t>& packet) {
    size_t index = 1;

    uint8_t nameSize = packet[index];
    index++;
    std::string name(packet.begin() + index, packet.begin() + index + nameSize);
    index += nameSize;

    auto it = players.find(name);
    if (it == players.end()) {
        std::cout << "игрок " << name << " не найден!\n";
        return "ERROR";
    }

    Player player = it->second;

    // определение границ обзора
    int startX = std::max(0, player.x-1);
    int endX = std::min(MAZE_WIDTH - 1, player.x + 1);
    int startY = std::max(0, player.y - 1);
    int endY = std::min(MAZE_HEIGHT - 1, player.y + 1);

    // формирование ответа
    std::string response;
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++)
            response += std::to_string(maze[y][x]) + " ";
        response.pop_back();
        response += "\n";
    }

    return response;
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
    int startX = 7, startY = 8;

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
std::string handleMovement(const std::vector<uint8_t>& packet) {
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
        return "ERROR";
    }

    Player& player = it->second;

    // вычисление новой позиции
    int newX = player.x, newY = player.y;
    switch (direction) {
        case 1: newY--; break; // вверх
        case 2: newY++; break; // вниз
        case 3: newX--; break; // влево
        case 4: newX++; break; // вправо
        default: std::cout << "неверное направление: " << (int)direction << "\n"; return "ERROR";
    }

    // проверка возможности движения
    if (canMove(newX, newY)) {
        player.x = newX;
        player.y = newY;
        
        if (maze[newX][newY] == 2) {
            std::cout << "игрок " << name << " вышел \n";
            return "WIN";
        }

        std::cout << "игрок " << name << " переместился на (" << newX << ", " << newY << ")\n";
        return "OK";
    } else {
        std::cout << "игрок " << name << " не может двигаться в (" << newX << ", " << newY << ")\n";
        return "NOT OK";
    }
}

// обработка входящего пакета
void handlePacket(const std::vector<uint8_t>& packet, struct sockaddr_in& clientAddr, int sockfd) {
    if (packet.empty()) return;

    uint8_t packetType = packet[0];
    std::string response;

    switch (packetType) {
        case 1: // регистрация
            handleRegistration(packet);
            response = "REGISTERED";
            break;
        case 2: // движение
            response = handleMovement(packet);
            break;
        case 3: // обзор
            response = handleView(packet);
            break;
        default: // неизвестный тип
            std::cout << "неизвестный тип пакета: " << (int)packetType << "\n";
            response = "UNKNOWN";
    }

    // отправка ответа
    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

void initServer() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Не удалось создать сокет");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Не удалось привязать сокет к адресу");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Сервер запущен на порту " << PORT << ".\n";

    while (true) {
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(clientAddr);

        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &len);
        if (n < 0) {
            perror("Ошибка при получении данных");
            continue;
        }

        std::vector<uint8_t> packet(buffer, buffer + n);
        handlePacket(packet, clientAddr, sockfd);
    }

    close(sockfd);
}

int main() {
    initServer();
    return 0;
}