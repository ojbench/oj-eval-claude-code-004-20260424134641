#include "Index.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stack>
#include <map>
#include <set>
#include <vector>

using namespace std;

typedef FixedString<30> ID;
typedef FixedString<20> ISBN;
typedef FixedString<60> Name;
typedef FixedString<60> Author;
typedef FixedString<60> Keyword;

struct Account {
    ID userID;
    ID password;
    ID username;
    int privilege = 0;
};

struct Book {
    ISBN isbn;
    Name name;
    Author author;
    Keyword keyword;
    double price = 0;
    long long stock = 0;
};

struct FinancialRecord {
    double income = 0;
    double expenditure = 0;
};

// Global Managers
DataStorage<Account, 1> account_storage("accounts.dat");
Index<ID, int, 400> account_index("accounts_index");

DataStorage<Book, 1> book_storage("books.dat");
Index<ISBN, int, 400> book_isbn_index("books_isbn_index");
Index<Name, int, 400> book_name_index("books_name_index");
Index<Author, int, 400> book_author_index("books_author_index");
Index<Keyword, int, 400> book_keyword_index("books_keyword_index");

DataStorage<FinancialRecord, 1> finance_storage("finance.dat");

struct LoginSession {
    ID userID;
    int privilege = 0;
    int selected_book_pos = -1;
};

stack<LoginSession> login_stack;

void initialize() {
    int count;
    account_storage.get_info(count, 0);
    if (count == 0) {
        Account root;
        root.userID = ID("root");
        root.password = ID("sjtu");
        root.username = ID("root");
        root.privilege = 7;
        int pos = account_storage.write(root);
        account_index.insert(root.userID, pos);
        account_storage.write_info(1, 0);
    }
}

int get_current_privilege() {
    return login_stack.empty() ? 0 : login_stack.top().privilege;
}

bool is_legal_id_char(char c) {
    return isalnum(c) || c == '_';
}

bool is_valid_id(const string &s, size_t max_len) {
    if (s.empty() || s.length() > max_len) return false;
    for (char c : s) if (!is_legal_id_char(c)) return false;
    return true;
}

bool is_valid_string_param(const string &s, size_t max_len) {
    if (s.empty() || s.length() > max_len) return false;
    for (char c : s) if ((unsigned char)c < 33 || (unsigned char)c > 126) return false;
    return true;
}

void print_invalid() {
    cout << "Invalid" << endl;
}

// Finance helpers
void add_finance(double income, double expenditure) {
    int total_records;
    finance_storage.get_info(total_records, 0);
    FinancialRecord record;
    record.income = income;
    record.expenditure = expenditure;
    finance_storage.write(record);
    finance_storage.write_info(total_records + 1, 0);
}

void show_finance(int count = -1) {
    int total_records;
    finance_storage.get_info(total_records, 0);
    if (count == -1) count = total_records;
    if (count == 0) {
        cout << endl;
        return;
    }
    if (count > total_records || count < 0) {
        print_invalid();
        return;
    }
    double total_income = 0, total_expenditure = 0;
    for (int i = 0; i < count; ++i) {
        FinancialRecord record;
        finance_storage.read(record, (total_records - 1 - i) * sizeof(FinancialRecord) + sizeof(int));
        total_income += record.income;
        total_expenditure += record.expenditure;
    }
    cout << fixed << setprecision(2) << "+ " << total_income << " - " << total_expenditure << endl;
}

// Book helpers
void print_book(const Book &b) {
    cout << b.isbn.to_string() << "\t" << b.name.to_string() << "\t" << b.author.to_string() << "\t"
         << b.keyword.to_string() << "\t" << fixed << setprecision(2) << b.price << "\t" << b.stock << endl;
}

vector<string> split_keywords(const string &k) {
    vector<string> res;
    if (k.empty()) return {};
    size_t start = 0;
    size_t end = k.find('|');
    while (end != string::npos) {
        string sub = k.substr(start, end - start);
        if (sub.empty()) return {};
        res.push_back(sub);
        start = end + 1;
        end = k.find('|', start);
    }
    string last = k.substr(start);
    if (last.empty()) return {};
    res.push_back(last);
    return res;
}

