# Lab Access Verification System

A simple **C++ Data Structures and Algorithms micro-project** that simulates access verification for a high-security research laboratory.

The system manages researchers, processes access requests, verifies permissions, handles emergency lockdowns, and maintains recent access records.

## Features

* Researcher authorization management
* Hash-based researcher lookup
* Linked List for data organization
* Queue for access requests
* Stack for recent access events
* Linear Search
* Binary Search
* Sorting
* Emergency zone lockdown
* Credential and permission updates
* Basic testing

## DSA Concepts Used

| Concept            | Purpose                       |
| ------------------ | ----------------------------- |
| Array / Hash Table | Researcher authorization      |
| Linked List        | Dynamic data organization     |
| Queue              | Access request processing     |
| Stack              | Recent access events          |
| Linear Search      | Researcher searching          |
| Binary Search      | Fast searching in sorted data |
| Sorting            | Organizing researcher records |

## How It Works

```text
Access Request
      ↓
Queue
      ↓
Find Researcher
      ↓
Verify Credential & Permission
      ↓
Check Zone / Lockdown
      ↓
Grant or Deny Access
      ↓
Store Access Event
```

## Requirements

* C++ compiler
* C++17 or later
* No external libraries required

## Run

Compile:

```bash
g++ -std=c++17 main.cpp -o lab_access
```

Run:

```bash
./lab_access
```

On Windows:

```bash
lab_access.exe
```

## Project Purpose

This project demonstrates how basic **Data Structures and Algorithms** can be applied to a real-world laboratory access-control problem while keeping the implementation simple and easy to understand.
