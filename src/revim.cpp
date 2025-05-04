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

void printDebug(gap_buffer<char> &gb) {
    mvprintw(0, 0, gb.dump_buffer());
    mvprintw(2,0, gb.dump_gap_end());
    mvprintw(3,0, gb.dump_gap_start());
    mvprintw(1,0, gb.dump_gap_address());
}

// Placeholder for your function
void handleLeft(gap_buffer<char> &gb) {
    gb.left();
    gb.fill_gap('@');
    printDebug(gb);
}

void handleRight(gap_buffer<char> &gb) {
    gb.right();
    gb.fill_gap('@');
    printDebug(gb);
}   

int main(int argc, char** argv) {
    gap_buffer<char> gb;

    std::fstream file("testfile", std::ios::in | std::ios::out);
    gb.init_gap_buffer(file, 8);

    // Initialize ncurses
    initscr();
    cbreak();            // Disable line buffering
    noecho();            // Don't echo pressed keys to screen
    keypad(stdscr, TRUE); // Enable special keys like arrows

    mvprintw(0, 0, "Press L or R (press Q to quit)");

    bool running = true;
    while (running) {
        int ch = getch();
        switch (ch) {
            case 'L':
            case 'l':
                clear();
                handleLeft(gb);
                break;
            case 'R':
            case 'r':
                clear();
                handleRight(gb);
                break;
            case 'Q':
            case 'q':
                running = false;
                break;
            case 'w':
            case 'W':
                clear();
                gb.fill_gap('@');
                printDebug(gb);
                break;
            default:
                mvprintw(4, 0, "Invalid key: %c            ", ch);
                break;
        }
        move(0, 0);
        refresh(); // Refresh the screen with updates
    }

    // Move cursor to the left side of the screen
    move(0, 0);
    refresh();

    // End ncurses mode
    getch(); // Wait for a final key press
    endwin();

    return 0;
}