#include <stdio.h>
#include <ncurses.h>
#include <cstdio>
#include <string>
#include <vector>

#include <iostream>
#include <sstream>

#include "gap_buffer.hpp"

// ------------------------- EDITOR WIDE VARIABLES -----------------------------

// ------------------------------ GAP BUFFER -----------------------------------

// ------------------------------ TERMINAL  ------------------------------------

/**
 * @brief Functions that need to run when exiting the program 
 */
void exit_cleanup() {
    endwin();
}

/**
 * @brief Error exit and displays error
 * 
 * @param {string} message - Messsage to display error ouput
 */
void error_exit(const char* message) {
    exit_cleanup();

    perror(message);
    exit(1);
}

// ------------------------------- HELPERS -------------------------------------
/**
 * @brief Check if a file exists
 * 
 * @param name std::string of the file name
 * @return true if yes
 * @return false if no
 */
inline bool file_exists (const std::string& name) {
    if (FILE* fptr = fopen(name.c_str(), "r")) {
        fclose(fptr);
        return true;
    } else {
        return false;
    }
}

// ----------------------------- INIT and MAIN ---------------------------------

void init_global_config() {

}

int main(int argc, char** argv) {
    gap_buffer<char> gb;

    std::fstream file("func.c", std::ios::in | std::ios::out);
    gb.init_gap_buffer(file, 128);
    gb.insert('c');
    gb.insert('c');
    gb.insert('c');
    gb.insert('c');
    gb.dump_buffer(std::cerr);

    // initscr(); 

    // getchar();

    // endwin();
}