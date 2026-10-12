#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string.h>
#ifdef _WIN32
#include <windows.h> 
#endif
using namespace std;

// Структура записи БД
struct Employee
{
    int id;
    char name[100];
    char position[100];
    char department[100];
    int experience;
    int salary;
    float rating;
};

// Динамический массив структур
struct Database
{
    Employee* data;
    int size;
    int capacity;
};

//Вспомогательные функции для таблицы

int charCount(const char* s)
{
    int n = 0;
    for (int i = 0; s[i] != '\0'; i++)
        if ((s[i] & 0xC0) != 0x80) n++;
    return n;
}

int intLen(int v)
{
    int n = 1;
    while (v >= 10)
    {
        v = v / 10;
        n++;
    }
    return n;
}

void printSpaces(int n)
{
    for (int i = 0; i < n; i++) cout << ' ';
}

int ratingTenths(float r)
{
    return (int)(r * 10 + 0.5);
}

const int COLS = 7;
int widths[COLS] = { 4, 32, 22, 16, 5, 8, 8 };

void printLine()
{
    for (int i = 0; i < COLS; i++)
    {
        cout << '+';
        for (int j = 0; j < widths[i] + 2; j++) cout << '-';
    }
    cout << '+' << endl;
}

void cellText(const char* s, int width)
{
    cout << "| " << s;
    printSpaces(width - charCount(s));
    cout << ' ';
}

void cellInt(int v, int width)
{
    cout << "| " << v;
    printSpaces(width - intLen(v));
    cout << ' ';
}

void cellRating(float r, int width)
{
    int t = ratingTenths(r);
    cout << "| " << t / 10 << '.' << t % 10;
    printSpaces(width - (intLen(t / 10) + 2));
    cout << ' ';
}

// Ввод с клавиатуры с проверкой

int readInt(const char* prompt, int minV, int maxV)
{
    char buf[100];
    while (1)
    {
        cout << prompt;
        cin >> buf;
        int len = (int)strlen(buf);
        int ok = (len > 0 && len <= 9);
        for (int i = 0; i < len && ok; i++)
            if (buf[i] < '0' || buf[i] > '9') ok = 0;
        if (ok)
        {
            int v = 0;
            for (int i = 0; i < len; i++) v = v * 10 + (buf[i] - '0');
            if (v >= minV && v <= maxV) return v;
        }
        cout << "Ошибка ввода. Нужно целое число от " << minV << " до " << maxV << "." << endl;
    }
}

float readFloat(const char* prompt, float minV, float maxV)
{
    char buf[100];
    while (1)
    {
        cout << prompt;
        cin >> buf;
        int len = (int)strlen(buf);
        int ok = (len > 0 && len <= 9);
        int dots = 0, digits = 0;
        for (int i = 0; i < len && ok; i++)
        {
            if (buf[i] == '.') dots++;
            else if (buf[i] >= '0' && buf[i] <= '9') digits++;
            else ok = 0;
        }
        if (dots > 1 || digits == 0) ok = 0;
        if (ok)
        {
            float v = 0, frac = 0.1f;
            int afterDot = 0;
            for (int i = 0; i < len; i++)
            {
                if (buf[i] == '.') afterDot = 1;
                else if (!afterDot) v = v * 10 + (buf[i] - '0');
                else
                {
                    v = v + (buf[i] - '0') * frac;
                    frac = frac / 10;
                }
            }
            if (v >= minV && v <= maxV) return v;
        }
        cout << "Ошибка ввода. Нужно число от " << minV << " до " << maxV << " (дробная часть через точку)." << endl;
    }
}

// Ввод слова (без пробелов)
void readText(const char* prompt, char* dest, int maxChars)
{
    char buf[300];
    while (1)
    {
        cout << prompt;
        cin >> buf;
        if (strlen(buf) < 100 && charCount(buf) <= maxChars)
        {
            strcpy(dest, buf);
            return;
        }
        cout << "Слишком длинная строка (максимум " << maxChars << " символов)." << endl;
    }
}

// Работа с динамическим массивом

void initDb(Database* db)
{
    db->capacity = 4;
    db->size = 0;
    db->data = new Employee[db->capacity];
}

void freeDb(Database* db)
{
    delete[] db->data;
    db->data = NULL;
    db->size = 0;
    db->capacity = 0;
}

void growDb(Database* db)
{
    int newCap = db->capacity * 2;
    Employee* tmp = new Employee[newCap];
    for (int i = 0; i < db->size; i++) tmp[i] = db->data[i];
    delete[] db->data;
    db->data = tmp;
    db->capacity = newCap;
}

