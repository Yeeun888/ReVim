#include <stdio.h>
#include <fstream>

#define BUF_GAP_SIZE 128
#define LINE_GAP_SIZE 64

class gap_buffer {
    private:
        typedef struct char_buffer {
            char *buffer;
            char *c_cur;
            char *c_end;
            int size;
        } char_buffer;

        // Gap buffer to keep track of inserts and line performance
        typedef struct line_buffer {
            int *line_size;
            int *c_cur;
            int *c_end;
            int size;
        } line_buffer;

        char_buffer cb;
        line_buffer lb;

    public:   
        /**
         * @brief Construct new gap buffer. Initializes all variables in gap buffer
         * accordingly. Must call init_gap_buffer to properly initialize the
         * gap buffer 
         */ 
        gap_buffer();

        /**
         * @brief Initialize gap buffer with a file
         * 
         * @param Initializes the gap buffer (and line buffer). Assumes
         * file does exist
         */
        void init_gap_buffer(std::fstream& file);

        /**
         * @brief Insert a character AFTER pos 
         * into the gap buffer and save changes in gap buffer
         * 
         * @param c 
         * @param line 
         * @param pos 
         */
        void insert_character(char c, int line, int pos);   

        /**
         * @brief Replace character at line in position 
         * 
         * @param c 
         * @param line 
         */
        void replace_character(char c, int line, int pos);

        /**
         * @brief Delete character AT position in a line
         * 
         * @param c 
         * @param pos 
         */
        void delete_character(char c, int line, int pos);

        /**
         * @brief Remove a whole line from buffer
         * 
         */
        void delete_line(char c, int line);

        /**
         * @brief DEBUG FUNCTION - DUMPS CONTENTS OF GAP BUFFER
         * 
         * @param dump_stream 
         */
        void dump_buffer(std::ostream& dump_stream);
};