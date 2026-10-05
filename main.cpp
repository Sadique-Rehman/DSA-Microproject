// ============================================================================
//  LAB ACCESS VERIFICATION SYSTEM
//  Academic DSA micro-project (C++, console only, no external libraries)
//
//  Compile:  g++ main.cpp -o lab
//  Run:      ./lab        (Windows: lab.exe)
//
//  WHERE EACH DATA STRUCTURE / ALGORITHM IS USED
//  ---------------------------------------------------------------------------
//  Array        -> researchers[]        stores all researcher records
//  Hash table   -> HashTable            fast lookup of a researcher by ID
//  Linked list  -> Node chain           handles hash collisions (chaining)
//  Queue        -> Queue                pending access requests   (FIFO)
//  Stack        -> Stack                recent access events      (LIFO)
//  Linear search-> linearSearch()       O(n)      works on unsorted data
//  Binary search-> binarySearch()       O(log n)  needs data sorted by ID
//  Bubble sort  -> bubbleSortById()     O(n^2)    sorts by ID
//  Selection sort-> selectionSortByName() O(n^2)  sorts by name
//  bool array   -> labLocked[]          emergency lockdown per lab zone
// ============================================================================

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cctype>
using namespace std;

// ----------------------------------------------------------------------------
//  CONSTANTS AND SIMPLE GLOBAL DATA
// ----------------------------------------------------------------------------
const int MAX_RESEARCHERS = 50;
const int TABLE_SIZE = 7;       // small on purpose, so collisions are visible
const int MAX_REQUESTS = 50;
const int MAX_EVENTS = 100;
const int NUM_ZONES = 4;

string zoneNames[NUM_ZONES] = {"LAB-A", "LAB-B", "LAB-C", "LAB-D"};
int zoneRequiredLevel[NUM_ZONES] = {1, 2, 3, 4};   // minimum access level per zone
bool labLocked[NUM_ZONES];                         // true = zone is in lockdown

string methodNames[3] = {"Fingerprint", "NFC", "Controller"};
string checkpointNames[3] = {"Checkpoint A", "Checkpoint B", "Checkpoint C"};

// ----------------------------------------------------------------------------
//  RECORDS (plain structures)
// ----------------------------------------------------------------------------
struct Researcher {
    string id;
    string name;
    int accessLevel;    // 1 (lowest) to 4 (highest)
    bool active;        // false = credential revoked
};

struct AccessRequest {
    string researcherId;
    int zone;           // index into zoneNames[]
    int method;         // index into methodNames[]
    int checkpoint;     // index into checkpointNames[]
};

struct AccessEvent {
    string researcherId;
    int zone;
    bool granted;
    string reason;
};

// ----------------------------------------------------------------------------
//  ARRAY of researchers
// ----------------------------------------------------------------------------
Researcher researchers[MAX_RESEARCHERS];
int researcherCount = 0;
bool sortedById = false;    // binary search is allowed only when this is true

// ----------------------------------------------------------------------------
//  HASH TABLE (separate chaining with a singly linked list)
//
//      buckets[0] -> Node -> Node -> NULL
//      buckets[1] -> NULL
//      buckets[2] -> Node -> NULL
//
//  Each node stores the researcher ID and the INDEX of that researcher
//  in the researchers[] array.
// ----------------------------------------------------------------------------
struct Node {
    string id;
    int index;
    Node* next;
};

// Simple hash function: add up the ASCII codes of the characters, then % size.
int hashFunction(string id) {
    int sum = 0;
    for (int i = 0; i < (int)id.length(); i++) {
        sum += id[i];
    }
    return sum % TABLE_SIZE;
}

struct HashTable {
    Node* buckets[TABLE_SIZE];

    HashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) buckets[i] = NULL;
    }

    // Insert at the HEAD of the bucket's linked list: O(1)
    void insert(string id, int index) {
        Node* n = new Node;
        n->id = id;
        n->index = index;
        int b = hashFunction(id);
        n->next = buckets[b];      // new node points to the old head
        buckets[b] = n;            // new node becomes the head
    }

    // Walk down the chain of ONE bucket. Returns array index, or -1.
    // 'steps' counts how many nodes were checked.
    int find(string id, int &steps) {
        steps = 0;
        Node* current = buckets[hashFunction(id)];
        while (current != NULL) {
            steps++;
            if (current->id == id) return current->index;
            current = current->next;
        }
        return -1;
    }

    // Free every node (needed because we used 'new')
    void clear() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            Node* current = buckets[i];
            while (current != NULL) {
                Node* temp = current;
                current = current->next;
                delete temp;
            }
            buckets[i] = NULL;
        }
    }

    void print() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            cout << "Bucket " << i << ": ";
            Node* current = buckets[i];
            while (current != NULL) {
                cout << current->id << " -> ";
                current = current->next;
            }
            cout << "NULL\n";
        }
    }
};

HashTable table;

// ----------------------------------------------------------------------------
//  QUEUE (array based, circular) - FIFO - stores pending access requests
//
//   enqueue adds at the REAR, dequeue removes from the FRONT.
//   Circular means freed slots at the start of the array are reused.
// ----------------------------------------------------------------------------
struct Queue {
    AccessRequest data[MAX_REQUESTS];
    int front;
    int rear;
    int count;

    Queue() { front = 0; rear = -1; count = 0; }

    bool isEmpty() { return count == 0; }
    bool isFull()  { return count == MAX_REQUESTS; }

    bool enqueue(AccessRequest r) {
        if (isFull()) return false;
        rear = (rear + 1) % MAX_REQUESTS;
        data[rear] = r;
        count++;
        return true;
    }

    bool dequeue(AccessRequest &r) {
        if (isEmpty()) return false;
        r = data[front];
        front = (front + 1) % MAX_REQUESTS;
        count--;
        return true;
    }

    bool peek(AccessRequest &r) {          // look at the front, do not remove
        if (isEmpty()) return false;
        r = data[front];
        return true;
    }

    // Look at the request 'pos' places behind the front (0 = front). No removal.
    bool peekAt(int pos, AccessRequest &r) {
        if (pos < 0 || pos >= count) return false;
        r = data[(front + pos) % MAX_REQUESTS];
        return true;
    }

    void clear() { front = 0; rear = -1; count = 0; }
};

// ----------------------------------------------------------------------------
//  STACK (array based) - LIFO - stores recent access events
//
//   push adds on TOP, pop removes from TOP. The newest event is always on top.
// ----------------------------------------------------------------------------
struct Stack {
    AccessEvent data[MAX_EVENTS];
    int top;

    Stack() { top = -1; }

    bool isEmpty() { return top == -1; }

    void push(AccessEvent e) {
        if (top == MAX_EVENTS - 1) {       // full: drop the oldest event
            for (int i = 1; i <= top; i++) data[i - 1] = data[i];
            top--;
        }
        top++;
        data[top] = e;
    }

    bool pop(AccessEvent &e) {
        if (isEmpty()) return false;
        e = data[top];
        top--;
        return true;
    }

    bool peek(AccessEvent &e) {            // look at the top, do not remove
        if (isEmpty()) return false;
        e = data[top];
        return true;
    }

    // Look 'depth' places below the top (0 = top). No removal.
    bool peekAt(int depth, AccessEvent &e) {
        if (depth < 0 || depth > top) return false;
        e = data[top - depth];
        return true;
    }

    void clear() { top = -1; }
};

Queue pendingRequests;
Stack recentEvents;

// ----------------------------------------------------------------------------
//  SMALL INPUT HELPERS
// ----------------------------------------------------------------------------
string readLine(string prompt) {
    string line;
    while (true) {
        cout << prompt;
        if (!getline(cin, line)) exit(0);          // input ended
        if (line != "") return line;
        cout << "Input cannot be empty.\n";
    }
}

// Remove spaces and make UPPERCASE, so "r101" and "R101" are the same ID
string cleanId(string text) {
    string result = "";
    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] != ' ') result += (char)toupper(text[i]);
    }
    return result;
}