void addToDb(Database* db, Employee e)
{
    if (db->size == db->capacity) growDb(db);
    db->data[db->size] = e;
    db->size++;
}

int findIndexById(const Database* db, int id)
{
    for (int i = 0; i < db->size; i++)
        if (db->data[i].id == id) return i;
    return -1;
}

int nextFreeId(const Database* db)
{
    int m = 0;
    for (int i = 0; i < db->size; i++)
        if (db->data[i].id > m) m = db->data[i].id;
    return m + 1;
}

// Чтение из файла
// Формат строки: id ФИО должность отдел стаж оклад оценка

int loadFromFile(Database* db, const char* fileName)
{
    ifstream fin;
    fin.open(fileName);
    if (!fin)
    {
        cout << "Файл не найден!" << endl;
        return 0;
    }
    Employee e;
    char n[200], p[200], d[200];
    int count = 0, skipped = 0;
    while (fin >> e.id >> n >> p >> d >> e.experience >> e.salary >> e.rating)
    {
        if (e.id <= 0 || findIndexById(db, e.id) != -1 ||
            strlen(n) >= 100 || strlen(p) >= 100 || strlen(d) >= 100 ||
            charCount(n) > 32 || charCount(p) > 22 || charCount(d) > 16 ||
            e.experience < 0 || e.experience > 60 ||
            e.salary < 0 || e.rating < 0 || e.rating > 10)
        {
            skipped++;
            continue;
        }
        strcpy(e.name, n);
        strcpy(e.position, p);
        strcpy(e.department, d);
        addToDb(db, e);
        count++;
    }
    fin.close();
    cout << "Загружено записей: " << count << ", пропущено некорректных: " << skipped << endl;
    return 1;
}

// Просмотр

void printTable(const Database* db)
{
    if (db->size == 0)
    {
        cout << "База данных пуста." << endl;
        return;
    }
    printLine();
    cellText("ID", widths[0]);
    cellText("ФИО", widths[1]);
    cellText("Должность", widths[2]);
    cellText("Отдел", widths[3]);
    cellText("Стаж", widths[4]);
    cellText("Оклад", widths[5]);
    cellText("Оценка", widths[6]);
    cout << '|' << endl;
    printLine();
    for (int i = 0; i < db->size; i++)
    {
        const Employee* e = &db->data[i];
        cellInt(e->id, widths[0]);
        cellText(e->name, widths[1]);
        cellText(e->position, widths[2]);
        cellText(e->department, widths[3]);
        cellInt(e->experience, widths[4]);
        cellInt(e->salary, widths[5]);
        cellRating(e->rating, widths[6]);
        cout << '|' << endl;
    }
    printLine();
}

// Изменение

void editEmployee(Database* db)
{
    if (db->size == 0)
    {
        cout << "База данных пуста." << endl;
        return;
    }
    int id = readInt("Введите id записи для редактирования: ", 1, 999999999);
    int idx = findIndexById(db, id);
    if (idx == -1)
    {
        cout << "Запись с таким id не найдена." << endl;
        return;
    }
    Employee* e = db->data + idx;

    cout << "Что изменить?" << endl;
    cout << " 1 - ФИО" << endl;
    cout << " 2 - Должность" << endl;
    cout << " 3 - Отдел" << endl;
    cout << " 4 - Стаж" << endl;
    cout << " 5 - Оклад" << endl;
    cout << " 6 - Оценка" << endl;
    cout << " 0 - Отмена" << endl;
    int c = readInt("Выбор: ", 0, 6);
    switch (c)
    {
    case 1: readText("Новое ФИО (слова через _): ", e->name, 32); break;
    case 2: readText("Новая должность: ", e->position, 22); break;
    case 3: readText("Новый отдел: ", e->department, 16); break;
    case 4: e->experience = readInt("Новый стаж (лет): ", 0, 60); break;
    case 5: e->salary = readInt("Новый оклад: ", 0, 100000000); break;
    case 6: e->rating = readFloat("Новая оценка (0..10): ", 0, 10); break;
    default: return;
    }
    cout << "Запись изменена." << endl;
}

//Добавление

