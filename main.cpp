#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <thread>
#include <chrono>

class LibraryDB {
private:
    pqxx::connection* conn;

    void printResult(const pqxx::result& res) {
        if (res.empty()) {
            std::cout << "Нет данных" << std::endl;
            return;
        }

        for (size_t i = 0; i < res.size(); ++i) {
            const auto& row = res[i];
            std::cout << "\n--- Запись " << (i + 1) << " ---" << std::endl;

            for (size_t j = 0; j < row.size(); ++j) {
                std::string col_name = res.column_name(j);
                auto field = row[static_cast<int>(j)];
                std::string value = field.is_null() ? "Нет данных" : field.c_str();

                std::string display_name = col_name;
                if (col_name == "genre") display_name = "Жанр";
                else if (col_name == "title") display_name = "Название";
                else if (col_name == "published_year") display_name = "Год";
                else if (col_name == "language") display_name = "Язык";
                else if (col_name == "is_reference") display_name = "Справочное";
                else if (col_name == "full_name") display_name = "ФИО";
                else if (col_name == "country") display_name = "Страна";
                else if (col_name == "book_count") display_name = "Книг";
                else if (col_name == "author_count") display_name = "Авторов";
                else if (col_name == "inventory_number") display_name = "Инвентарный номер";
                else if (col_name == "location") display_name = "Местоположение";
                else if (col_name == "status") display_name = "Статус";
                else if (col_name == "loan_date") display_name = "Дата выдачи";
                else if (col_name == "due_date") display_name = "Срок возврата";
                else if (col_name == "return_date") display_name = "Дата возврата";
                else if (col_name == "fine_amount") display_name = "Штраф";
                else if (col_name == "days_overdue") display_name = "Дней просрочки";
                else if (col_name == "loan_count") display_name = "Выдач";
                else if (col_name == "reader") display_name = "Читатель";
                else if (col_name == "book") display_name = "Книга";

                std::cout << std::left << std::setw(30) << display_name + ":" << value << std::endl;
            }
        }
        std::cout << std::endl;
    }

    void clearLine() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

public:
    LibraryDB(const std::string& conn_str) : conn(nullptr) {
        const int max_attempts = 20;
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            try {
                conn = new pqxx::connection(conn_str);
                if (conn->is_open()) {
                    std::cout << "Подключение к БД установлено" << std::endl;
                    return;
                }
            } catch (const std::exception &e) {
                std::cerr << "Ошибка подключения (попытка " << attempt
                          << "): " << e.what() << std::endl;
            }

            delete conn;
            conn = nullptr;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cerr << "Не удалось подключиться к БД после " << max_attempts << " попыток" << std::endl;
        exit(1);
    }

    ~LibraryDB() {
        delete conn;
    }

