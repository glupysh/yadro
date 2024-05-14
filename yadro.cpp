#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <regex>

struct Event {
    std::string time;
    int id;
    std::string client;
    int table;
    std::string error;
};

struct Table {
    int id;
    int time;
    int revenue;
    bool isBusy;
    std::string tmp_time;
};

struct Client {
    std::string name;
    int table;
};

// Проверка корректности входных данных
bool checkFileFormat(const std::string& filename) {
    std::string line;
    int lineNumber = 1;
    std::ifstream inputFile(filename);

    // Регулярные выражения для проверки формата 
    std::regex tablesFormat("^\\d+$");
    std::regex timeFormat("^\\d{2}:\\d{2} \\d{2}:\\d{2}$");
    std::regex costFormat("^\\d+$");
    std::regex eventFormat("^\\d{2}:\\d{2} \\d+ [a-zA-Z0-9_-]+( \\d+)?$");

    // Основной цикл обработки строк
    while (std::getline(inputFile, line)) {
        if (line.empty())
            continue;
        if (lineNumber == 1) {
            if (!std::regex_match(line, tablesFormat)) {
                std::cerr << line << std::endl;
                return false;
            }
        } else if (lineNumber == 2) {
            if (!std::regex_match(line, timeFormat)) {
                std::cerr << line << std::endl;
                return false;
            }
        } else if (lineNumber == 3) {
            if (!std::regex_match(line, costFormat)) {
                std::cerr << line << std::endl;
                return false;
            }
        } else {
            if (!std::regex_match(line, eventFormat)) {
                std::cerr << line << std::endl;
                return false;
            }
        }
        lineNumber++;
    }
    return true;
}

// Проверка времени события на часы работы клуба
bool isOpen(const std::string& time, const std::string& startTime) {
    int h1, m1, h2, m2;
    sscanf(time.c_str(), "%d:%d", &h1, &m1);
    sscanf(startTime.c_str(), "%d:%d", &h2, &m2);
    return (h1 > h2 || (h1 == h2 && m1 >= m2));
}

// Подсчет времени и выручки для стола
void calculate(Table& table, std::string& endTime, int costPerHour) {
    int h1, m1, h2, m2, total_minutes, cost;
    sscanf(table.tmp_time.c_str(), "%d:%d", &h1, &m1);
    sscanf(endTime.c_str(), "%d:%d", &h2, &m2);
    total_minutes = (h2 - h1) * 60 + (m2 - m1);
    if (total_minutes < 0)
        total_minutes += 24 * 60;
    if (total_minutes % 60 == 0)
        cost = costPerHour * (total_minutes / 60);
    else
        cost = costPerHour * (total_minutes / 60 + 1);
    table.time += total_minutes;
    table.revenue += cost;
}