string readId(string prompt) {
    while (true) {
        string id = cleanId(readLine(prompt));
        if (id != "") return id;
        cout << "ID cannot be empty.\n";
    }
}

// Read a whole number between low and high (keeps asking until valid)
int readInt(string prompt, int low, int high) {
    string line;
    while (true) {
        cout << prompt;
        if (!getline(cin, line)) exit(0);
        bool ok = (line != "" && line.length() <= 6);
        for (int i = 0; i < (int)line.length(); i++) {
            if (line[i] < '0' || line[i] > '9') ok = false;
        }
        if (ok) {
            int value = 0;
            for (int i = 0; i < (int)line.length(); i++) {
                value = value * 10 + (line[i] - '0');
            }
            if (value >= low && value <= high) return value;
        }
        cout << "Invalid input. Enter a number from " << low << " to " << high << ".\n";
    }
}

// ----------------------------------------------------------------------------
//  RESEARCHER DATABASE (array + hash table kept in sync)
// ----------------------------------------------------------------------------
void rebuildHashTable() {
    // Sorting moves records to new array positions, so the stored indexes
    // become wrong. We simply rebuild the table from the array.
    table.clear();
    for (int i = 0; i < researcherCount; i++) {
        table.insert(researchers[i].id, i);
    }
}

bool addResearcherRecord(string id, string name, int level, bool active) {
    if (researcherCount >= MAX_RESEARCHERS) return false;
    researchers[researcherCount].id = id;
    researchers[researcherCount].name = name;
    researchers[researcherCount].accessLevel = level;
    researchers[researcherCount].active = active;
    table.insert(id, researcherCount);     // index of the new record
    researcherCount++;
    sortedById = false;                    // new record added at the end
    return true;
}

// Reset EVERYTHING and load 12 sample researchers (deliberately unsorted)
void loadSampleData() {
    researcherCount = 0;
    sortedById = false;
    table.clear();
    pendingRequests.clear();
    recentEvents.clear();
    for (int i = 0; i < NUM_ZONES; i++) labLocked[i] = false;

    addResearcherRecord("R104", "Anita Rao",   3, true);
    addResearcherRecord("R101", "Vikram Singh", 4, true);
    addResearcherRecord("R109", "Meera Nair",  1, true);
    addResearcherRecord("R103", "Rahul Verma", 2, true);
    addResearcherRecord("R107", "Sara Khan",   3, false);   // revoked
    addResearcherRecord("R102", "Arjun Patel", 2, true);
    addResearcherRecord("R110", "Neha Gupta",  1, true);
    addResearcherRecord("R105", "Kiran Das",   4, true);
    addResearcherRecord("R108", "Pooja Iyer",  2, false);   // revoked
    addResearcherRecord("R106", "Dev Mehta",   1, true);
    addResearcherRecord("R111", "Isha Joshi",  3, true);
    addResearcherRecord("R112", "Rohan Shah",  2, true);
}

// ----------------------------------------------------------------------------
//  SEARCHING
// ----------------------------------------------------------------------------

// LINEAR SEARCH: check records one by one from the start. O(n)
int linearSearch(string id, int &steps) {
    steps = 0;
    for (int i = 0; i < researcherCount; i++) {
        steps++;
        if (researchers[i].id == id) return i;
    }
    return -1;
}

// BINARY SEARCH: repeatedly look at the middle and discard half. O(log n)
// REQUIRES researchers[] to be sorted by ID.
int binarySearch(string id, int &steps) {
    steps = 0;
    int low = 0;
    int high = researcherCount - 1;
    while (low <= high) {
        steps++;
        int mid = (low + high) / 2;
        if (researchers[mid].id == id) return mid;
        else if (researchers[mid].id < id) low = mid + 1;    // go right
        else high = mid - 1;                                 // go left
    }
    return -1;
}

// ----------------------------------------------------------------------------
//  SORTING
// ----------------------------------------------------------------------------

