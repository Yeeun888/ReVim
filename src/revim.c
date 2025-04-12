//------------------------------ Includes ---------------------------------------
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>

//------------------------------- DEBUG ----------------------------------------
#define DEBUG
#ifdef DEBUG
FILE* debug_file = NULL;
#endif 

//------------------------------ Defines ---------------------------------------
#define REVIM_VER "0.0.1" 
#define REVIM_TAB_SIZE 4

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey { // Internal representation of keys for editor
    CURSOR_LEFT = 500,
    CURSOR_RIGHT,
    CURSOR_UP,
    CURSOR_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
};

//------------------------ FORWARD DECLARATIONS --------------------------------
void disableRawMode();

//-------------------------------- DATA ----------------------------------------
typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

struct editorConfig {
    int cx, cy;                         // Cursor position (0-index)
    int rx;                             // Cursor position after space tab
    int rowoff;                         // Row offset
    int coloff;                         // Column offset
    int terminal_rows;                  // Current terminal height (rows / 1-index)
    int terminal_cols;                  // Current terminal width (columns / 1-index)
    int numrows;                        // Number of all rows in terminal
    erow *row;                          // Array of rows (All text stored in memory)
    char *filename;                     // Current filename
    char statusmsg[80];                 // Status for user to see
    time_t statusmsg_time;              // Time stamp for status message
    struct termios init_termios;        // Initial terminal configuration
};

struct editorConfig gc; //Global config (gc)

//------------------------------ Terminal --------------------------------------
void exitCleanup() {
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void die(const char *s) {
    //Clear screen when exiting
    exitCleanup();

    perror(s);
    exit(1);
}

void disableRawMode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &gc.init_termios) == -1) {
        die("tcgetattr");
    }
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &gc.init_termios) == -1) {
        die("tcgetattr");
    }
    atexit(disableRawMode);
    
    struct termios mode = gc.init_termios;
    mode.c_iflag &= ~(IXON | ICRNL | ISTRIP | INPCK | BRKINT);
    mode.c_oflag &= ~(OPOST);
    mode.c_cflag |= (CS8);
    mode.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode) == -1) {
        die("tcgetattr");
    }

    //Flags notes
    //IXON = Enable XON / XOFF contrls (ctrl-s, ctrl-q)
    //ICRNL = Enable ctrl-m (\r into \n)
    //ISTRIP = Strip eight bit of input bytes
    //INPCK = Parity checking
    //BRKINT = Break in program causes sigint

    //OPOST = Output processing

    //CS8 = All character size eight bytes

    //ECHO = Enable terminal output
    //ICANON = Enable canonical mode
    //ISIG = Enable interrupt signals
    //IEXTEN = Enable next character literal input

    mode.c_cc[VMIN] = 0;
    mode.c_cc[VTIME] = 1;
}

int editorReadKey() {
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1) != 1)) {
        if(nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
        
    if (c == '\x1b') {
        char seq[3]; //Store three characters after escape

        //When read times out and reads nothing else then return esc key
        if (read(STDIN_FILENO, &seq[0], 1) != 1) { return '\x1b'; } // No key read
        if (read(STDIN_FILENO, &seq[1], 1) != 1) { return '\x1b'; } // Only escape key read

        if(seq[0] == '[') {
            //Handle VT100 codes for \x1b[0~ - \x1b[0~
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) { return '\x1b'; } //Improper escape key
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;  
                        case '3': return DEL_KEY;   //Home and End can be handled
                        case '4': return END_KEY;   //many different ways
                        case '5': return PAGE_UP;   //See code below for more escape codes
                        case '6': return PAGE_DOWN; //on PAGE_DOWN and PAGE_UP
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch(seq[1]) {
                    // Arrow escape codes
                    case 'A': return CURSOR_UP;
                    case 'B': return CURSOR_DOWN;
                    case 'C': return CURSOR_RIGHT;
                    case 'D': return CURSOR_LEFT;

                    //Handle \x1b[H, \x1b[F
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }

        } else if (seq[0] == 'O') { //Handle escape key \x1bOF \x1bOH
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int* rows, int* columns) {
    char buf[32];
    unsigned int i = 0;

    // Request cursor response from terminal
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    // Write cursor response from terminal
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) { break; }
        if (buf[i] == 'R') { break; }
        ++i;
    }
    buf[i] = '\0';
    
    //Check for error
    if (buf[0] != '\x1b' || buf[1] != '[') { return -1; }
    
    //String format and input into variable
    if (sscanf(&buf[2], "%d;%d", rows, columns) != 2) { return -1; }
    
    return 0;    

    // Extra code for later
    // Printing cursor response
    // printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);

    // printf("\r\n");
    // char c;
    // while (read(STDOUT_FILENO, &c, 1) == 1) {
    //     if (iscntrl(c)) {
    //         printf("%d\r\n", c);
    //       } else {
    //         printf("%d ('%c')\r\n", c, c);
    //       }
    // }
}

