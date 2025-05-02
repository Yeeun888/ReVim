#include <fstream>

#include "gap_buffer.hpp"

// ----------------------------- PROTOTYPES ------------------------------------

//---------------------------- IMPLEMENTATION ----------------------------------
gap_buffer::gap_buffer() {
    // Initial constraint before any initialization
    cb.buffer = nullptr;
    cb.c_cur = nullptr;
    cb.c_end = nullptr;
    cb.size = 0;

    lb.line_size = nullptr;
    lb.c_cur = nullptr;
    lb.c_end = nullptr;
    lb.size = 0;
};
 
void gap_buffer::init_gap_buffer(std::fstream& file) {
    //Add everything from that point onwards
    char c;
    int num_lines = 0;
    int num_chars = 0;
    while (file.get(c)) { //Go through file to calculate characters and lines
        num_chars++;
        if (c == '\n') {
            num_lines++;
        }
    }

    //Advance stream pointer back to the beginning
    file.clear(); 
    file.seekg(0, std::ios::beg);

    cb.buffer = (char *)calloc(sizeof(char) * (num_chars + BUF_GAP_SIZE), sizeof(char));
    lb.line_size = (int *)calloc(sizeof(int) * (num_lines + LINE_GAP_SIZE), sizeof(int));

    //Start the gap at the end
    cb.c_cur = &cb.buffer[num_chars];
    cb.c_end = &cb.buffer[BUF_GAP_SIZE + num_chars];
    cb.size = BUF_GAP_SIZE + num_chars;

    //Start the gap at the beginning
    lb.c_cur = &lb.line_size[num_lines];
    lb.c_end = &lb.line_size[LINE_GAP_SIZE + num_lines];
    lb.size = LINE_GAP_SIZE + num_lines;

    //Fill buffer with characters after gap
    int line_size = 0;
    int line_index = 0;
    for (int i = 0; i < num_chars; ++i) {
        file.get(c);
        cb.buffer[i] = c;
        ++line_size;

        if (c == '\n') {
            lb.line_size[line_index] = line_size;
            ++line_index;
        }
    }
};

//--------------------------------- DEBUG  -------------------------------------
void gap_buffer::dump_buffer(std::ostream& dump_stream) {
    dump_stream << cb.buffer << '\n';
    dump_stream << cb.size << ' ' << lb.size << '\n';
}

//-------------------------------- HELPERS  ------------------------------------
std::streamsize get_file_size(std::fstream& file) {
    std::streampos curPos = file.tellg();

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
   
    file.seekg(curPos, std::ios::beg);

    return size;
}