// BUBBLE SORT by ID: compare neighbours, swap if out of order. O(n^2)
void bubbleSortById() {
    for (int i = 0; i < researcherCount - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < researcherCount - 1 - i; j++) {
            if (researchers[j].id > researchers[j + 1].id) {
                Researcher temp = researchers[j];
                researchers[j] = researchers[j + 1];
                researchers[j + 1] = temp;
                swapped = true;
            }
        }
        if (!swapped) break;       // already sorted, stop early
    }
    sortedById = true;
    rebuildHashTable();
}

// SELECTION SORT by name: find the smallest, put it in place. O(n^2)
void selectionSortByName() {
    for (int i = 0; i < researcherCount - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < researcherCount; j++) {
            if (researchers[j].name < researchers[minIndex].name) minIndex = j;
        }
        Researcher temp = researchers[i];
        researchers[i] = researchers[minIndex];
        researchers[minIndex] = temp;
    }
    sortedById = false;            // array is now ordered by name, not ID
    rebuildHashTable();
}

// ----------------------------------------------------------------------------
//  ACCESS VERIFICATION
//
//  Receive request -> Find researcher -> Active? -> Zone locked? ->
//  Enough access level? -> GRANT / DENY
// ----------------------------------------------------------------------------
bool checkAccess(AccessRequest req, string &reason) {
    int steps;
    int index = table.find(req.researcherId, steps);       // hash lookup

    if (index == -1) {
        reason = "Researcher not found";
        return false;
    }
    if (!researchers[index].active) {
        reason = "Credential revoked";
        return false;
    }
    if (labLocked[req.zone]) {
        reason = "Zone is locked down";
        return false;
    }
    if (researchers[index].accessLevel < zoneRequiredLevel[req.zone]) {
        reason = "Access level too low for this zone";
        return false;
    }
    reason = "All checks passed";
    return true;
}

// Take the FRONT request out of the queue, decide, and push the event on the stack
void processNextRequest() {
    AccessRequest req;
    if (!pendingRequests.dequeue(req)) {
        cout << "No pending requests.\n";
        return;
    }

    string reason;
    bool granted = checkAccess(req, reason);

    AccessEvent e;
    e.researcherId = req.researcherId;
    e.zone = req.zone;
    e.granted = granted;
    e.reason = reason;
    recentEvents.push(e);

    cout << "[" << checkpointNames[req.checkpoint] << " | " << methodNames[req.method] << "] "
         << req.researcherId << " -> " << zoneNames[req.zone] << " : "
         << (granted ? "GRANTED" : "DENIED") << " (" << reason << ")\n";
}

// ----------------------------------------------------------------------------
//  MENU FUNCTIONS
// ----------------------------------------------------------------------------
void printResearcher(int i) {
    cout << researchers[i].id << " | " << researchers[i].name
         << " | Level " << researchers[i].accessLevel << " | "
         << (researchers[i].active ? "ACTIVE" : "REVOKED") << "\n";
}

void showZones() {
    cout << "\nLab zones:\n";
    for (int i = 0; i < NUM_ZONES; i++) {
        cout << "  " << (i + 1) << ". " << zoneNames[i]
             << " (needs level " << zoneRequiredLevel[i] << ") "
             << (labLocked[i] ? "[LOCKED]" : "[open]") << "\n";
    }
}

// 1
void addResearcher() {
    if (researcherCount >= MAX_RESEARCHERS) {
        cout << "Database is full.\n";
        return;
    }
    string id = readId("Enter new researcher ID (e.g. R113): ");
    int steps;
    if (linearSearch(id, steps) != -1) {
        cout << "That ID already exists.\n";
        return;
    }
    string name = readLine("Enter name: ");
    int level = readInt("Enter access level (1-4): ", 1, 4);
    addResearcherRecord(id, name, level, true);
    cout << "Researcher added.\n";
}

// 2
void viewResearchers() {
    cout << "\nIdx  ID     Name                 Level  Status\n";
    cout << "------------------------------------------------\n";
    for (int i = 0; i < researcherCount; i++) {
        cout << left << setw(5) << i << setw(7) << researchers[i].id
             << setw(21) << researchers[i].name << setw(7) << researchers[i].accessLevel
             << (researchers[i].active ? "ACTIVE" : "REVOKED") << "\n";
    }
    cout << "Total: " << researcherCount << " | Sorted by ID: " << (sortedById ? "YES" : "NO") << "\n";
}

