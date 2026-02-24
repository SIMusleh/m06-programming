/* Program name: main.cpp
 * Author: Sahar Musleh
 * Date last updated: 02/23/2026
 * Purpose: Adding Enter a Rental to my Starter code
 */

#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

// Utility callback to collect rows
static int collectRows(void *data, int argc, char **argv, char **azColName) {
    auto *rows = static_cast<vector<vector<string>>*>(data);
    vector<string> row;
    for(int i = 0; i < argc; i++) {
        row.push_back(argv[i] ? argv[i] : "NULL");
    }
    rows->push_back(row);
    return 0;
}

// Run query and return rows
vector<vector<string>> runQuery(sqlite3* db, const string& sql) {
    vector<vector<string>> rows;
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), collectRows, &rows, &errMsg);
    if(rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    return rows;
}

// Paging menu
int chooseWithPaging(sqlite3* db, const string& sql, const string& label, int pageSize=10) {
    int offset = 0;
    while(true) {
        string pagedSQL = sql + " LIMIT " + to_string(pageSize) + " OFFSET " + to_string(offset) + ";";
        auto rows = runQuery(db, pagedSQL);
        if(rows.empty()) {
            cout << "No more " << label << "s." << endl;
            return -1;
        }
        cout << "Please choose the " << label << " (enter 0 for next page):" << endl;
        for(size_t i=0; i<rows.size(); i++) {
            cout << (i+1) << ". " << rows[i][0] << " - " << rows[i][1] << " " << rows[i][2] << endl;
        }
        int choice;
        cin >> choice;
        if(choice == 0) {
            offset += pageSize;
        } else if(choice > 0 && choice <= (int)rows.size()) {
            return stoi(rows[choice-1][0]); // return id
        } else {
            cout << "Invalid choice." << endl;
        }
    }
}

// Enter rental transaction
bool enterRental(sqlite3* db, int customerId, int filmId, int staffId) {
    char *errMsg = nullptr;
    int rc;

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK) { cerr << "Begin failed: " << errMsg << endl; sqlite3_free(errMsg); return false; }

    string rentalSQL = "INSERT INTO rental (rental_date, inventory_id, customer_id, staff_id) "
                       "VALUES (datetime('now'), " + to_string(filmId) + ", " +
                       to_string(customerId) + ", " + to_string(staffId) + ");";
    rc = sqlite3_exec(db, rentalSQL.c_str(), nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK) { cerr << "Rental insert failed: " << errMsg << endl; sqlite3_exec(db,"ROLLBACK;",nullptr,nullptr,nullptr); sqlite3_free(errMsg); return false; }

    string paymentSQL = "INSERT INTO payment (customer_id, staff_id, rental_id, amount, payment_date) "
                        "VALUES (" + to_string(customerId) + ", " + to_string(staffId) +
                        ", last_insert_rowid(), 5.99, datetime('now'));";
    rc = sqlite3_exec(db, paymentSQL.c_str(), nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK) { cerr << "Payment insert failed: " << errMsg << endl; sqlite3_exec(db,"ROLLBACK;",nullptr,nullptr,nullptr); sqlite3_free(errMsg); return false; }

    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK) { cerr << "Commit failed: " << errMsg << endl; sqlite3_free(errMsg); return false; }

    cout << "Rental and Payment entered successfully." << endl;
    return true;
}

int main() {
    sqlite3 *db;
    int rc = sqlite3_open("sakila.db", &db);
    if(rc) { cerr << "Can't open database: " << sqlite3_errmsg(db) << endl; return 1; }

    cout << "Welcome to Sakila" << endl;
    int choice;
    do {
        cout << "Please choose an option (enter -1 to quit):" << endl;
        cout << "1. View the rentals for a customer" << endl;
        cout << "2. View Customer Information" << endl;
        cout << "3. Enter a Rental" << endl;
        cin >> choice;

        if(choice == 1) {
            int custId = chooseWithPaging(db, "SELECT customer_id, first_name, last_name FROM customer", "customer");
            if(custId > 0) {
                auto rows = runQuery(db, "SELECT * FROM rental WHERE customer_id=" + to_string(custId) + ";");
                for(auto &r : rows) {
                    for(auto &c : r) cout << c << " "; cout << endl;
                }
            }
        } else if(choice == 2) {
            int custId = chooseWithPaging(db, "SELECT customer_id, first_name, last_name FROM customer", "customer");
            if(custId > 0) {
                auto rows = runQuery(db, "SELECT * FROM customer WHERE customer_id=" + to_string(custId) + ";");
                for(auto &r : rows) { for(auto &c : r) cout << c << " "; cout << endl; }
            }
        } else if(choice == 3) {
            int custId = chooseWithPaging(db, "SELECT customer_id, first_name, last_name FROM customer", "customer");
            int filmId = chooseWithPaging(db, "SELECT film_id, title, '' FROM film", "film");
            auto staffRows = runQuery(db, "SELECT staff_id, first_name, last_name FROM staff;");
            cout << "Please choose the staff member logging the transaction:" << endl;
            for(size_t i=0; i<staffRows.size(); i++) {
                cout << (i+1) << ". " << staffRows[i][0] << " - " << staffRows[i][1] << " " << staffRows[i][2] << endl;
            }
            int staffChoice; cin >> staffChoice;
            int staffId = stoi(staffRows[staffChoice-1][0]);

            enterRental(db, custId, filmId, staffId);
        }
    } while(choice != -1);

    sqlite3_close(db);
    return 0;
}