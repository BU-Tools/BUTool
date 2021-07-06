#ifndef __PREAMBLE_HH__
#define __PREAMBLE_HH__

enum Preamble : unsigned {
    Disabled = 0x1,     // 
    Enabled = 0x2,      // 
    Text = 0x4,         // (is there text?)
    Timestamp = 0x8     // (is there timestamp?)
};

// allow for bitwise operations on the enum
Preamble constexpr operator | (Preamble lhs, Preamble rhs) {
    return static_cast<Preamble>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

Preamble& operator |= (Preamble &lhs, Preamble rhs) {
    lhs = static_cast<Preamble>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
    return lhs;
}

Preamble constexpr operator & (Preamble lhs, Preamble rhs) {
    return static_cast<Preamble>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
}

Preamble& operator &= (Preamble &lhs, Preamble rhs) {
    lhs = static_cast<Preamble>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
    return lhs;
}

Preamble constexpr operator ^ (Preamble lhs, Preamble rhs) {
    return static_cast<Preamble>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs));
}

Preamble& operator ^= (Preamble &lhs, Preamble rhs) {
    lhs = static_cast<Preamble>(static_cast<unsigned>(lhs) ^ static_cast<unsigned>(rhs));
    return lhs;
}

Preamble constexpr operator ~ (Preamble rhs) {
    return ~static_cast<Preamble>(static_cast<unsigned>(rhs));
}

#endif