// 3
void searchResearcher() {
    string id = readId("Enter researcher ID to search: ");
    cout << "1. Linear search  (O(n))\n2. Hash table lookup  (O(1) average)\n3. Binary search  (O(log n), needs sorted data)\n";
    int choice = readInt("Choose method: ", 1, 3);

    int steps = 0;
    int index = -1;
    if (choice == 1) {
        index = linearSearch(id, steps);
    } else if (choice == 2) {
        index = table.find(id, steps);
    } else {
        if (!sortedById) {
            cout << "Binary search needs data sorted by ID. Use option 9 first.\n";
            return;
        }
        index = binarySearch(id, steps);
    }

    if (index == -1) {
        cout << "Not found. ";
    } else {
        cout << "Found at index " << index << ": ";
        printResearcher(index);
    }
    cout << "Steps taken: " << steps << "\n";
}

// 4  (any ID is accepted on purpose, so invalid IDs can be tested)
void createAccessRequest() {
    if (pendingRequests.isFull()) {
        cout << "Request queue is full. Process some requests first.\n";
        return;
    }
    AccessRequest r;
    r.researcherId = readId("Enter researcher ID: ");
    showZones();
    r.zone = readInt("Choose zone (1-4): ", 1, NUM_ZONES) - 1;
    cout << "Method: 1. Fingerprint  2. NFC  3. Controller\n";
    r.method = readInt("Choose method (1-3): ", 1, 3) - 1;
    cout << "Checkpoint: 1. A  2. B  3. C\n";
    r.checkpoint = readInt("Choose checkpoint (1-3): ", 1, 3) - 1;

    pendingRequests.enqueue(r);
    cout << "Request added to queue. Pending requests: " << pendingRequests.count << "\n";
}

// 5
void viewPendingRequests() {
    if (pendingRequests.isEmpty()) {
        cout << "No pending requests.\n";
        return;
    }
    cout << "\nPending requests (front of queue is processed first):\n";
    for (int i = 0; i < pendingRequests.count; i++) {
        AccessRequest r;
        pendingRequests.peekAt(i, r);
        cout << "  " << (i + 1) << ". " << r.researcherId << " -> " << zoneNames[r.zone]
             << " [" << checkpointNames[r.checkpoint] << ", " << methodNames[r.method] << "]"
             << (i == 0 ? "  <-- FRONT" : "") << "\n";
    }
}

// 6
void processRequests() {
    if (pendingRequests.isEmpty()) {
        cout << "No pending requests.\n";
        return;
    }
    cout << "1. Process next request\n2. Process all requests\n";
    int choice = readInt("Choose: ", 1, 2);
    if (choice == 1) {
        processNextRequest();
    } else {
        while (!pendingRequests.isEmpty()) processNextRequest();
    }
}

// 7
void emergencyLockdown() {
    showZones();
    cout << "1. Lock one zone\n2. Unlock one zone\n3. LOCK ALL zones\n4. Unlock all zones\n";
    int choice = readInt("Choose: ", 1, 4);
    if (choice == 1 || choice == 2) {
        int zone = readInt("Zone number (1-4): ", 1, NUM_ZONES) - 1;
        labLocked[zone] = (choice == 1);
        cout << zoneNames[zone] << (labLocked[zone] ? " is now LOCKED.\n" : " is now UNLOCKED.\n");
    } else {
        for (int i = 0; i < NUM_ZONES; i++) labLocked[i] = (choice == 3);
        cout << (choice == 3 ? "ALL zones are now LOCKED.\n" : "All zones are now unlocked.\n");
    }
}

