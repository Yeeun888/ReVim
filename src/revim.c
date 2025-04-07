//------------------------------ Includes ---------------------------------------
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

//------------------------------ Defines ---------------------------------------
#define REVIM_VER "0.0.1" 
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
struct editorConfig {
    int cx, cy;
    int rows;
    int columns;
    struct termios init_termios;
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
void editorDrawRows(struct abuf *ab) {
    for(int i = 0; i < gc.rows; ++i) {
        if(i == gc.rows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome), "Revim Editor -- Version %s", REVIM_VER);
            
            //Make it so that the message is shortend whenever the screen is not large enough
            if (welcomelen > gc.columns) {
                welcomelen = gc.columns;
            }

            int sidePadding = (gc.columns - welcomelen) / 2;
            if (sidePadding) {
                abAppend(ab, "~", 1);       
                sidePadding--;
            }
            while (sidePadding--) { abAppend(ab, " ", 1); }
            abAppend(ab, welcome, welcomelen);

        } else {
            abAppend(ab, "~", 1);       
        }

        abAppend(ab, "\x1b[K", 3); // Clear line
        if(i < gc.rows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    struct abuf ab = ABUF_INIT;

    // \x1b escape character
    abAppend(&ab, "\x1b[?25l", 6); // Hide cursor
    abAppend(&ab, "\x1b[H", 3); // Move cursor to position 0,0

    editorDrawRows(&ab);
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", gc.cy + 1, gc.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6); // Unhide cursor

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

//-------------------------------- Input ---------------------------------------
void editorMoveCursor(int key) {
   switch (key) {
        //Using vim commands
        case(CURSOR_LEFT):
            if (gc.cx != 0) {
                gc.cx--;
            }
            break;
        case(CURSOR_RIGHT):
            if (gc.cx != gc.columns - 1) {
                gc.cx++;
            }
            break;
        case(CURSOR_DOWN):
            if (gc.cy != gc.rows - 1) {
                gc.cy++;
            }
            break;
        case(CURSOR_UP):
            if (gc.cy != 0) {
                gc.cy--;
            }
            break;
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
            gc.cx = gc.columns - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = gc.rows;
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
    gc.cx = 0;
    gc.cy = 0;

    if(getWindowSize(&gc.rows, &gc.columns) == -1) {
        die("getWindowSize");
    }
}

int main() {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        processEditorKeyPress();
    }

    return 0;
}