int getWindowSize(int* rows, int* columns) {
    static struct winsize w;

    //IOCTL is not guaranteed on all devices
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
    
        return getCursorPosition(columns, rows);
    }

    *columns = w.ws_col;
    *rows = w.ws_row;

    return 0;
}

//---------------------------- Rows Operations ---------------------------------
int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;

    for (int i = 0; i < cx; ++i) {
        if (row->chars[i] == '\t') {
            rx += (REVIM_TAB_SIZE - 1) - (rx % REVIM_TAB_SIZE);
        }
        ++rx;
    }

    return rx;
}

void editorUpdateRow(erow *row) {
    //Malloc count tabs to malloc required space based on tab size
    int tabs = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') { tabs++; };
    }

    free(row->render);
    row->render = malloc(row->size + 1 + tabs*(REVIM_TAB_SIZE - 1));

    int idx = 0;
    for (int j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % REVIM_TAB_SIZE != 0) {
                //Add spaces until no more tab
                row->render[idx++] = ' ';
            }
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorAppendRow(char *s, size_t len) {
    gc.row = realloc(gc.row, sizeof(erow) * (gc.numrows + 1));

    int at = gc.numrows;
    gc.row[at].size = len;
    gc.row[at].chars = malloc(len + 1);
    memcpy(gc.row[at].chars, s, len);
    gc.row[at].chars[len] = '\0';

    gc.row[at].rsize = 0;
    gc.row[at].render = NULL;
    editorUpdateRow(&gc.row[at]);

    gc.numrows++;
}

