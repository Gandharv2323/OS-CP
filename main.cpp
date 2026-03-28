/*
 * ============================================================
 *  Multiprogramming Operating System (MOS) — Phase 1
 *  Single-file implementation following the full specification.
 *
 *  Compile:  g++ -std=c++17 -Wall -o mos main.cpp
 *  Run:      ./mos   (reads input.txt, writes output.txt)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>   // memcpy
#include <stdexcept>

using namespace std;

// ─────────────────────────────────────────────────────────────
//  GLOBAL VARIABLES  (spec: all global, never passed as args)
// ─────────────────────────────────────────────────────────────
char M[100][4];   // Main memory: 100 words × 4 bytes
char IR[4];       // Instruction Register
char R[4];        // General-Purpose Register
int  IC;          // Instruction Counter (0–99)
char C;           // Condition toggle: 'T' or 'F'
int  SI;          // Service Interrupt: 1=READ, 2=WRITE, 3=TERMINATE

ifstream inputFile;   // input.txt
ofstream outputFile;  // output.txt

// ─────────────────────────────────────────────────────────────
//  Forward declarations (needed because MOS ↔ EXECUTEUSERPROGRAM
//  call each other)
// ─────────────────────────────────────────────────────────────
void INIT();
void STARTEXECUTION();
void EXECUTEUSERPROGRAM();
void MOS();

// ─────────────────────────────────────────────────────────────
//  HELPER 1 — initMemory
//  Resets the entire simulated machine to a blank state:
//  all memory cells = ' ', registers cleared, counters zeroed.
// ─────────────────────────────────────────────────────────────
void initMemory() {
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 4; ++j)
            M[i][j] = ' ';

    IR[0] = IR[1] = IR[2] = IR[3] = ' ';
    R[0]  = R[1]  = R[2]  = R[3]  = ' ';
    IC = 0;
    C  = 'F';
    SI = 0;
}

// ─────────────────────────────────────────────────────────────
//  HELPER 2 — storeLineToMemory
//  Stores one card image (exactly 40 chars) into 10 consecutive
//  memory words starting at startWord.
//  The input line is padded / trimmed to exactly 40 characters.
// ─────────────────────────────────────────────────────────────
void storeLineToMemory(const string& line, int startWord) {
    // Build a 40-character image (pad with spaces on the right)
    string card = line;
    if (card.size() < 40)
        card.resize(40, ' ');
    // Do not keep more than 40 chars
    // (trim is implicit because we index only 0..39)

    for (int i = 0; i < 10; ++i) {
        M[startWord + i][0] = card[i * 4 + 0];
        M[startWord + i][1] = card[i * 4 + 1];
        M[startWord + i][2] = card[i * 4 + 2];
        M[startWord + i][3] = card[i * 4 + 3];
    }
}

// ─────────────────────────────────────────────────────────────
//  HELPER 3 — readWordAsString
//  Returns the 4 bytes at M[address] as a std::string.
// ─────────────────────────────────────────────────────────────
string readWordAsString(int address) {
    return string(M[address], 4);
}

// ─────────────────────────────────────────────────────────────
//  HELPER 4 — getOperandAddress
//  Extracts the 2-character numeric operand from IR[2..3] and
//  converts it to an integer address (0–99).
// ─────────────────────────────────────────────────────────────
int getOperandAddress() {
    string addrStr = { IR[2], IR[3] };
    try {
        return stoi(addrStr);
    } catch (...) {
        return 0;   // safe fallback
    }
}

// ─────────────────────────────────────────────────────────────
//  HELPER 5 — compareArrays
//  Returns true iff the first `len` elements of arrays a and b
//  are identical (byte-by-byte comparison).
// ─────────────────────────────────────────────────────────────
bool compareArrays(char a[], char b[], int len) {
    for (int i = 0; i < len; ++i)
        if (a[i] != b[i]) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────
//  MODULE 2 — INIT
//  Called at the start of every $AMJ card.
//  Wipes all memory, registers, and control flags clean so the
//  next job starts in a pristine environment.
// ─────────────────────────────────────────────────────────────
void INIT() {
    initMemory();
    cout << "[INIT] Machine state reset for new job.\n";
}

// ─────────────────────────────────────────────────────────────
//  MODULE 5 — MOS  (Master / Kernel mode interrupt handler)
//  Dispatches on the current value of SI:
//    SI=1 → READ  : load one data card into memory
//    SI=2 → WRITE : print 10 words from memory to output file
//    SI=3 → TERMINATE : emit two blank lines, job is done
// ─────────────────────────────────────────────────────────────
void MOS() {
    cout << "[MOS] Interrupt fired — SI=" << SI << "\n";

    switch (SI) {

        // ── READ ──────────────────────────────────────────────
        case 1: {
            int addr = getOperandAddress();
            string dataLine;

            if (!getline(inputFile, dataLine)) {
                outputFile << "ABORT: Out of data (EOF reached)\n";
                cout << "[MOS] READ: EOF reached — job aborted.\n";
                return;
            }

            // Trim Windows \r
            if (!dataLine.empty() && dataLine.back() == '\r')
                dataLine.pop_back();

            // Guard: stop if we hit a control card
            if (dataLine.substr(0, 4) == "$END") {
                outputFile << "ABORT: Out of data ($END found)\n";
                cout << "[MOS] READ: $END found — job aborted.\n";
                return;
            }

            // Pad to 40 characters and store
            storeLineToMemory(dataLine, addr);
            cout << "[MOS] READ: data loaded at word " << addr << "\n";
            return;
        }

        // ── WRITE ─────────────────────────────────────────────
        case 2: {
            int addr = getOperandAddress();
            string out = "";
            for (int i = 0; i < 10; ++i) {
                out += M[addr + i][0];
                out += M[addr + i][1];
                out += M[addr + i][2];
                out += M[addr + i][3];
            }
            outputFile << out << "\n";
            cout << "[MOS] WRITE: 40 chars written from word " << addr << "\n";
            return;
        }

        // ── TERMINATE ─────────────────────────────────────────
        case 3: {
            outputFile << "\n\n";   // two blank lines between jobs
            cout << "[MOS] TERMINATE: job ended, separator written.\n";
            return;
        }

        default:
            cout << "[MOS] WARNING: unknown SI=" << SI << "\n";
            return;
    }
}

// ─────────────────────────────────────────────────────────────
//  MODULE 4 — EXECUTEUSERPROGRAM  (Slave / User mode)
//  Classic fetch–decode–execute loop.
//  Supported opcodes:
//    LR   Load Register from memory
//    SR   Store Register to memory
//    CR   Compare Register with memory (sets C flag)
//    BT   Branch if C == 'T'
//    GD   Get Data  → SI=1, calls MOS()
//    PD   Put Data  → SI=2, calls MOS()
//    H    Halt      → SI=3, calls MOS(), then returns
// ─────────────────────────────────────────────────────────────
void EXECUTEUSERPROGRAM() {
    cout << "[EXEC] Starting user program execution at IC=" << IC << "\n";

    while (true) {
        // ── STEP 1: Fetch ──────────────────────────────────────
        memcpy(IR, M[IC], 4);
        cout << "[FETCH] IC=" << IC
             << "  IR=[" << IR[0] << IR[1] << IR[2] << IR[3] << "]\n";

        // ── STEP 2: Increment IC ───────────────────────────────
        IC++;

        // ── STEP 3: Decode opcode ─────────────────────────────
        string opcode = { IR[0], IR[1] };

        // ── STEP 4: Get operand address ───────────────────────
        int operand = getOperandAddress();

        // ── STEP 5: Execute ───────────────────────────────────

        if (opcode == "LR") {
            // Load Register: R ← M[operand]
            memcpy(R, M[operand], 4);
            cout << "[LR]  R ← M[" << operand << "]\n";
        }
        else if (opcode == "SR") {
            // Store Register: M[operand] ← R
            memcpy(M[operand], R, 4);
            cout << "[SR]  M[" << operand << "] ← R\n";
        }
        else if (opcode == "CR") {
            // Compare Register with memory word
            C = compareArrays(R, M[operand], 4) ? 'T' : 'F';
            cout << "[CR]  Compare R with M[" << operand << "] → C='" << C << "'\n";
        }
        else if (opcode == "BT") {
            // Branch if True
            if (C == 'T') {
                IC = operand;
                cout << "[BT]  C='T' → Branch to IC=" << IC << "\n";
            } else {
                cout << "[BT]  C='F' → No branch\n";
            }
        }
        else if (opcode == "GD") {
            // Get Data (READ)
            SI = 1;
            MOS();
        }
        else if (opcode == "PD") {
            // Put Data (WRITE)
            SI = 2;
            MOS();
        }
        else if (IR[0] == 'H') {
            // Halt / Terminate
            // IR[1] might be a space, so we only check IR[0]
            SI = 3;
            MOS();
            cout << "[H]   Program halted.\n";
            return;   // exit EXECUTEUSERPROGRAM
        }
        else {
            cout << "[EXEC] WARNING: Unknown opcode '" << opcode << "' — skipping.\n";
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  MODULE 3 — STARTEXECUTION
//  Entry point right after $DTA is encountered.
//  Resets IC to 0 and hands control to the user program.
// ─────────────────────────────────────────────────────────────
void STARTEXECUTION() {
    IC = 0;
    cout << "[START] Execution begins at IC=0\n";
    EXECUTEUSERPROGRAM();
}

// ─────────────────────────────────────────────────────────────
//  MODULE 1 — LOAD
//  Top-level loader / job sequencer.
//  Reads the input file card by card, dispatching on control
//  cards ($AMJ, $DTA, $END) or storing program cards into memory.
// ─────────────────────────────────────────────────────────────
void LOAD() {
    int    m = 0;      // memory pointer (resets per job)
    string buffer;

    while (getline(inputFile, buffer)) {
        // Trim Windows \r
        if (!buffer.empty() && buffer.back() == '\r')
            buffer.pop_back();

        if (buffer.substr(0, 4) == "$AMJ") {
            // ── New job ─────────────────────────────────────
            INIT();
            m = 0;
            cout << "[LOAD] Job started: " << buffer << "\n";
            continue;
        }

        if (buffer.substr(0, 4) == "$DTA") {
            // ── Data section: run the loaded program ─────────
            cout << "[LOAD] $DTA encountered — starting execution\n";
            STARTEXECUTION();
            continue;
        }

        if (buffer.substr(0, 4) == "$END") {
            // ── End of job (output already written by TERMINATE)
            cout << "[LOAD] $END encountered — job complete\n";
            continue;
        }

        // ── Program card ─────────────────────────────────────
        if (m >= 100) {
            outputFile << "ABORT: Memory overflow\n";
            cout << "[LOAD] ABORT: Memory overflow at m=" << m << "\n";
            continue;
        }
        storeLineToMemory(buffer, m);
        cout << "[LOAD] Program card stored at word " << m << "\n";
        m += 10;
    }

    cout << "[LOAD] All jobs complete.\n";
}

// ─────────────────────────────────────────────────────────────
//  MAIN FUNCTION
//  Opens files, kicks off LOAD, and cleans up.
// ─────────────────────────────────────────────────────────────
int main() {
    inputFile.open("input.txt");
    outputFile.open("output.txt");

    if (!inputFile.is_open()) {
        cerr << "ERROR: Cannot open input.txt\n";
        return 1;
    }
    if (!outputFile.is_open()) {
        cerr << "ERROR: Cannot open output.txt\n";
        return 1;
    }

    cout << "=== MOS Phase 1 Simulator ===\n";

    LOAD();

    inputFile.close();
    outputFile.close();

    cout << "Done. Check output.txt\n";
    return 0;
}