// 8
void viewRecentEvents() {
    if (recentEvents.isEmpty()) {
        cout << "No access events yet.\n";
        return;
    }
    cout << "\nRecent events (newest first = top of stack, max 10 shown):\n";
    for (int depth = 0; depth < 10; depth++) {
        AccessEvent e;
        if (!recentEvents.peekAt(depth, e)) break;
        cout << "  " << (depth + 1) << ". " << e.researcherId << " -> " << zoneNames[e.zone]
             << " : " << (e.granted ? "GRANTED" : "DENIED") << " (" << e.reason << ")"
             << (depth == 0 ? "  <-- TOP" : "") << "\n";
    }
}

// 9
void sortResearchers() {
    cout << "1. Bubble sort by ID (enables binary search)\n2. Selection sort by name\n";
    int choice = readInt("Choose: ", 1, 2);
    if (choice == 1) {
        bubbleSortById();
        cout << "Sorted by ID using bubble sort.\n";
    } else {
        selectionSortByName();
        cout << "Sorted by name using selection sort.\n";
    }
    viewResearchers();
}

// 11
void toggleCredential() {
    string id = readId("Enter researcher ID: ");
    int steps;
    int index = table.find(id, steps);
    if (index == -1) {
        cout << "Researcher not found.\n";
        return;
    }
    researchers[index].active = !researchers[index].active;
    cout << id << " credential is now " << (researchers[index].active ? "ACTIVE" : "REVOKED") << ".\n";
}

// 12
void viewHashTable() {
    cout << "\nHash table (size " << TABLE_SIZE << ", separate chaining):\n";
    table.print();
}

// ----------------------------------------------------------------------------
//  TESTS (option 10) - each test really calls the function it checks
// ----------------------------------------------------------------------------
int testsPassed = 0;
int testsTotal = 0;

void check(string name, bool ok) {
    testsTotal++;
    if (ok) testsPassed++;
    cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
}

AccessRequest makeRequest(string id, int zone) {
    AccessRequest r;
    r.researcherId = id;
    r.zone = zone;
    r.method = 0;
    r.checkpoint = 0;
    return r;
}