//------------------------------- File I/O -------------------------------------
void editorOpen(char *filename) {
    free(gc.filename);
    gc.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if(!fp) {
        die("fopen");
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    
    while((linelen = getline(&line, &linecap, fp)) != -1) {;
        if(linelen != -1) {
            //Remov all the \n and \r from a line before printing it
            while (linelen > 0 && (line[linelen -1] == '\n' ||
                                line[linelen -1] == '\r')) {
                linelen--;
            }
            editorAppendRow(line, linelen);
        }
    }

    free(line);
    fclose(fp);
}

//----------------------------- Append Buffer ----------------------------------
struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    //Allocate new memory for larger buffer
    char *newBuffer = (char *)realloc(ab->b, ab->len + len);

    //Error on memory allocation
    if (newBuffer == NULL) {
        return;
    }

    memcpy(&newBuffer[ab->len], s, len);
    ab->b = newBuffer;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

//-------------------------------- Output --------------------------------------
void editorScroll() {
    gc.rx = 0;
    if (gc.cy < gc.numrows) {
        gc.rx = editorRowCxToRx(&gc.row[gc.cy], gc.cx);
    }

    //Scroll up
    if (gc.cy < gc.rowoff) {
        gc.rowoff = gc.cy;
    }
    //Scroll down
    if (gc.cy >= gc.rowoff + gc.terminal_rows) {
        gc.rowoff = gc.cy - gc.terminal_rows + 1;
    }
    //Scroll left
    if (gc.rx < gc.coloff) {
        gc.coloff = gc.rx;
    }
    //Scroll right
    if (gc.rx >= gc.coloff + gc.terminal_cols) {
        gc.coloff = gc.rx - gc.terminal_cols + 1;
    }
}

void editorDrawRows(struct abuf *ab) {
    for(int i = 0; i < gc.terminal_rows; ++i) {
        int filerow = i + gc.rowoff; // Starting row to render at

        if(filerow >= gc.numrows) { //When filerow exceeds number of rows in file
            if(gc.numrows == 0 && i == gc.terminal_rows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "Revim Editor -- Version %s", REVIM_VER);
                
                //Make it so that the message is shortend whenever the screen is not large enough
                if (welcomelen > gc.terminal_cols) {
                    welcomelen = gc.terminal_cols;
                }
    
                int sidePadding = (gc.terminal_cols - welcomelen) / 2;
                if (sidePadding) {
                    abAppend(ab, "~", 1);       
                    sidePadding--;
                }
                while (sidePadding--) { abAppend(ab, " ", 1); }
                abAppend(ab, welcome, welcomelen);
    
            } else {
                abAppend(ab, "~", 1);       
            }
        } else { // Appends file contents into AB otherwise
            int len = gc.row[filerow].rsize - gc.coloff;
            if (len < 0) { len = 0; }
            if (len > gc.terminal_cols) { len = gc.terminal_cols; }
            abAppend(ab, &gc.row[filerow].render[gc.coloff], len);
        }


        abAppend(ab, "\x1b[K", 3); // Clear line
        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), 
        " %.20s - %d lines", gc.filename ? gc.filename : "[No Name]", gc.numrows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d,%d ", gc.cy + 1, gc.numrows);
    if (len > gc.terminal_cols) {
        len = gc.terminal_cols;
    }
    abAppend(ab, status, len);

    while (len < gc.terminal_cols) {
        if(gc.terminal_cols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(gc.statusmsg);
    if (msglen > gc.terminal_cols) {
        msglen = gc.terminal_cols;
    }
    if (msglen && (time(NULL) - gc.statusmsg_time) < 5) {
        abAppend(ab, gc.statusmsg, msglen);
    }
}

void editorRefreshScreen() {
    editorScroll();

    struct abuf ab = ABUF_INIT;

    // \x1b escape character
    abAppend(&ab, "\x1b[?25l", 6); // Hide cursor
    abAppend(&ab, "\x1b[H", 3); // Move cursor to position 0,0

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (gc.cy - gc.rowoff) + 1, 
                                              (gc.rx - gc.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6); // Unhide cursor

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(gc.statusmsg, sizeof(gc.statusmsg), fmt, ap);
    va_end(ap);
    gc.statusmsg_time = time(NULL);
}

//-------------------------------- Input ---------------------------------------
void editorMoveCursor(int key) {
    erow *row = (gc.cy >= gc.numrows) ? NULL : &gc.row[gc.cy];

    switch (key) {
        //Using vim commands
        case(CURSOR_LEFT):
            if (gc.cx != 0) {
                gc.cx--;
            }
            break;
        case(CURSOR_RIGHT):
            if(row && gc.cx < row->size) {
                gc.cx++;
            }
            fprintf(debug_file, "debug here");
            break;
        case(CURSOR_DOWN):
            if (gc.cy < gc.numrows) {
                gc.cy++;
            }
            break;
        case(CURSOR_UP):
            if (gc.cy != 0) {
                gc.cy--;
            }
            break;

    } 

    //Check if went up or down a row and update row variable
    row = (gc.cy >= gc.numrows) ? NULL : &gc.row[gc.cy];
    int rowlen = row ? row->size : 0;
    if(gc.cx > rowlen) {
        gc.cx = rowlen;
    }
}

void processEditorKeyPress() {
    int c = editorReadKey();

    switch(c) {
        case(CTRL_KEY('q')):
            //Clear screen when exiting
            exitCleanup();
            exit(0);
            break;

        case HOME_KEY:
            gc.cx = 0;
            break;
        case END_KEY:
            if (gc.cy < gc.numrows) {
                gc.cx = gc.row[gc.cy].size;
            }
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                //If statement required to clear whole screen without going too far
                if (c == PAGE_UP) {
                    gc.cy = gc.rowoff;
                } else if (c == PAGE_DOWN) {
                    gc.cy = gc.rowoff + gc.terminal_rows - 1;
                    if (gc.cy > gc.numrows) {
                        gc.cy = gc.numrows;
                    }
                }

                int times = gc.terminal_rows;
                while(times--) {
                    editorMoveCursor(c == PAGE_UP ? CURSOR_UP : CURSOR_DOWN);
                }
            }    
            break;

        case(CURSOR_LEFT):
        case(CURSOR_RIGHT):
        case(CURSOR_UP):
        case(CURSOR_DOWN):
            editorMoveCursor(c);
            break;
    }
}

//-------------------------------- Init ----------------------------------------

void initEditor() {
    // Initial variable initialization
    gc.cx = 0;
    gc.cy = 0;
    gc.rx = 0;
    gc.rowoff = 0;
    gc.coloff = 0;
    gc.numrows = 0;
    gc.row = NULL;
    gc.filename = NULL;
    gc.statusmsg[0] = '\0';
    gc.statusmsg_time = 0;

    editorSetStatusMessage("Ctrl-Q = Quit Editor");

    if(getWindowSize(&gc.terminal_rows, &gc.terminal_cols) == -1) {
        die("getWindowSize");
    }

    gc.terminal_rows -= 2;
}

int main(int argc, char** argv) {

    #ifdef DEBUG
    debug_file = fopen("debug.log", "w");
    if (debug_file == NULL) {
        die("debug_file fopen error");
    }

    fprintf(debug_file, "test");
    #endif

    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    while (1) {
        editorRefreshScreen();
        processEditorKeyPress();
    }
    
    #ifdef DEBUG
    fclose(debug_file);
    #endif

    return 0;
}