void addEmployee(Database* db)
{
    Employee e;
    e.id = nextFreeId(db);
    cout << "Новая запись, id = " << e.id << endl;
    readText("ФИО (слова через _): ", e.name, 32);
    readText("Должность: ", e.position, 22);
    readText("Отдел: ", e.department, 16);
    e.experience = readInt("Стаж (лет): ", 0, 60);
    e.salary = readInt("Оклад: ", 0, 100000000);
    e.rating = readFloat("Оценка (0..10): ", 0, 10);
    addToDb(db, e);
    cout << "Запись добавлена." << endl;
}

// Удаление 

void deleteEmployee(Database* db)
{
    if (db->size == 0)
    {
        cout << "База данных пуста." << endl;
        return;
    }
    int id = readInt("Введите id записи для удаления: ", 1, 999999999);
    int idx = findIndexById(db, id);
    if (idx == -1)
    {
        cout << "Запись с таким id не найдена." << endl;
        return;
    }
    for (int i = idx; i < db->size - 1; i++) db->data[i] = db->data[i + 1];
    db->size--;
    cout << "Запись удалена." << endl;
}

// Вычисления

void calculate(const Database* db)
{
    if (db->size == 0)
    {
        cout << "База данных пуста." << endl;
        return;
    }
    double sum = 0, expSum = 0, ratSum = 0;
    int iMax = 0, iMin = 0;
    for (int i = 0; i < db->size; i++)
    {
        sum += db->data[i].salary;
        expSum += db->data[i].experience;
        ratSum += db->data[i].rating;
        if (db->data[i].salary > db->data[iMax].salary) iMax = i;
        if (db->data[i].salary < db->data[iMin].salary) iMin = i;
    }
    cout << "Количество сотрудников: " << db->size << endl;
    cout << "Суммарный фонд оплаты:  " << sum << endl;
    cout << "Средний оклад:          " << sum / db->size << endl;
    cout << "Максимальный оклад:     " << db->data[iMax].salary << " (" << db->data[iMax].name << ")" << endl;
    cout << "Минимальный оклад:      " << db->data[iMin].salary << " (" << db->data[iMin].name << ")" << endl;
    cout << "Средний стаж:           " << expSum / db->size << " лет" << endl;
    cout << "Средняя оценка:         " << ratSum / db->size << endl;

    char dep[100];
    readText("\nСредний оклад по отделу (введите отдел или - чтобы пропустить): ", dep, 16);
    if (strcmp(dep, "-") != 0)
    {
        double s = 0;
        int n = 0;
        for (int i = 0; i < db->size; i++)
            if (strcmp(db->data[i].department, dep) == 0)
            {
                s += db->data[i].salary;
                n++;
            }
        if (n == 0) cout << "Отдел не найден." << endl;
        else cout << "Отдел " << dep << ": сотрудников " << n << ", средний оклад " << s / n << endl;
    }
}

// Сохранение

int saveToFile(const Database* db, const char* fileName)
{
    ofstream fout;
    fout.open(fileName);
    if (!fout)
    {
        cout << "Не удалось создать файл!" << endl;
        return 0;
    }
    for (int i = 0; i < db->size; i++)
    {
        const Employee* e = &db->data[i];
        fout << e->id << " " << e->name << " " << e->position << " " << e->department << " "
            << e->experience << " " << e->salary << " " << e->rating << endl;
    }
    fout.close();
    cout << "Данные сохранены в файл " << fileName << endl;
    return 1;
}

// Меню 

void printMenu()
{
    cout << endl << "===== МЕНЮ: Сотрудники =====" << endl;
    cout << "1 - Просмотр данных" << endl;
    cout << "2 - Корректировка записи" << endl;
    cout << "3 - Добавление записи" << endl;
    cout << "4 - Удаление записи" << endl;
    cout << "5 - Вычисления" << endl;
    cout << "6 - Сохранить в новый файл и выйти" << endl;
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001); // UTF-8
    SetConsoleCP(65001);
#endif
    Database db;
    initDb(&db);

    char inName[100];
    readText("Введите имя входного файла: ", inName, 40);
    if (!loadFromFile(&db, inName))
    {
        freeDb(&db);
        return 1;
    }

    int run = 1;
    while (run)
    {
        printMenu();
        int c = readInt("Ваш выбор: ", 1, 6);
        switch (c)
        {
        case 1: printTable(&db); break;
        case 2: editEmployee(&db); break;
        case 3: addEmployee(&db); break;
        case 4: deleteEmployee(&db); break;
        case 5: calculate(&db); break;
        case 6:
        {
            char outName[100];
            readText("Введите имя выходного файла: ", outName, 40);
            if (saveToFile(&db, outName)) run = 0;
            break;
        }
        }
    }

    freeDb(&db);
    return 0;
}