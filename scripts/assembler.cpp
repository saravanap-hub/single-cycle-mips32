// =============================================================
// assembler.cpp -- A tiny two-pass MIPS32 assembler (C++ version)
// =============================================================
// Converts real MIPS assembly text (add, addi, lw, sw, beq, j ...)
// into 32-bit hex machine code -- the same format $readmemh loads
// into imem.v. Two passes:
//   Pass 1: scan every line, record the byte-address of every
//           label (so forward branches/jumps can be resolved).
//   Pass 2: actually encode each instruction into its 32-bit word.
//
// Build:   g++ -std=c++17 -O2 -o assembler assembler.cpp
// Run:     ./assembler test_program.asm memfile.dat
// =============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <stdexcept>

using namespace std;

// ---- Opcode / funct tables (same as the ISA table in the README) ----
static const map<string, uint32_t> RTYPE_FUNCT = {
    {"add", 0x20}, {"sub", 0x22}, {"and", 0x24}, {"or", 0x25}, {"slt", 0x2a}
};
static const map<string, uint32_t> ITYPE_OPCODE = {
    {"addi", 0x08}, {"lw", 0x23}, {"sw", 0x2b}, {"beq", 0x04}
};
static const map<string, uint32_t> JTYPE_OPCODE = {
    {"j", 0x02}
};

// ---- Small helpers --------------------------------------------------

// Strip a trailing "#..." comment and surrounding whitespace.
string stripComment(const string &raw) {
    string line = raw.substr(0, raw.find('#'));
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end   = line.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return line.substr(start, end - start + 1);
}

// Split a line into whitespace/comma separated tokens, e.g.
// "add $5, $5, $4" -> ["add", "$5", "$5", "$4"]
vector<string> tokenize(const string &line) {
    vector<string> tokens;
    string cur;
    for (char c : line) {
        if (c == ' ' || c == '\t' || c == ',') {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

// "$7" -> 7  (register numbers 0-31 only, matches our regfile width)
uint32_t parseReg(const string &tok) {
    if (tok.empty() || tok[0] != '$')
        throw runtime_error("Bad register '" + tok + "', expected e.g. $2");
    int n = stoi(tok.substr(1));
    if (n < 0 || n > 31)
        throw runtime_error("Register out of range: " + tok);
    return static_cast<uint32_t>(n);
}

// Parse a signed decimal/hex immediate and mask it to 'bits' width
// (two's complement wraps automatically via unsigned masking).
uint32_t parseImm(const string &tok, int bits = 16) {
    int32_t val = static_cast<int32_t>(stol(tok, nullptr, 0)); // base 0 -> handles 0x too
    uint32_t mask = (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    return static_cast<uint32_t>(val) & mask;
}

// "68($3)" -> offset=68, baseReg=3
void parseMemOperand(const string &tok, int32_t &offset, uint32_t &baseReg) {
    size_t open  = tok.find('(');
    size_t close = tok.find(')');
    if (open == string::npos || close == string::npos)
        throw runtime_error("Bad memory operand '" + tok + "', expected e.g. 68($3)");
    string offStr = tok.substr(0, open);
    string regStr = tok.substr(open + 1, close - open - 1);
    offset  = static_cast<int32_t>(stol(offStr, nullptr, 0));
    baseReg = parseReg(regStr);
}

// ---- The assembler proper --------------------------------------------
struct RawLine { uint32_t addr; string text; };

vector<uint32_t> assemble(const vector<string> &rawLines) {
    // ---------- Pass 1: compute label addresses ----------
    map<string, uint32_t> labels;
    vector<RawLine> lines;
    uint32_t addr = 0;

    for (const auto &raw : rawLines) {
        string line = stripComment(raw);
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon != string::npos) {
            string label = line.substr(0, colon);
            labels[label] = addr;
            string rest = line.substr(colon + 1);
            size_t s = rest.find_first_not_of(" \t");
            if (s == string::npos) continue;              // label-only line
            rest = rest.substr(s);
            lines.push_back({addr, rest});
        } else {
            lines.push_back({addr, line});
        }
        addr += 4;
    }

    // ---------- Pass 2: encode instructions ----------
    vector<uint32_t> words;
    for (const auto &l : lines) {
        vector<string> tok = tokenize(l.text);
        string mnem = tok[0];
        for (auto &c : mnem) c = tolower(c);
        uint32_t word = 0;

        if (RTYPE_FUNCT.count(mnem)) {
            // R-type: mnemonic rd, rs, rt
            uint32_t rd = parseReg(tok[1]);
            uint32_t rs = parseReg(tok[2]);
            uint32_t rt = parseReg(tok[3]);
            word = (0u << 26) | (rs << 21) | (rt << 16) | (rd << 11) | RTYPE_FUNCT.at(mnem);

        } else if (mnem == "addi") {
            uint32_t rt  = parseReg(tok[1]);
            uint32_t rs  = parseReg(tok[2]);
            uint32_t imm = parseImm(tok[3]);
            word = (ITYPE_OPCODE.at(mnem) << 26) | (rs << 21) | (rt << 16) | imm;

        } else if (mnem == "lw" || mnem == "sw") {
            uint32_t rt = parseReg(tok[1]);
            int32_t offset; uint32_t rs;
            parseMemOperand(tok[2], offset, rs);
            uint32_t imm = static_cast<uint32_t>(offset) & 0xFFFF;
            word = (ITYPE_OPCODE.at(mnem) << 26) | (rs << 21) | (rt << 16) | imm;

        } else if (mnem == "beq") {
            uint32_t rs = parseReg(tok[1]);
            uint32_t rt = parseReg(tok[2]);
            const string &target = tok[3];
            int32_t offsetWords;
            if (labels.count(target))
                offsetWords = (static_cast<int32_t>(labels.at(target)) - static_cast<int32_t>(l.addr + 4)) / 4;
            else
                offsetWords = static_cast<int32_t>(stol(target, nullptr, 0));
            word = (ITYPE_OPCODE.at(mnem) << 26) | (rs << 21) | (rt << 16) | (static_cast<uint32_t>(offsetWords) & 0xFFFF);

        } else if (JTYPE_OPCODE.count(mnem)) {
            const string &target = tok[1];
            uint32_t byteAddr = labels.count(target) ? labels.at(target)
                                                       : static_cast<uint32_t>(stol(target, nullptr, 0));
            uint32_t wordAddr = (byteAddr >> 2) & 0x3FFFFFFu;
            word = (JTYPE_OPCODE.at(mnem) << 26) | wordAddr;

        } else {
            throw runtime_error("Unknown instruction '" + mnem + "'");
        }

        words.push_back(word);
    }
    return words;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " program.asm memfile.dat\n";
        return 1;
    }

    ifstream in(argv[1]);
    if (!in) { cerr << "Cannot open input file: " << argv[1] << "\n"; return 1; }

    vector<string> rawLines;
    string line;
    while (getline(in, line)) rawLines.push_back(line);

    vector<uint32_t> words;
    try {
        words = assemble(rawLines);
    } catch (const exception &e) {
        cerr << "Assembly error: " << e.what() << "\n";
        return 1;
    }

    ofstream out(argv[2]);
    if (!out) { cerr << "Cannot open output file: " << argv[2] << "\n"; return 1; }

    for (uint32_t w : words) {
        out << hex << setfill('0') << setw(8) << w << "\n";
    }

    cout << "Assembled " << words.size() << " instructions -> " << argv[2] << "\n";
    return 0;
}
