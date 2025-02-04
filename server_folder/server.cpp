#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <random>

// Параметры сервера
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

// Типы пакетов
#define PACKET_REGISTER 1
#define PACKET_MOVE 2
#define PACKET_VIEW 3

// Направления движения
#define MOVE_UP 1
#define MOVE_DOWN 2
#define MOVE_LEFT 3
#define MOVE_RIGHT 4

// Глобальные переменные
std::map<std::string, std::pair<int, int>> players; // Игроки и их позиции
std::vector<std::pair<int, int>> botPositions;     // Боты и их позиции
std::mutex gameMutex;                              // Мьютекс для синхронизации

std::vector<std::vector<int>> maze;

// Функция генерации лабиринта
void generateMaze(int width, int height, int exitCount) {
    maze = std::vector<std::vector<int>>(height, std::vector<int>(width, 1));

    auto carvePassages = [&](int x, int y, auto&& carvePassagesRef) -> void {
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        std::vector<int> directions = {0, 1, 2, 3};

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(directions.begin(), directions.end(), g);

        for (int i : directions) {
            int nx = x + dx[i] * 2;
            int ny = y + dy[i] * 2;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && maze[ny][nx] == 1) {
                maze[y + dy[i]][x + dx[i]] = 0;
                maze[ny][nx] = 0;

                carvePassagesRef(nx, ny, carvePassagesRef);
            }
        }
    };

    carvePassages(1, 1, carvePassages);

    for (int x = 0; x < width; ++x) {
        maze[0][x] = 1;            // Верхняя граница
        maze[height - 1][x] = 1;   // Нижняя граница
    }
    for (int y = 0; y < height; ++y) {
        maze[y][0] = 1;            // Левая граница
        maze[y][width - 1] = 1;    // Правая граница
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<int> distX(1, width - 2);
    std::uniform_int_distribution<int> distY(1, height - 2);

    for (int i = 0; i < exitCount; ++i) {
        while (true) {
            int x = distX(g);
            int y = distY(g);

            if (maze[y][x] == 0) {
                maze[y][x] = 2;
                break;
            }
        }
    }
}

// Функция регистрации игрока
std::string handleRegistration(const std::vector<uint8_t>& data) {
    uint8_t nameLength = data[1];
    std::string name(data.begin() + 2, data.begin() + 2 + nameLength);

    int x = data[2 + nameLength];
    int y = data[3 + nameLength];
    int С1 = data[4 + nameLength];
    int С2 = data[5 + nameLength];
    int С3 = data[6 + nameLength];

    std::lock_guard<std::mutex> lock(gameMutex);
    if (players.find(name) == players.end()) {
        players[name] = {x, y};
        return "REGISTERED";
    } else {
        return "ALREADY REGISTERED";
    }
}

// Функция обработки движения
std::string handleMovement(const std::vector<uint8_t>& data) {
    uint8_t nameLength = data[1];
    std::string name(data.begin() + 2, data.begin() + 2 + nameLength);
    uint8_t direction = data[2 + nameLength];

    std::lock_guard<std::mutex> lock(gameMutex);
    if (players.find(name) == players.end()) return "NOT REGISTERED";

    int x = players[name].first;
    int y = players[name].second;

    int nx = x, ny = y;
    if (direction == MOVE_UP) ny--;
    else if (direction == MOVE_DOWN) ny++;
    else if (direction == MOVE_LEFT) nx--;
    else if (direction == MOVE_RIGHT) nx++;

    if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10 && maze[ny][nx] == 0) {
        players[name] = {nx, ny};
        return "OK";
    } else {
        return "NOT OK";
    }
}

// Функция обработки обзора
std::string handleView(const std::vector<uint8_t>& data) {
    uint8_t nameLength = data[1];
    std::string name(data.begin() + 2, data.begin() + 2 + nameLength);

    std::lock_guard<std::mutex> lock(gameMutex);
    if (players.find(name) == players.end()) return "NOT REGISTERED";

    int x = players[name].first;
    int y = players[name].second;

    std::string result = "VIEW\n";
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
                result += std::to_string(maze[ny][nx]) + " ";
            } else {
                result += "1 "; // Вне лабиринта
            }
        }
        result += "\n";
    }
    return result;
}

// Регистрация ботов
void registerBots(int botCount) {
    std::srand(std::time(0));
    for (int i = 0; i < botCount; ++i) {
        int x, y;
        do {
            x = std::rand() % 10;
            y = std::rand() % 10;
        } while (maze[y][x] != 0);

        botPositions.push_back({x, y});
        std::cout << "Bot " << i + 1 << " registered at (" << x << ", " << y << ")\n";
    }
}

// Движение ботов
void moveBots() {
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(gameMutex);

        for (auto& bot : botPositions) {
            int x = bot.first;
            int y = bot.second;

            // Если бот уже достиг выхода, пропускаем его
            if (maze[y][x] == 2) {
                continue;
            }

            for (int i = 0; i < 4; ++i) {
                int dir = std::rand() % 4;
                int nx = x + dx[dir];
                int ny = y + dy[dir];

                // Проверяем, можно ли переместиться в новую ячейку
                if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
                    if (maze[ny][nx] == 0) {
                        bot = {nx, ny};
                        break;
                    } else if (maze[ny][nx] == 2) {
                        // Если бот достиг ячейки с выходом, обновляем его положение и останавливаем
                        bot = {nx, ny};
                        break;
                    }
                }
            }
        }
    }
}

// Функция для отображения текущих позиций ботов
void logBotPositions() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Интервал обновления
        std::lock_guard<std::mutex> lock(gameMutex);

        std::cout << "\nТекущее положение ботов:\n";
        for (size_t i = 0; i < botPositions.size(); ++i) {
            std::cout << "Бот " << i + 1 << ": (" << botPositions[i].first << ", " << botPositions[i].second << ")\n";
        }
        std::cout << "---------------------------\n";
    }
}

// Основной сервер
void startServer() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(sockfd, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "Server started on port " << SERVER_PORT << "\n";

    socklen_t len = sizeof(clientAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &len);
        if (n <= 0) continue;

        std::vector<uint8_t> data(buffer, buffer + n);
        std::string response;

        if (data[0] == PACKET_REGISTER) response = handleRegistration(data);
        else if (data[0] == PACKET_MOVE) response = handleMovement(data);
        else if (data[0] == PACKET_VIEW) response = handleView(data);

        sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, len);
    }
}

// Функция для отображения лабиринта в консоль
void printMaze() {
    for (const auto& row : maze) {
        for (int cell : row) {
            if (cell == 1) {
                std::cout << "# "; // Стена
            } else if (cell == 2) {
                std::cout << "E "; // Выход
            } else {
                std::cout << ". "; // Проход
            }
        }
        std::cout << "\n";
    }
}

int main() {
    generateMaze(10, 10, 10);
    printMaze();
    registerBots(3); // Регистрация 3 ботов
    std::thread botThread(moveBots);         // Поток для движения ботов
    std::thread logThread(logBotPositions); // Поток для логирования позиций ботов

    startServer(); // Запуск сервера
    botThread.join();
    logThread.join();

    return 0;
}