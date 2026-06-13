# Bank Account Management System

A console-based banking application built in **C++** demonstrating Object-Oriented Programming, file handling, and exception handling.

## Features

- **Create Account** – auto-generates a unique account number with a 4-digit PIN
- **Secure Login** – PIN-based authentication with a 3-attempt lockout for security
- **Deposit / Withdraw** – with validation and custom exception handling
- **Fund Transfer** – transfer money between accounts
- **Transaction History** – view last 5 transactions with timestamps
- **Update Account Details** – edit account holder name
- **Close Account** – remove account from records
- **Admin Panel** – password-protected view of all accounts and their details

## Concepts Demonstrated

- Object-Oriented Programming (Classes, Encapsulation, Constructors)
- STL (Vectors)
- File Handling (persistent storage using `accounts.txt` and `transactions.txt`)
- Custom Exception Handling (`InsufficientBalanceException`, `InvalidAmountException`, `AccountNotFoundException`)
- Basic security practices (PIN authentication, login attempt lockout)

## How to Compile and Run

```bash
g++ -std=c++17 bank_management.cpp -o bank_management
./bank_management
```

On Windows (using g++/MinGW):

```bash
g++ -std=c++17 bank_management.cpp -o bank_management.exe
bank_management.exe
```

## Files Generated at Runtime

- `accounts.txt` – stores account records (account number, name, PIN, balance)
- `transactions.txt` – logs every transaction with a timestamp

## Default Credentials

- **Admin Password:** `admin123`

## Tech Stack

- Language: C++ (C++17)
- Libraries: Standard Template Library (STL)

## Author

[Rahul Mishra]