string extract_quoted(const string &s) {
    if (s.length() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.length() - 2);
    }
    return "";
}

void handle_command(string line) {
    if (line.empty() || line.find_first_not_of(' ') == string::npos) return;
    stringstream ss(line);
    string cmd;
    ss >> cmd;

    if (cmd == "quit" || cmd == "exit") {
        exit(0);
    } else if (cmd == "su") {
        string userID, password;
        ss >> userID >> password;
        if (!is_valid_id(userID, 30)) { print_invalid(); return; }
        auto pos_vec = account_index.find(ID(userID));
        if (pos_vec.empty()) { print_invalid(); return; }
        Account acc;
        account_storage.read(acc, pos_vec[0]);
        if (password.empty()) {
            if (get_current_privilege() > acc.privilege) {
                login_stack.push({acc.userID, acc.privilege, -1});
            } else { print_invalid(); }
        } else {
            if (!is_valid_id(password, 30)) { print_invalid(); return; }
            if (acc.password == ID(password)) {
                login_stack.push({acc.userID, acc.privilege, -1});
            } else { print_invalid(); }
        }
    } else if (cmd == "logout") {
        if (login_stack.empty() || get_current_privilege() < 1) { print_invalid(); return; }
        login_stack.pop();
    } else if (cmd == "register") {
        string userID, password, username;
        if (!(ss >> userID >> password >> username)) { print_invalid(); return; }
        if (!is_valid_id(userID, 30) || !is_valid_id(password, 30) || username.length() > 30) { print_invalid(); return; }
        if (account_index.find(ID(userID)).size() > 0) { print_invalid(); return; }
        Account acc;
        acc.userID = ID(userID);
        acc.password = ID(password);
        acc.username = ID(username);
        acc.privilege = 1;
        int pos = account_storage.write(acc);
        account_index.insert(acc.userID, pos);
        int count;
        account_storage.get_info(count, 0);
        account_storage.write_info(count + 1, 0);
    } else if (cmd == "passwd") {
        if (get_current_privilege() < 1) { print_invalid(); return; }
        string userID, p1, p2;
        ss >> userID >> p1 >> p2;
        if (!is_valid_id(userID, 30)) { print_invalid(); return; }
        auto pos_vec = account_index.find(ID(userID));
        if (pos_vec.empty()) { print_invalid(); return; }
        Account acc;
        account_storage.read(acc, pos_vec[0]);
        if (p2.empty()) { // passwd [UserID] [NewPassword]
            if (get_current_privilege() != 7) { print_invalid(); return; }
            if (!is_valid_id(p1, 30)) { print_invalid(); return; }
            acc.password = ID(p1);
            account_storage.update(acc, pos_vec[0]);
        } else { // passwd [UserID] [CurrentPassword] [NewPassword]
            if (!is_valid_id(p1, 30) || !is_valid_id(p2, 30)) { print_invalid(); return; }
            if (acc.password == ID(p1)) {
                acc.password = ID(p2);
                account_storage.update(acc, pos_vec[0]);
            } else { print_invalid(); }
        }
    } else if (cmd == "useradd") {
        if (get_current_privilege() < 3) { print_invalid(); return; }
        string userID, password, username;
        int privilege;
        if (!(ss >> userID >> password >> privilege >> username)) { print_invalid(); return; }
        if (!is_valid_id(userID, 30) || !is_valid_id(password, 30) || username.length() > 30) { print_invalid(); return; }
        if (privilege != 1 && privilege != 3 && privilege != 7) { print_invalid(); return; }
        if (privilege >= get_current_privilege()) { print_invalid(); return; }
        if (account_index.find(ID(userID)).size() > 0) { print_invalid(); return; }
        Account acc;
        acc.userID = ID(userID);
        acc.password = ID(password);
        acc.username = ID(username);
        acc.privilege = privilege;
        int pos = account_storage.write(acc);
        account_index.insert(acc.userID, pos);
        int count;
        account_storage.get_info(count, 0);
        account_storage.write_info(count + 1, 0);
    } else if (cmd == "delete") {
        if (get_current_privilege() < 7) { print_invalid(); return; }
        string userID;
        ss >> userID;
        if (!is_valid_id(userID, 30)) { print_invalid(); return; }
        auto pos_vec = account_index.find(ID(userID));
        if (pos_vec.empty()) { print_invalid(); return; }
        // check if logged in
        stack<LoginSession> temp = login_stack;
        while (!temp.empty()) {
            if (temp.top().userID == ID(userID)) { print_invalid(); return; }
            temp.pop();
        }
        account_index.remove(ID(userID), pos_vec[0]);
    } else if (cmd == "show") {
        if (get_current_privilege() < 1) { print_invalid(); return; }
        string arg;
        ss >> arg;
        if (arg == "finance") {
            if (get_current_privilege() < 7) { print_invalid(); return; }
            string count_str;
            if (ss >> count_str) {
                try {
                    int count = stoi(count_str);
                    show_finance(count);
                } catch (...) { print_invalid(); }
            } else show_finance();
        } else {
            vector<int> results;
            if (arg.empty()) {
                results = book_isbn_index.get_all();
            } else {
                size_t eq = arg.find('=');
                if (eq == string::npos) { print_invalid(); return; }
                string key = arg.substr(0, eq);
                string val_with_quotes = line.substr(line.find('=') + 1);
                // Simple trimming of spaces at end
                val_with_quotes.erase(val_with_quotes.find_last_not_of(" ") + 1);

                string val;
                if (key == "-ISBN") {
                    val = val_with_quotes;
                    if (!is_valid_string_param(val, 20)) { print_invalid(); return; }
                    results = book_isbn_index.find(ISBN(val));
                } else if (key == "-name") {
                    val = extract_quoted(val_with_quotes);
                    if (!is_valid_string_param(val, 60)) { print_invalid(); return; }
                    results = book_name_index.find(Name(val));
                } else if (key == "-author") {
                    val = extract_quoted(val_with_quotes);
                    if (!is_valid_string_param(val, 60)) { print_invalid(); return; }
                    results = book_author_index.find(Author(val));
                } else if (key == "-keyword") {
                    val = extract_quoted(val_with_quotes);
                    if (!is_valid_string_param(val, 60) || val.find('|') != string::npos) { print_invalid(); return; }
                    results = book_keyword_index.find(Keyword(val));
                } else { print_invalid(); return; }
            }
            set<pair<ISBN, int>> sorted_results;
            for (int pos : results) {
                Book b;
                book_storage.read(b, pos);
                sorted_results.insert({b.isbn, pos});
            }
            if (sorted_results.empty()) cout << endl;
            else {
                for (auto &p : sorted_results) {
                    Book b;
                    book_storage.read(b, p.second);
                    print_book(b);
                }
            }
        }
    } else if (cmd == "buy") {
        if (get_current_privilege() < 1) { print_invalid(); return; }
        string isbn;
        long long quantity;
        if (!(ss >> isbn >> quantity) || quantity <= 0) { print_invalid(); return; }
        auto pos_vec = book_isbn_index.find(ISBN(isbn));
        if (pos_vec.empty()) { print_invalid(); return; }
        Book b;
        book_storage.read(b, pos_vec[0]);
        if (b.stock < quantity) { print_invalid(); return; }
        b.stock -= quantity;
        book_storage.update(b, pos_vec[0]);
        double total = b.price * quantity;
        cout << fixed << setprecision(2) << total << endl;
        add_finance(total, 0);
    } else if (cmd == "select") {
        if (get_current_privilege() < 3) { print_invalid(); return; }
        string isbn;
        if (!(ss >> isbn) || !is_valid_string_param(isbn, 20)) { print_invalid(); return; }
        auto pos_vec = book_isbn_index.find(ISBN(isbn));
        int pos;
        if (pos_vec.empty()) {
            Book b;
            b.isbn = ISBN(isbn);
            pos = book_storage.write(b);
            book_isbn_index.insert(b.isbn, pos);
        } else {
            pos = pos_vec[0];
        }
        login_stack.top().selected_book_pos = pos;
    } else if (cmd == "modify") {
        if (get_current_privilege() < 3) { print_invalid(); return; }
        if (login_stack.top().selected_book_pos == -1) { print_invalid(); return; }

        string rem;
        getline(ss, rem);
        if (rem.empty()) { print_invalid(); return; }

        map<string, string> params;
        stringstream rss(rem);
        string arg;
        while (rss >> arg) {
            size_t eq = arg.find('=');
            if (eq == string::npos) { print_invalid(); return; }
            string key = arg.substr(0, eq);
            string val = arg.substr(eq + 1);
            if (val.front() == '"') {
                while (val.back() != '"') {
                    string next;
                    if (!(rss >> next)) break;
                    val += " " + next;
                }
            }
            if (params.count(key)) { print_invalid(); return; }
            params[key] = val;
        }
        if (params.empty()) { print_invalid(); return; }

        Book b;
        book_storage.read(b, login_stack.top().selected_book_pos);
        Book old_b = b;

        if (params.count("-ISBN")) {
            string new_isbn = params["-ISBN"];
            if (!is_valid_string_param(new_isbn, 20) || new_isbn == b.isbn.to_string() || book_isbn_index.find(ISBN(new_isbn)).size() > 0) { print_invalid(); return; }
            b.isbn = ISBN(new_isbn);
        }
        if (params.count("-name")) {
            string name = extract_quoted(params["-name"]);
            if (!is_valid_string_param(name, 60)) { print_invalid(); return; }
            b.name = Name(name);
        }
        if (params.count("-author")) {
            string author = extract_quoted(params["-author"]);
            if (!is_valid_string_param(author, 60)) { print_invalid(); return; }
            b.author = Author(author);
        }
        if (params.count("-keyword")) {
            string keyword = extract_quoted(params["-keyword"]);
            if (!is_valid_string_param(keyword, 60)) { print_invalid(); return; }
            auto k_list = split_keywords(keyword);
            if (k_list.empty()) { print_invalid(); return; }
            set<string> unique_k(k_list.begin(), k_list.end());
            if (unique_k.size() != k_list.size()) { print_invalid(); return; }
            b.keyword = Keyword(keyword);
        }
        if (params.count("-price")) {
            try { b.price = stod(params["-price"]); } catch (...) { print_invalid(); return; }
        }

        // Update indices
        if (old_b.isbn != b.isbn) {
            book_isbn_index.remove(old_b.isbn, login_stack.top().selected_book_pos);
            book_isbn_index.insert(b.isbn, login_stack.top().selected_book_pos);
        }
        if (old_b.name != b.name) {
            if (old_b.name.to_string() != "") book_name_index.remove(old_b.name, login_stack.top().selected_book_pos);
            book_name_index.insert(b.name, login_stack.top().selected_book_pos);
        }
        if (old_b.author != b.author) {
            if (old_b.author.to_string() != "") book_author_index.remove(old_b.author, login_stack.top().selected_book_pos);
            book_author_index.insert(b.author, login_stack.top().selected_book_pos);
        }
        if (old_b.keyword != b.keyword) {
            auto old_k = split_keywords(old_b.keyword.to_string());
            for (auto &k : old_k) book_keyword_index.remove(Keyword(k), login_stack.top().selected_book_pos);
            auto new_k = split_keywords(b.keyword.to_string());
            for (auto &k : new_k) book_keyword_index.insert(Keyword(k), login_stack.top().selected_book_pos);
        }
        book_storage.update(b, login_stack.top().selected_book_pos);
    } else if (cmd == "import") {
        if (get_current_privilege() < 3) { print_invalid(); return; }
        if (login_stack.top().selected_book_pos == -1) { print_invalid(); return; }
        long long quantity;
        double total_cost;
        if (!(ss >> quantity >> total_cost) || quantity <= 0 || total_cost <= 0) { print_invalid(); return; }
        Book b;
        book_storage.read(b, login_stack.top().selected_book_pos);
        b.stock += quantity;
        book_storage.update(b, login_stack.top().selected_book_pos);
        add_finance(0, total_cost);
    } else if (cmd == "log" || cmd == "report") {
        if (get_current_privilege() < 7) { print_invalid(); return; }
        // Placeholder
    } else {
        print_invalid();
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    initialize();
    string line;
    while (getline(cin, line)) {
        handle_command(line);
    }
    return 0;
}
