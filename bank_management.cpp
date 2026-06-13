/*
    BANK ACCOUNT MANAGEMENT SYSTEM
    Language: C++ (STL, File Handling, OOP, Exception Handling)
    Features:
        - Create Account
        - Login (Account No + PIN, 3 attempts then lockout)
        - Deposit / Withdraw / Check Balance
        - Transfer Money
        - Transaction History (last 5 per account)
        - Update Account Holder Name
        - Close Account
        - Admin Panel (view all accounts)
    Data Persistence:
        - accounts.txt
        - transactions.txt
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <stdexcept>
#include <algorithm>

using namespace std;

const string ACCOUNTS_FILE = "accounts.txt";
const string TRANSACTIONS_FILE = "transactions.txt";
const string ADMIN_PASSWORD = "admin123";
const int MAX_LOGIN_ATTEMPTS = 3;

// ---------------------- Custom Exceptions ----------------------
class InsufficientBalanceException : public runtime_error {
public:
    InsufficientBalanceException(const string& msg) : runtime_error(msg) {}
};

class InvalidAmountException : public runtime_error {
public:
    InvalidAmountException(const string& msg) : runtime_error(msg) {}
};

class AccountNotFoundException : public runtime_error {
public:
    AccountNotFoundException(const string& msg) : runtime_error(msg) {}
};

// ---------------------- Account Class ----------------------
class Account {
private:
    int accountNo;
    string name;
    string pin;        // stored as string to allow leading zeros
    double balance;

public:
    Account() : accountNo(0), name(""), pin(""), balance(0.0) {}

    Account(int accNo, const string& holderName, const string& pinCode, double bal)
        : accountNo(accNo), name(holderName), pin(pinCode), balance(bal) {}

    // Getters
    int getAccountNo() const { return accountNo; }
    string getName() const { return name; }
    string getPin() const { return pin; }
    double getBalance() const { return balance; }

    // Setters
    void setName(const string& newName) { name = newName; }
    void setPin(const string& newPin) { pin = newPin; }

    void deposit(double amount) {
        if (amount <= 0)
            throw InvalidAmountException("Deposit amount must be positive.");
        balance += amount;
    }

    void withdraw(double amount) {
        if (amount <= 0)
            throw InvalidAmountException("Withdrawal amount must be positive.");
        if (amount > balance)
            throw InsufficientBalanceException("Insufficient balance for this withdrawal.");
        balance -= amount;
    }

    bool verifyPin(const string& enteredPin) const {
        return pin == enteredPin;
    }

    // Serialize for file storage: accNo|name|pin|balance
    string toFileString() const {
        ostringstream oss;
        oss << accountNo << "|" << name << "|" << pin << "|" << balance;
        return oss.str();
    }

    // Deserialize from file line
    static Account fromFileString(const string& line) {
        stringstream ss(line);
        string token;
        vector<string> parts;

        while (getline(ss, token, '|')) {
            parts.push_back(token);
        }

        if (parts.size() != 4)
            throw runtime_error("Corrupted account record.");

        int accNo = stoi(parts[0]);
        string nm = parts[1];
        string pn = parts[2];
        double bal = stod(parts[3]);

        return Account(accNo, nm, pn, bal);
    }
};

// ---------------------- Bank Class ----------------------
class Bank {
private:
    vector<Account> accounts;
    int nextAccountNo;

    int findAccountIndex(int accNo) const {
        for (size_t i = 0; i < accounts.size(); i++) {
            if (accounts[i].getAccountNo() == accNo)
                return static_cast<int>(i);
        }
        return -1;
    }

    string getCurrentTimestamp() const {
        time_t now = time(0);
        char buf[80];
        struct tm* timeinfo = localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
        return string(buf);
    }

    void logTransaction(int accNo, const string& type, double amount, double balanceAfter) {
        ofstream out(TRANSACTIONS_FILE, ios::app);
        if (!out) {
            cerr << "Warning: could not open transaction log.\n";
            return;
        }
        out << getCurrentTimestamp() << "|" << accNo << "|" << type
            << "|" << amount << "|" << balanceAfter << "\n";
        out.close();
    }

public:
    Bank() : nextAccountNo(1001) {
        loadAccounts();
    }

    ~Bank() {
        saveAccounts();
    }

    void loadAccounts() {
        ifstream in(ACCOUNTS_FILE);
        if (!in) return; // no file yet, fresh start

        string line;
        int maxAcc = 1000;
        while (getline(in, line)) {
            if (line.empty()) continue;
            try {
                Account acc = Account::fromFileString(line);
                accounts.push_back(acc);
                if (acc.getAccountNo() > maxAcc)
                    maxAcc = acc.getAccountNo();
            } catch (const exception& e) {
                cerr << "Skipping corrupted record: " << e.what() << "\n";
            }
        }
        nextAccountNo = maxAcc + 1;
        in.close();
    }

    void saveAccounts() const {
        ofstream out(ACCOUNTS_FILE, ios::trunc);
        if (!out) {
            cerr << "Error: could not save accounts to file.\n";
            return;
        }
        for (const auto& acc : accounts) {
            out << acc.toFileString() << "\n";
        }
        out.close();
    }

    // Create a new account, returns the generated account number
    int createAccount(const string& name, const string& pin, double initialDeposit) {
        if (initialDeposit < 0)
            throw InvalidAmountException("Initial deposit cannot be negative.");
        if (pin.length() != 4)
            throw InvalidAmountException("PIN must be exactly 4 digits.");

        Account newAcc(nextAccountNo, name, pin, initialDeposit);
        accounts.push_back(newAcc);
        logTransaction(nextAccountNo, "ACCOUNT_CREATED", initialDeposit, initialDeposit);
        saveAccounts();
        int createdAccNo = nextAccountNo;
        nextAccountNo++;
        return createdAccNo;
    }

    // Returns pointer to account if login successful, else nullptr
    Account* login(int accNo, const string& pin, int& attemptsLeft) {
        int idx = findAccountIndex(accNo);
        if (idx == -1) {
            throw AccountNotFoundException("No account exists with this account number.");
        }

        if (accounts[idx].verifyPin(pin)) {
            attemptsLeft = MAX_LOGIN_ATTEMPTS;
            return &accounts[idx];
        } else {
            attemptsLeft--;
            return nullptr;
        }
    }

    void deposit(int accNo, double amount) {
        int idx = findAccountIndex(accNo);
        if (idx == -1)
            throw AccountNotFoundException("Account not found.");

        accounts[idx].deposit(amount);
        logTransaction(accNo, "DEPOSIT", amount, accounts[idx].getBalance());
        saveAccounts();
    }

    void withdraw(int accNo, double amount) {
        int idx = findAccountIndex(accNo);
        if (idx == -1)
            throw AccountNotFoundException("Account not found.");

        accounts[idx].withdraw(amount); // throws if insufficient
        logTransaction(accNo, "WITHDRAW", amount, accounts[idx].getBalance());
        saveAccounts();
    }

    void transfer(int fromAcc, int toAcc, double amount) {
        int fromIdx = findAccountIndex(fromAcc);
        int toIdx = findAccountIndex(toAcc);

        if (fromIdx == -1)
            throw AccountNotFoundException("Source account not found.");
        if (toIdx == -1)
            throw AccountNotFoundException("Destination account not found.");
        if (fromAcc == toAcc)
            throw InvalidAmountException("Cannot transfer to the same account.");

        accounts[fromIdx].withdraw(amount); // throws if insufficient
        accounts[toIdx].deposit(amount);

        logTransaction(fromAcc, "TRANSFER_OUT", amount, accounts[fromIdx].getBalance());
        logTransaction(toAcc, "TRANSFER_IN", amount, accounts[toIdx].getBalance());
        saveAccounts();
    }

    double checkBalance(int accNo) const {
        int idx = findAccountIndex(accNo);
        if (idx == -1)
            throw AccountNotFoundException("Account not found.");
        return accounts[idx].getBalance();
    }

    void updateName(int accNo, const string& newName) {
        int idx = findAccountIndex(accNo);
        if (idx == -1)
            throw AccountNotFoundException("Account not found.");
        accounts[idx].setName(newName);
        saveAccounts();
    }

    void closeAccount(int accNo) {
        int idx = findAccountIndex(accNo);
        if (idx == -1)
            throw AccountNotFoundException("Account not found.");

        logTransaction(accNo, "ACCOUNT_CLOSED", accounts[idx].getBalance(), 0);
        accounts.erase(accounts.begin() + idx);
        saveAccounts();
    }

    // Returns up to last N transactions for a given account
    vector<string> getRecentTransactions(int accNo, int count) const {
        vector<string> result;
        ifstream in(TRANSACTIONS_FILE);
        if (!in) return result;

        vector<string> allLines;
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string accStr;
            getline(ss, accStr, '|'); // timestamp -- skip
            getline(ss, accStr, '|'); // account number
            try {
                if (stoi(accStr) == accNo) {
                    allLines.push_back(line);
                }
            } catch (...) {
                continue;
            }
        }
        in.close();

        int total = static_cast<int>(allLines.size());
        int start = max(0, total - count);
        for (int i = start; i < total; i++) {
            result.push_back(allLines[i]);
        }
        return result;
    }

    const vector<Account>& getAllAccounts() const {
        return accounts;
    }

    Account* getAccountPtr(int accNo) {
        int idx = findAccountIndex(accNo);
        if (idx == -1) return nullptr;
        return &accounts[idx];
    }
};

// ---------------------- Helper Input Functions ----------------------
double getValidatedDouble(const string& prompt) {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            return val;
        } else {
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            cin.ignore(10000, '\n');
        }
    }
}

int getValidatedInt(const string& prompt) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            return val;
        } else {
            cout << "Invalid input. Please enter a valid integer.\n";
            cin.clear();
            cin.ignore(10000, '\n');
        }
    }
}

string getValidatedPin() {
    string pin;
    while (true) {
        cout << "Set a 4-digit PIN: ";
        cin >> pin;
        if (pin.length() == 4 && all_of(pin.begin(), pin.end(), ::isdigit)) {
            return pin;
        }
        cout << "PIN must be exactly 4 digits.\n";
    }
}

// ---------------------- Print Helpers ----------------------
void printTransactionLine(const string& line) {
    // format: timestamp|accNo|type|amount|balanceAfter
    stringstream ss(line);
    string timestamp, accNo, type, amount, balAfter;
    getline(ss, timestamp, '|');
    getline(ss, accNo, '|');
    getline(ss, type, '|');
    getline(ss, amount, '|');
    getline(ss, balAfter, '|');

    cout << "  [" << timestamp << "] " << type
         << " | Amount: " << amount
         << " | Balance After: " << balAfter << "\n";
}

// ---------------------- Menu Functions ----------------------
void customerMenu(Bank& bank, int accNo) {
    int choice;
    do {
        cout << "\n----- Customer Menu (Account #" << accNo << ") -----\n";
        cout << "1. Check Balance\n";
        cout << "2. Deposit\n";
        cout << "3. Withdraw\n";
        cout << "4. Transfer Funds\n";
        cout << "5. View Recent Transactions\n";
        cout << "6. Update Name\n";
        cout << "7. Close Account\n";
        cout << "8. Logout\n";
        choice = getValidatedInt("Enter your choice: ");

        try {
            switch (choice) {
                case 1: {
                    double bal = bank.checkBalance(accNo);
                    cout << "Current Balance: Rs. " << bal << "\n";
                    break;
                }
                case 2: {
                    double amt = getValidatedDouble("Enter amount to deposit: Rs. ");
                    bank.deposit(accNo, amt);
                    cout << "Deposit successful. New Balance: Rs. "
                         << bank.checkBalance(accNo) << "\n";
                    break;
                }
                case 3: {
                    double amt = getValidatedDouble("Enter amount to withdraw: Rs. ");
                    bank.withdraw(accNo, amt);
                    cout << "Withdrawal successful. New Balance: Rs. "
                         << bank.checkBalance(accNo) << "\n";
                    break;
                }
                case 4: {
                    int toAcc = getValidatedInt("Enter destination account number: ");
                    double amt = getValidatedDouble("Enter amount to transfer: Rs. ");
                    bank.transfer(accNo, toAcc, amt);
                    cout << "Transfer successful. New Balance: Rs. "
                         << bank.checkBalance(accNo) << "\n";
                    break;
                }
                case 5: {
                    cout << "\nLast 5 transactions:\n";
                    vector<string> txns = bank.getRecentTransactions(accNo, 5);
                    if (txns.empty()) {
                        cout << "  No transactions found.\n";
                    } else {
                        for (const auto& t : txns) printTransactionLine(t);
                    }
                    break;
                }
                case 6: {
                    cin.ignore();
                    string newName;
                    cout << "Enter new name: ";
                    getline(cin, newName);
                    bank.updateName(accNo, newName);
                    cout << "Name updated successfully.\n";
                    break;
                }
                case 7: {
                    char confirm;
                    cout << "Are you sure you want to close this account? (y/n): ";
                    cin >> confirm;
                    if (confirm == 'y' || confirm == 'Y') {
                        bank.closeAccount(accNo);
                        cout << "Account closed successfully.\n";
                        return; // exit customer menu, account no longer exists
                    } else {
                        cout << "Account closure cancelled.\n";
                    }
                    break;
                }
                case 8:
                    cout << "Logging out...\n";
                    break;
                default:
                    cout << "Invalid choice. Try again.\n";
            }
        } catch (const InsufficientBalanceException& e) {
            cout << "Error: " << e.what() << "\n";
        } catch (const InvalidAmountException& e) {
            cout << "Error: " << e.what() << "\n";
        } catch (const AccountNotFoundException& e) {
            cout << "Error: " << e.what() << "\n";
            return;
        } catch (const exception& e) {
            cout << "Unexpected error: " << e.what() << "\n";
        }

    } while (choice != 8);
}

void adminMenu(Bank& bank) {
    int choice;
    do {
        cout << "\n----- Admin Menu -----\n";
        cout << "1. View All Accounts\n";
        cout << "2. View Account Details by Account Number\n";
        cout << "3. Logout\n";
        choice = getValidatedInt("Enter your choice: ");

        switch (choice) {
            case 1: {
                const auto& accounts = bank.getAllAccounts();
                if (accounts.empty()) {
                    cout << "No accounts found.\n";
                    break;
                }
                cout << "\n" << "Acc No.  | Name                 | Balance\n";
                cout << "---------------------------------------------\n";
                for (const auto& acc : accounts) {
                    cout << acc.getAccountNo() << "     | "
                         << acc.getName();
                    int padding = 20 - static_cast<int>(acc.getName().length());
                    for (int i = 0; i < max(1, padding); i++) cout << " ";
                    cout << "| Rs. " << acc.getBalance() << "\n";
                }
                break;
            }
            case 2: {
                int accNo = getValidatedInt("Enter account number: ");
                try {
                    double bal = bank.checkBalance(accNo);
                    Account* acc = bank.getAccountPtr(accNo);
                    cout << "Account No: " << accNo << "\n";
                    cout << "Name: " << acc->getName() << "\n";
                    cout << "Balance: Rs. " << bal << "\n";
                    cout << "Recent Transactions:\n";
                    vector<string> txns = bank.getRecentTransactions(accNo, 5);
                    for (const auto& t : txns) printTransactionLine(t);
                } catch (const AccountNotFoundException& e) {
                    cout << "Error: " << e.what() << "\n";
                }
                break;
            }
            case 3:
                cout << "Logging out of admin panel...\n";
                break;
            default:
                cout << "Invalid choice.\n";
        }
    } while (choice != 3);
}

// ---------------------- Main ----------------------
int main() {
    Bank bank;
    int mainChoice;

    cout << "=============================================\n";
    cout << "      WELCOME TO C++ BANK MANAGEMENT SYSTEM   \n";
    cout << "=============================================\n";

    do {
        cout << "\n----- Main Menu -----\n";
        cout << "1. Create New Account\n";
        cout << "2. Login to Account\n";
        cout << "3. Admin Login\n";
        cout << "4. Exit\n";
        mainChoice = getValidatedInt("Enter your choice: ");

        switch (mainChoice) {
            case 1: {
                cin.ignore();
                string name;
                cout << "Enter account holder name: ";
                getline(cin, name);

                string pin = getValidatedPin();
                double initialDeposit = getValidatedDouble("Enter initial deposit amount: Rs. ");

                try {
                    int accNo = bank.createAccount(name, pin, initialDeposit);
                    cout << "\nAccount created successfully!\n";
                    cout << "Your Account Number is: " << accNo << "\n";
                    cout << "Please remember this number along with your PIN.\n";
                } catch (const InvalidAmountException& e) {
                    cout << "Error: " << e.what() << "\n";
                }
                break;
            }
            case 2: {
                int accNo = getValidatedInt("Enter account number: ");
                int attemptsLeft = MAX_LOGIN_ATTEMPTS;
                bool loggedIn = false;

                try {
                    while (attemptsLeft > 0) {
                        string pin;
                        cout << "Enter 4-digit PIN: ";
                        cin >> pin;

                        Account* acc = bank.login(accNo, pin, attemptsLeft);
                        if (acc != nullptr) {
                            cout << "\nLogin successful. Welcome, " << acc->getName() << "!\n";
                            customerMenu(bank, accNo);
                            loggedIn = true;
                            break;
                        } else {
                            if (attemptsLeft > 0) {
                                cout << "Incorrect PIN. Attempts left: " << attemptsLeft << "\n";
                            }
                        }
                    }

                    if (!loggedIn && attemptsLeft == 0) {
                        cout << "\nToo many incorrect attempts. Account locked for this session.\n";
                        cout << "Please try again later or contact admin.\n";
                    }
                } catch (const AccountNotFoundException& e) {
                    cout << "Error: " << e.what() << "\n";
                }
                break;
            }
            case 3: {
                string pwd;
                cout << "Enter admin password: ";
                cin >> pwd;
                if (pwd == ADMIN_PASSWORD) {
                    adminMenu(bank);
                } else {
                    cout << "Incorrect admin password.\n";
                }
                break;
            }
            case 4:
                cout << "Thank you for using the Bank Management System. Goodbye!\n";
                break;
            default:
                cout << "Invalid choice. Please try again.\n";
        }

    } while (mainChoice != 4);

    return 0;
}