    void execute(const std::string& sql) {
        try {
            pqxx::work txn(*conn);
            txn.exec(sql);
            txn.commit();
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    pqxx::result query(const std::string& sql) {
        try {
            pqxx::work txn(*conn);
            pqxx::result res = txn.exec(sql);
            txn.commit();
            return res;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка запроса: " << e.what() << std::endl;
            throw;
        }
    }

    void query1_books_by_genre(const std::string& genre) {
        std::string sql = "SELECT b.title, g.genre, b.published_year, b.language, "
                          "CASE WHEN b.is_reference THEN 'Да' ELSE 'Нет' END as is_reference "
                          "FROM books b "
                          "JOIN genres g ON b.genre_id = g.genre_id "
                          "WHERE g.genre = '" + genre + "' "
                          "ORDER BY b.title";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "1. Книги жанра: " << genre << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query2_books_with_multiple_authors() {
        std::string sql = "SELECT b.title, COUNT(ba.author_id) as author_count "
                          "FROM books b "
                          "JOIN book_authors ba ON b.book_id = ba.book_id "
                          "GROUP BY b.book_id, b.title "
                          "HAVING COUNT(ba.author_id) > 1 "
                          "ORDER BY author_count DESC, b.title";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "2. Книги с несколькими авторами" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query3_authors_book_count() {
        std::string sql = "SELECT a.full_name, COUNT(ba.book_id) as book_count "
                          "FROM authors a "
                          "LEFT JOIN book_authors ba ON a.author_id = ba.author_id "
                          "GROUP BY a.author_id, a.full_name "
                          "ORDER BY book_count DESC, a.full_name";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "3. Авторы и количество книг" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query4_available_copies_by_title(const std::string& title) {
        std::string sql = "SELECT b.title, c.inventory_number, c.location "
                          "FROM copies c "
                          "JOIN books b ON c.book_id = b.book_id "
                          "WHERE b.title = '" + title + "' AND c.status = 'in_stock' "
                          "ORDER BY c.inventory_number";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "4. Доступные экземпляры: " << title << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query5_active_loans() {
        std::string sql = "SELECT r.full_name as reader, b.title as book, "
                          "c.inventory_number, l.loan_date, l.due_date "
                          "FROM loans l "
                          "JOIN readers r ON l.reader_id = r.reader_id "
                          "JOIN copies c ON l.copy_id = c.copy_id "
                          "JOIN books b ON c.book_id = b.book_id "
                          "WHERE l.return_date IS NULL "
                          "ORDER BY l.due_date, r.full_name";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "5. Текущие выдачи" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query6_overdue_loans() {
        std::string sql = "SELECT r.full_name as reader, b.title as book, "
                          "l.due_date, (CURRENT_DATE - l.due_date) as days_overdue, "
                          "l.fine_amount "
                          "FROM loans l "
                          "JOIN readers r ON l.reader_id = r.reader_id "
                          "JOIN copies c ON l.copy_id = c.copy_id "
                          "JOIN books b ON c.book_id = b.book_id "
                          "WHERE l.return_date IS NULL AND l.due_date < CURRENT_DATE "
                          "ORDER BY days_overdue DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "6. Просроченные выдачи" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query7_popular_genres() {
        std::string sql = "SELECT g.genre, COUNT(l.loan_id) as loan_count "
                          "FROM loans l "
                          "JOIN copies c ON l.copy_id = c.copy_id "
                          "JOIN books b ON c.book_id = b.book_id "
                          "JOIN genres g ON b.genre_id = g.genre_id "
                          "GROUP BY g.genre "
                          "ORDER BY loan_count DESC, g.genre";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "7. Популярные жанры (по выдачам)" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }

    void query8_return_book(int loan_id) {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "8. Возврат книги (loan_id = " << loan_id << ")" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        try {
            pqxx::work txn(*conn);
            pqxx::result updated = txn.exec_params(
                "UPDATE loans "
                "SET return_date = CURRENT_DATE, "
                "fine_amount = GREATEST(0, CURRENT_DATE - due_date) * 10 "
                "WHERE loan_id = $1 AND return_date IS NULL "
                "RETURNING loan_id, copy_id",
                loan_id
            );

            if (updated.empty()) {
                std::cout << "Выдача не найдена или уже закрыта" << std::endl;
                txn.commit();
                return;
            }

            int copy_id = updated[0]["copy_id"].as<int>();
            txn.exec_params("UPDATE copies SET status = 'in_stock' WHERE copy_id = $1", copy_id);

            pqxx::result info = txn.exec_params(
                "SELECT l.loan_id, r.full_name as reader, b.title as book, "
                "l.due_date, l.return_date, l.fine_amount "
                "FROM loans l "
                "JOIN readers r ON l.reader_id = r.reader_id "
                "JOIN copies c ON l.copy_id = c.copy_id "
                "JOIN books b ON c.book_id = b.book_id "
                "WHERE l.loan_id = $1",
                loan_id
            );
            txn.commit();
            printResult(info);
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void query9_add_reader() {
        std::string full_name, group, email, status;

        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "9. Добавление читателя" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        std::cout << "ФИО: ";
        clearLine();
        std::getline(std::cin, full_name);
        std::cout << "Группа (опционально): ";
        std::getline(std::cin, group);
        std::cout << "Email (опционально): ";
        std::getline(std::cin, email);
        std::cout << "Статус (active/inactive), по умолчанию active: ";
        std::getline(std::cin, status);
        if (status.empty()) {
            status = "active";
        }

        try {
            pqxx::work txn(*conn);
            std::string sql =
                "INSERT INTO readers (full_name, \"group\", email, status) VALUES (" +
                txn.quote(full_name) + ", " +
                (group.empty() ? "NULL" : txn.quote(group)) + ", " +
                (email.empty() ? "NULL" : txn.quote(email)) + ", " +
                txn.quote(status) + ") "
                "RETURNING reader_id, full_name, status";
            pqxx::result res = txn.exec(sql);
            txn.commit();
            printResult(res);
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void query10_issue_loan(int reader_id, int copy_id, const std::string& due_date) {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "10. Выдать книгу" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        try {
            pqxx::work txn(*conn);
            pqxx::result copy = txn.exec_params(
                "SELECT c.copy_id, c.status, b.title "
                "FROM copies c "
                "JOIN books b ON c.book_id = b.book_id "
                "WHERE c.copy_id = $1 FOR UPDATE",
                copy_id
            );

            if (copy.empty()) {
                std::cout << "Экземпляр не найден" << std::endl;
                txn.commit();
                return;
            }

            std::string status = copy[0]["status"].c_str();
            if (status != "in_stock") {
                std::cout << "Экземпляр недоступен (status = " << status << ")" << std::endl;
                txn.commit();
                return;
            }

            pqxx::result res = txn.exec_params(
                "INSERT INTO loans (reader_id, copy_id, due_date) "
                "VALUES ($1, $2, $3) "
                "RETURNING loan_id",
                reader_id,
                copy_id,
                due_date
            );

            txn.exec_params("UPDATE copies SET status = 'loaned' WHERE copy_id = $1", copy_id);
            txn.commit();

            std::cout << "Выдача создана. loan_id = " << res[0]["loan_id"].c_str() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void injection1_vulnerable_login() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 1: Уязвимый логин" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM readers WHERE email = '$email' AND status = '$status'" << std::endl;
        std::cout << "\nАтака:" << std::endl;
        std::cout << "Email: any' OR '1'='1" << std::endl;
        std::cout << "Status: любой" << std::endl;
        std::cout << "\nИтоговый SQL:" << std::endl;
        std::cout << "SELECT * FROM readers WHERE email = 'any' OR '1'='1' AND status = 'любой'" << std::endl;
        std::cout << "\nРезультат: выдаст всех читателей" << std::endl;
    }

    void injection2_vulnerable_search() {
        std::string search = "%' OR '1'='1";
        std::string sql = "SELECT * FROM books WHERE title LIKE '%" + search + "%'";

        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 2: Уязвимый поиск" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Пользовательский ввод: " << search << std::endl;
        std::cout << "\nСформированный SQL:" << std::endl;
        std::cout << sql << std::endl;
        std::cout << "\nРезультат: покажет ВСЕ книги" << std::endl;

        try {
            auto res = query(sql);
            std::cout << "\nНайдено записей: " << res.size() << std::endl;
        } catch (...) {}
    }

    void injection3_union_attack() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 3: UNION-атака" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT title, published_year FROM books WHERE book_id = $id" << std::endl;
        std::cout << "\nАтака (id):" << std::endl;
        std::cout << "1 UNION SELECT full_name, 0 FROM readers" << std::endl;
        std::cout << "\nИтоговый SQL:" << std::endl;
        std::cout << "SELECT title, published_year FROM books WHERE book_id = 1 UNION SELECT full_name, 0 FROM readers" << std::endl;
        std::cout << "\nРезультат: получаем список читателей" << std::endl;
    }

    void injection4_error_based() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 4: Error-based" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM books WHERE book_id = $id" << std::endl;
        std::cout << "\nАтака (id):" << std::endl;
        std::cout << "1 AND 1=CAST((SELECT version()) AS INT)" << std::endl;
        std::cout << "\nРезультат: ошибка БД покажет версию PostgreSQL" << std::endl;
    }

    void injection5_time_based() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 5: Time-based (Blind)" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM readers WHERE email = '$email'" << std::endl;
        std::cout << "\nАтака (email):" << std::endl;
        std::cout << "test' AND (SELECT pg_sleep(5))--" << std::endl;
        std::cout << "\nРезультат: если ответ задерживается 5 сек, значит запись существует" << std::endl;
    }

    void safe_search_books(const std::string& search_term) {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Безопасный поиск книги" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        try {
            pqxx::work txn(*conn);
            std::string pattern = "%" + search_term + "%";
            std::string sql = "SELECT title, published_year, language "
                              "FROM books WHERE title ILIKE " + txn.quote(pattern);
            pqxx::result res = txn.exec(sql);
            txn.commit();

            std::cout << "Поиск: " << search_term << std::endl;
            std::cout << "Найдено записей: " << res.size() << std::endl;
            printResult(res);
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void safe_insert_book() {
        std::string title, isbn, language, year_str, ref_str;
        int genre_id;

        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Безопасное добавление книги" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        std::cout << "Название: ";
        clearLine();
        std::getline(std::cin, title);
        std::cout << "ID жанра: ";
        std::cin >> genre_id;
        std::cout << "ISBN (опционально): ";
        clearLine();
        std::getline(std::cin, isbn);
        std::cout << "Год издания (опционально): ";
        std::getline(std::cin, year_str);
        std::cout << "Язык (опционально): ";
        std::getline(std::cin, language);
        std::cout << "Справочная книга? (да/нет): ";
        std::getline(std::cin, ref_str);

        try {
            pqxx::work txn(*conn);
            bool is_reference = (ref_str == "да" || ref_str == "Да" || ref_str == "yes" || ref_str == "y");

            std::string sql =
                "INSERT INTO books (genre_id, title, isbn, published_year, language, is_reference) VALUES (" +
                txn.quote(genre_id) + ", " +
                txn.quote(title) + ", " +
                (isbn.empty() ? "NULL" : txn.quote(isbn)) + ", " +
                (year_str.empty() ? "NULL" : txn.quote(std::stoi(year_str))) + ", " +
                (language.empty() ? "NULL" : txn.quote(language)) + ", " +
                txn.quote(is_reference) + ") "
                "RETURNING book_id, title";

            pqxx::result res = txn.exec(sql);
            txn.commit();

            std::cout << "\nКнига успешно добавлена!" << std::endl;
            std::cout << "ID: " << res[0]["book_id"].c_str() << std::endl;
            std::cout << "Название: " << res[0]["title"].c_str() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void init_database() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Инициализация базы данных" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        try {
            pqxx::work txn(*conn);

            std::vector<std::string> tables = {
                "CREATE TABLE IF NOT EXISTS genres ("
                "genre_id SERIAL PRIMARY KEY,"
                "genre VARCHAR(100) NOT NULL UNIQUE)",

                "CREATE TABLE IF NOT EXISTS authors ("
                "author_id SERIAL PRIMARY KEY,"
                "full_name VARCHAR(200) NOT NULL,"
                "country VARCHAR(100))",

                "CREATE TABLE IF NOT EXISTS readers ("
                "reader_id SERIAL PRIMARY KEY,"
                "full_name VARCHAR(200) NOT NULL,"
                "\"group\" VARCHAR(50),"
                "email VARCHAR(150) UNIQUE,"
                "status VARCHAR(50) NOT NULL DEFAULT 'active',"
                "registration_date DATE NOT NULL DEFAULT CURRENT_DATE)",

                "CREATE TABLE IF NOT EXISTS books ("
                "book_id SERIAL PRIMARY KEY,"
                "genre_id INT REFERENCES genres(genre_id),"
                "title VARCHAR(255) NOT NULL,"
                "isbn VARCHAR(32) UNIQUE,"
                "published_year INT,"
                "language VARCHAR(50),"
                "is_reference BOOLEAN NOT NULL DEFAULT FALSE)",

                "CREATE TABLE IF NOT EXISTS book_authors ("
                "book_id INT NOT NULL REFERENCES books(book_id) ON DELETE CASCADE,"
                "author_id INT NOT NULL REFERENCES authors(author_id) ON DELETE CASCADE,"
                "PRIMARY KEY (book_id, author_id))",

                "CREATE TABLE IF NOT EXISTS copies ("
                "copy_id SERIAL PRIMARY KEY,"
                "book_id INT NOT NULL REFERENCES books(book_id) ON DELETE CASCADE,"
                "inventory_number VARCHAR(50) UNIQUE,"
                "location VARCHAR(100),"
                "status VARCHAR(50) NOT NULL DEFAULT 'in_stock')",

                "CREATE TABLE IF NOT EXISTS loans ("
                "loan_id SERIAL PRIMARY KEY,"
                "reader_id INT NOT NULL REFERENCES readers(reader_id),"
                "copy_id INT NOT NULL REFERENCES copies(copy_id),"
                "loan_date DATE NOT NULL DEFAULT CURRENT_DATE,"
                "due_date DATE NOT NULL,"
                "return_date DATE,"
                "fine_amount NUMERIC(10,2) NOT NULL DEFAULT 0)"
            };

            for (const auto& sql : tables) {
                txn.exec(sql);
            }

            txn.commit();
            std::cout << "Таблицы созданы успешно!" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }

    void seed_data() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Заполнение тестовыми данными" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;

        try {
            pqxx::work cleanup(*conn);
            cleanup.exec("TRUNCATE loans, copies, book_authors, books, authors, readers, genres CASCADE");
            cleanup.commit();

            pqxx::work txn(*conn);

            txn.exec(R"(
                INSERT INTO genres (genre_id, genre) VALUES
                (1, 'Фантастика'),
                (2, 'История'),
                (3, 'Классика'),
                (4, 'Научпоп')
            )");

            txn.exec(R"(
                INSERT INTO authors (author_id, full_name, country) VALUES
                (1, 'Айзек Азимов', 'США'),
                (2, 'Лев Толстой', 'Россия'),
                (3, 'Юваль Харари', 'Израиль'),
                (4, 'Братья Стругацкие', 'Россия')
            )");

            txn.exec(R"(
                INSERT INTO readers (reader_id, full_name, "group", email, status, registration_date) VALUES
                (1, 'Иван Петров', 'БИБ-101', 'ivan@example.com', 'active', CURRENT_DATE - 30),
                (2, 'Мария Соколова', 'БИБ-102', 'maria@example.com', 'active', CURRENT_DATE - 10),
                (3, 'Алексей Ким', NULL, 'alex@example.com', 'inactive', CURRENT_DATE - 100)
            )");

            txn.exec(R"(
                INSERT INTO books (book_id, genre_id, title, isbn, published_year, language, is_reference) VALUES
                (1, 1, 'Основание', '978-1-234', 1951, 'ru', false),
                (2, 3, 'Война и мир', '978-1-235', 1869, 'ru', false),
                (3, 4, 'Sapiens', '978-1-236', 2011, 'en', false),
                (4, 1, 'Понедельник начинается в субботу', '978-1-237', 1965, 'ru', false)
            )");

            txn.exec(R"(
                INSERT INTO book_authors (book_id, author_id) VALUES
                (1, 1),
                (2, 2),
                (3, 3),
                (4, 4)
            )");

            txn.exec(R"(
                INSERT INTO copies (copy_id, book_id, inventory_number, location, status) VALUES
                (1, 1, 'INV-001', 'Абонемент', 'in_stock'),
                (2, 1, 'INV-002', 'Зал 1', 'loaned'),
                (3, 2, 'INV-003', 'Абонемент', 'loaned'),
                (4, 3, 'INV-004', 'Зал 2', 'in_stock'),
                (5, 4, 'INV-005', 'Зал 1', 'in_stock')
            )");

            txn.exec(R"(
                INSERT INTO loans (loan_id, reader_id, copy_id, loan_date, due_date, return_date, fine_amount) VALUES
                (1, 1, 2, CURRENT_DATE - 15, CURRENT_DATE - 5, NULL, 0),
                (2, 2, 3, CURRENT_DATE - 7, CURRENT_DATE + 7, NULL, 0)
            )");

            txn.commit();

            pqxx::work reset_seq(*conn);
            reset_seq.exec("SELECT setval('genres_genre_id_seq', COALESCE((SELECT MAX(genre_id) FROM genres), 0) + 1, false)");
            reset_seq.exec("SELECT setval('authors_author_id_seq', COALESCE((SELECT MAX(author_id) FROM authors), 0) + 1, false)");
            reset_seq.exec("SELECT setval('readers_reader_id_seq', COALESCE((SELECT MAX(reader_id) FROM readers), 0) + 1, false)");
            reset_seq.exec("SELECT setval('books_book_id_seq', COALESCE((SELECT MAX(book_id) FROM books), 0) + 1, false)");
            reset_seq.exec("SELECT setval('copies_copy_id_seq', COALESCE((SELECT MAX(copy_id) FROM copies), 0) + 1, false)");
            reset_seq.exec("SELECT setval('loans_loan_id_seq', COALESCE((SELECT MAX(loan_id) FROM loans), 0) + 1, false)");
            reset_seq.commit();

            std::cout << "Все тестовые данные успешно добавлены!" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
};

static std::string get_env_or_default(const char* key, const std::string& def) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : def;
}

static std::string build_conn_string() {
    std::string host = get_env_or_default("DB_HOST", "localhost");
    std::string port = get_env_or_default("DB_PORT", "5432");
    std::string name = get_env_or_default("DB_NAME", "library");
    std::string user = get_env_or_default("DB_USER", "postgres");
    std::string pass = get_env_or_default("DB_PASSWORD", "postgres");

    return "host=" + host + " port=" + port + " dbname=" + name + " user=" + user + " password=" + pass;
}

void execute_10_queries(LibraryDB& db) {
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "   ВЫПОЛНЕНИЕ 10 ОСНОВНЫХ ЗАПРОСОВ" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;

    int query_choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Выберите запрос (1-10) или 0 для выхода:" << std::endl;
        std::cout << "1. Книги по жанру" << std::endl;
        std::cout << "2. Книги с несколькими авторами" << std::endl;
        std::cout << "3. Авторы и количество книг" << std::endl;
        std::cout << "4. Доступные экземпляры книги" << std::endl;
        std::cout << "5. Текущие выдачи" << std::endl;
        std::cout << "6. Просроченные выдачи" << std::endl;
        std::cout << "7. Популярные жанры" << std::endl;
        std::cout << "8. Возврат книги" << std::endl;
        std::cout << "9. Добавление читателя" << std::endl;
        std::cout << "10. Выдача книги" << std::endl;
        std::cout << "0. Выход в главное меню" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> query_choice;

        switch (query_choice) {
            case 1: {
                std::string genre;
                std::cout << "\nВведите жанр (например: Фантастика): ";
                std::cin.ignore();
                std::getline(std::cin, genre);
                db.query1_books_by_genre(genre);
                break;
            }
            case 2:
                db.query2_books_with_multiple_authors();
                break;
            case 3:
                db.query3_authors_book_count();
                break;
            case 4: {
                std::string title;
                std::cout << "\nВведите название книги: ";
                std::cin.ignore();
                std::getline(std::cin, title);
                db.query4_available_copies_by_title(title);
                break;
            }
            case 5:
                db.query5_active_loans();
                break;
            case 6:
                db.query6_overdue_loans();
                break;
            case 7:
                db.query7_popular_genres();
                break;
            case 8: {
                int loan_id;
                std::cout << "\nВведите loan_id: ";
                std::cin >> loan_id;
                db.query8_return_book(loan_id);
                break;
            }
            case 9:
                db.query9_add_reader();
                break;
            case 10: {
                int reader_id;
                int copy_id;
                std::string due_date;
                std::cout << "\nВведите reader_id: ";
                std::cin >> reader_id;
                std::cout << "Введите copy_id: ";
                std::cin >> copy_id;
                std::cout << "Введите due_date (YYYY-MM-DD): ";
                std::cin.ignore();
                std::getline(std::cin, due_date);
                db.query10_issue_loan(reader_id, copy_id, due_date);
                break;
            }
            case 0:
                std::cout << "Возврат в главное меню..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (query_choice != 0);
}

void execute_sql_injections(LibraryDB& db) {
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "      ДЕМОНСТРАЦИЯ SQL-ИНЪЕКЦИЙ" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;

    int injection_choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Выберите тип инъекции (1-5) или 0 для выхода:" << std::endl;
        std::cout << "1. Уязвимый логин" << std::endl;
        std::cout << "2. Уязвимый поиск" << std::endl;
        std::cout << "3. UNION-атака" << std::endl;
        std::cout << "4. Error-based атака" << std::endl;
        std::cout << "5. Time-based атака (Blind)" << std::endl;
        std::cout << "0. Выход в главное меню" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> injection_choice;

        switch (injection_choice) {
            case 1:
                db.injection1_vulnerable_login();
                break;
            case 2:
                db.injection2_vulnerable_search();
                break;
            case 3:
                db.injection3_union_attack();
                break;
            case 4:
                db.injection4_error_based();
                break;
            case 5:
                db.injection5_time_based();
                break;
            case 0:
                std::cout << "Возврат в главное меню..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (injection_choice != 0);
}

int main() {
    std::string conn_str = build_conn_string();
    LibraryDB db(conn_str);

    int choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "          БИБЛИОТЕЧНАЯ БАЗА ДАННЫХ" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "1. Инициализировать таблицы" << std::endl;
        std::cout << "2. Заполнить тестовыми данными" << std::endl;
        std::cout << "3. Выполнить 10 основных запросов" << std::endl;
        std::cout << "4. Демонстрация SQL-инъекций" << std::endl;
        std::cout << "5. Безопасный поиск книги" << std::endl;
        std::cout << "6. Безопасное добавление книги" << std::endl;
        std::cout << "0. Выход" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                db.init_database();
                break;
            case 2:
                db.seed_data();
                break;
            case 3:
                execute_10_queries(db);
                break;
            case 4:
                execute_sql_injections(db);
                break;
            case 5: {
                std::string search;
                std::cout << "Введите название для поиска: ";
                std::cin.ignore();
                std::getline(std::cin, search);
                db.safe_search_books(search);
                break;
            }
            case 6:
                db.safe_insert_book();
                break;
            case 0:
                std::cout << "Выход из программы..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (choice != 0);

    return 0;
}