void runTests() {
    cout << "\nNOTE: tests reset the system to the sample data.\n\n";
    loadSampleData();
    testsPassed = 0;
    testsTotal = 0;
    string reason;
    bool granted;
    int steps;

    // Test 1: valid access (R101 has level 4, LAB-D needs 4)
    granted = checkAccess(makeRequest("R101", 3), reason);
    check("Test 1: Valid access is granted", granted);

    // Test 2: unknown researcher
    granted = checkAccess(makeRequest("R999", 0), reason);
    check("Test 2: Invalid researcher is denied", !granted && reason == "Researcher not found");

    // Test 3: revoked credential (R107 is revoked)
    granted = checkAccess(makeRequest("R107", 0), reason);
    check("Test 3: Revoked credential is denied", !granted && reason == "Credential revoked");

    // Test 4: unauthorized zone (R109 is level 1, LAB-C needs 3)
    granted = checkAccess(makeRequest("R109", 2), reason);
    check("Test 4: Unauthorized zone is denied", !granted && reason == "Access level too low for this zone");

    // Test 5: locked zone (even the level-4 researcher is denied)
    labLocked[0] = true;
    granted = checkAccess(makeRequest("R101", 0), reason);
    labLocked[0] = false;
    check("Test 5: Locked zone is denied", !granted && reason == "Zone is locked down");

    // Test 6: queue is FIFO (local queue, so real data is untouched)
    Queue q;
    q.enqueue(makeRequest("R101", 0));
    q.enqueue(makeRequest("R102", 0));
    q.enqueue(makeRequest("R103", 0));
    AccessRequest a, b, c;
    q.dequeue(a);
    q.dequeue(b);
    q.dequeue(c);
    check("Test 6: Queue is FIFO", a.researcherId == "R101" && b.researcherId == "R102"
                                   && c.researcherId == "R103" && q.isEmpty());

    // Test 7: stack is LIFO
    Stack s;
    AccessEvent e1, e2, e3, p1, p2, p3;
    e1.researcherId = "R101"; e1.zone = 0; e1.granted = true; e1.reason = "";
    e2 = e1; e2.researcherId = "R102";
    e3 = e1; e3.researcherId = "R103";
    s.push(e1);
    s.push(e2);
    s.push(e3);
    s.pop(p1);
    s.pop(p2);
    s.pop(p3);
    check("Test 7: Stack is LIFO", p1.researcherId == "R103" && p2.researcherId == "R102"
                                   && p3.researcherId == "R101" && s.isEmpty());

    // Test 8: hash collision (two different IDs in the same bucket, both still found)
    int first = -1, second = -1;
    for (int i = 0; i < researcherCount && first == -1; i++) {
        for (int j = i + 1; j < researcherCount; j++) {
            if (hashFunction(researchers[i].id) == hashFunction(researchers[j].id)) {
                first = i;
                second = j;
                break;
            }
        }
    }
    bool collisionOk = false;
    if (first != -1) {
        collisionOk = (table.find(researchers[first].id, steps) == first)
                   && (table.find(researchers[second].id, steps) == second);
    }
    check("Test 8: Hash collision handled by linked list", collisionOk);

    // Test 9: linear search (array is still unsorted here)
    int found = linearSearch("R105", steps);
    int missing = linearSearch("R000", steps);
    check("Test 9: Linear search", found != -1 && researchers[found].id == "R105" && missing == -1);

    // Test 10: bubble sort puts IDs in ascending order
    bubbleSortById();
    bool ordered = true;
    for (int i = 0; i < researcherCount - 1; i++) {
        if (researchers[i].id > researchers[i + 1].id) ordered = false;
    }
    check("Test 10: Bubble sort orders IDs", ordered && sortedById);

    // Test 11: binary search on the sorted array
    found = binarySearch("R105", steps);
    missing = binarySearch("R000", steps);
    check("Test 11: Binary search", found != -1 && researchers[found].id == "R105" && missing == -1);

    // Test 12: selection sort puts names in ascending order, hash lookup still works
    selectionSortByName();
    ordered = true;
    for (int i = 0; i < researcherCount - 1; i++) {
        if (researchers[i].name > researchers[i + 1].name) ordered = false;
    }
    int idx = table.find("R105", steps);
    check("Test 12: Selection sort orders names", ordered && idx != -1 && researchers[idx].id == "R105");

    cout << "\nResult: " << testsPassed << " / " << testsTotal << " tests passed.\n";
    loadSampleData();      // leave the system in a clean state
    cout << "System reset to sample data.\n";
}

// ----------------------------------------------------------------------------
//  MAIN MENU
// ----------------------------------------------------------------------------
void printMenu() {
    cout << "\n========================================\n";
    cout << "       LAB ACCESS VERIFICATION\n";
    cout << "========================================\n";
    cout << " 1. Add Researcher\n";
    cout << " 2. View Researchers\n";
    cout << " 3. Search Researcher\n";
    cout << " 4. Create Access Request\n";
    cout << " 5. View Pending Requests\n";
    cout << " 6. Process Requests\n";
    cout << " 7. Emergency Lockdown\n";
    cout << " 8. View Recent Access Events\n";
    cout << " 9. Sort Researchers\n";
    cout << "10. Run Tests\n";
    cout << "11. Revoke / Restore Credential\n";
    cout << "12. View Hash Table\n";
    cout << " 0. Exit\n";
}

int main() {
    loadSampleData();
    cout << "Sample data loaded: " << researcherCount << " researchers, "
         << NUM_ZONES << " lab zones.\n";

    while (true) {
        printMenu();
        int choice = readInt("Enter choice: ", 0, 12);
        switch (choice) {
            case 1:  addResearcher();        break;
            case 2:  viewResearchers();      break;
            case 3:  searchResearcher();     break;
            case 4:  createAccessRequest();  break;
            case 5:  viewPendingRequests();  break;
            case 6:  processRequests();      break;
            case 7:  emergencyLockdown();    break;
            case 8:  viewRecentEvents();     break;
            case 9:  sortResearchers();      break;
            case 10: runTests();             break;
            case 11: toggleCredential();     break;
            case 12: viewHashTable();        break;
            case 0:
                table.clear();
                cout << "Goodbye.\n";
                return 0;
        }
    }
}