// Главная функция программы
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 2;
    }
    if (!checkFileFormat(argv[1])) return 3;

    int numTables;
    std::string startTime, endTime;
    int costPerHour;
    input >> numTables;
    input >> startTime >> endTime;
    input >> costPerHour;

    std::queue<std::string> queue;
    std::vector<Event> events;
    std::vector<Client> clients;
    std::vector<Table> tables(numTables);
    for (auto& table : tables) {
        table.id = &table - &tables[0] + 1;
        table.time = 0;
        table.revenue = 0;
        table.isBusy = false;
    }

    // Основной цикл обработки событий
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty())
            continue;
        Event event;
        std::istringstream iss(line);
        iss >> event.time >> event.id >> event.client >> event.table;

        // Определение возможных ошибок в событии
        if (!isOpen(event.time, startTime)) event.error = "NotOpenYet";
        else if (event.id == 1 && (std::find_if(clients.begin(), clients.end(), [&event](const Client& c) { return c.name == event.client; })) != clients.end())
            event.error = "YouShallNotPass";
        else if (event.id == 2) {
            if (std::find_if(clients.begin(), clients.end(), [&event](const Client& c) { return c.name == event.client; }) == clients.end())
                event.error = "ClientUnknown";
            if (std::find_if(tables.begin(), tables.end(), [&event](const Table& t) { return t.id == event.table; }) -> isBusy)
                event.error = "PlaceIsBusy";
        }
        else if ((event.id == 3) && (std::count_if(tables.begin(), tables.end(), [](const Table& t) { return t.isBusy; }) < numTables))
            event.error = "ICanWaitNoLonger!";
        else if (event.id == 4 && (std::find_if(clients.begin(), clients.end(), [&event](const Client& c) { return c.name == event.client; }) == clients.end()))
            event.error = "ClientUnknown";
        
        events.push_back(event); // Добавление события в историю
        
        // Случай корректного события
        if (event.error.empty()) {
            if (event.id == 1) {
                Client client = { event.client, 0 };
                clients.push_back(client); // Добавление клиента в историю
            }
            if (event.id == 2) {
                auto client = std::find_if(clients.begin(), clients.end(), [&event](const Client& c) { return c.name == event.client; });
                if (client -> table != 0) { // Случай смены стола клиентом
                    auto old_table = std::find_if(tables.begin(), tables.end(), [&client](const Table& t) { return t.id == client -> table; });
                    calculate(*old_table, event.time, costPerHour);
                    old_table -> isBusy = false;
                }
                // Занимание стола
                auto new_table = std::find_if(tables.begin(), tables.end(), [&event](const Table& t) { return t.id == event.table; });
                new_table -> tmp_time = event.time;
                new_table -> isBusy = true;
                client -> table = new_table -> id;
            }
            if (event.id == 3) {
                if (queue.size() == numTables) { // Случай переполнения очереди
                    Event event11 = { event.time, 11, event.client };
                    events.push_back(event11);
                }
                else // Добавление клиента в очередь
                    queue.push(event.client);
            }
            if (event.id == 4) {
                auto client = std::find_if(clients.begin(), clients.end(), [&event](const Client& c) { return c.name == event.client; });
                auto table = std::find_if(tables.begin(), tables.end(), [&client](const Table& t) { return t.id == client -> table; });
                calculate(*table, event.time, costPerHour);
                clients.erase(client); // Удаление клиента из истории
                if (queue.empty()) // Случай пустой очереди
                    table -> isBusy = false; // Освобождение стола
                else { // Занимание стола клиентом из очереди
                    Event event12 = { event.time, 12, queue.front(), table -> id };
                    queue.pop();
                    auto client = std::find_if(clients.begin(), clients.end(), [&event12](const Client& c) { return c.name == event12.client; });
                    client -> table = table -> id;
                    events.push_back(event12);
                    table -> tmp_time = event.time;
                }
            }
        }
    }

    std::cout << startTime << std::endl;

    // Вывод событий
    for (size_t i = 0; i < events.size(); ++i) {
        std::cout << events[i].time << " " << events[i].id << " " << events[i].client;
        if (events[i].id == 2 || events[i].id == 12)
            std::cout << " " << events[i].table;
        std::cout << std::endl;
        if (!events[i].error.empty())
            std::cout << events[i].time << " 13 " << events[i].error << std::endl;
    }

    // Уход клиентов в конце рабочего дня
    std::sort(clients.begin(), clients.end(), [](const Client& a, const Client& b) { return a.name < b.name; });
    for (size_t i = 0; i < clients.size(); ++i) {
        std::cout << endTime << " 11 " << clients[i].name << std::endl;
        auto client = clients[i];
        auto table = std::find_if(tables.begin(), tables.end(), [&client](const Table& t) { return t.id == client.table; });
        calculate(*table, endTime, costPerHour);
        table -> isBusy = false;
        clients.erase(clients.begin());
    }

    std::cout << endTime << std::endl;

    // Вывод статистики по каждому столу
    for (size_t i = 0; i < tables.size(); ++i)
        std::cout << tables[i].id << " " << tables[i].revenue << " " <<
        std::setw(2) << std::setfill('0') << tables[i].time / 60 << ":" <<
        std::setw(2) << std::setfill('0') << tables[i].time % 60 << std::endl;

    return 0;
}