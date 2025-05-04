#include <ostream>
#include <fstream>
#include <stdlib.h>

template <typename T>
class gap_buffer {
    private: 
        struct buf {
            T* _buffer;                 // Buffer containing text
            T* _gap_start;              // Gap start address
            T* _gap_end;                // Gap end address i.e next part buffer start
                                        // address

            size_t _buf_size;           // Buffer size 
        };

        static size_t gap_size;         // Gap starting size (double on resize)
        struct buf buf;                 // the actual buf

    public:
        /**
         * @brief Initializes buf with non-garbage values
         * 
         */
        gap_buffer();

        /**
         * @brief Accepts an fstream object of type T an initializes buffer
         * with 
         * 
         * @param stream 
         */
        void init_gap_buffer(std::fstream& stream, size_t start_buff_size);

        /**
         * @brief If buffer is empty adds more space in buffer and doubles 
         * buffer size. Else, adds element. 
         * 
         * @param element 
         * @param pos Position in the text without gap buffer
         * @return 1 on success, 0 on failure
         */
        int insert(T element);

        /**
         * @brief Shifts buffer left
         * 
         */
        void left();

        /**
         * @brief Shifts buffer right
         * 
         */
        void right();

        /**
         * @brief Move to a certain position in the gap buffer
         * 
         * @param pos 
         */
        void move_to(size_t pos);

        /**
         * @brief Doubles buffer capacity compared to initialized buffer. 
         * by default keeps doubling without limit. Behavior can be changed
         * 
         * @return 1 on success, 0 on filaure
         */ 
        int grow();

        // ------------------------ DEBUG FUNCTIONS ----------------------------
        T* dump_buffer();
        T* dump_gap_start();
        T* dump_gap_end();
        char* dump_gap_address();
        void fill_gap(T t);
};

// --------------------- TEMPLATE IMPLEMENTATION -------------------------------

template <typename T>
size_t gap_buffer<T>::gap_size; // Definition of the static member

template <typename T>
gap_buffer<T>::gap_buffer() {
    buf._buffer = nullptr;
    buf._gap_start = nullptr;
    buf._gap_end = nullptr;
    buf._buf_size = 0;
    gap_size = 0;
}

template <typename T>
void gap_buffer<T>::init_gap_buffer(std::fstream& stream, size_t start_gap_size) {
    //Save stream pos
    std::streampos init_pos = stream.tellg();

    gap_size = start_gap_size;

    T t;
    int num_t = 0;
    while(stream.get(t)) {
        num_t++;
    }

    //Clear all flags and bring back
    stream.clear();
    stream.seekg(0, std::ios::beg);

    buf._buffer = (T *)calloc(sizeof(T) * (num_t + gap_size), sizeof(T));

    //Start gap at the end
    buf._gap_start = &buf._buffer[num_t];
    buf._gap_end = &buf._buffer[num_t + gap_size];
    buf._buf_size = num_t;

    // //Initialize null terminator
    // *buf._gap_end = '\0';

    for (int i = 0; i < num_t; ++i) {
        stream.get(t);
        buf._buffer[i] = t;
    }

    //Clear all flags and return stream to the beginning
    stream.clear();
    stream.seekg(init_pos, std::ios::beg);

    return;
}

template <typename T>
int gap_buffer<T>::insert(T t) {
    if (buf._gap_start == buf._gap_end) {
        if(!grow()) {
            return 0;
        };
    }

    *buf._gap_start = t;
    ++buf._gap_start;
    ++buf._buf_size;

    return 1;
}

template <typename T>
int gap_buffer<T>::grow() {
    size_t pb_start_idx = buf._gap_start - buf._buffer;
    size_t pb_end_idx = buf._gap_end - buf._buffer;
    size_t after_buff_move = buf._buf_size - (buf._gap_start - buf._buffer);

    //Check for realloc failure so as to not loose the pointer
    T* newPtr = (T *)realloc(buf._buffer, sizeof(T) * (buf._buf_size + 2*gap_size));
    if (newPtr == nullptr)  {
        return 0;
    } else {
        buf._buffer = newPtr;
    }

    //Get new gap end location
    T* new_gap_end = &buf._buffer[pb_start_idx + 2*gap_size];
    memmove(new_gap_end, &buf._buffer[pb_end_idx], after_buff_move);

    //Move gap and ends to new memory locations
    buf._gap_start = &buf._buffer[pb_start_idx];
    buf._gap_end = new_gap_end; 
    
    //gap_size and buf_size updated to reflect new changes
    gap_size *= 2;

    // Alternative to limit gap growth
    // gap_size = (gap_size * 2 > 10000) 10000 : gap_size * 2;

    return 1;
}

template <typename T>
void gap_buffer<T>::left() {
    if (buf._gap_start == buf._buffer) return;

    *(buf._gap_end - 1) = *(buf._gap_start - 1);
    --buf._gap_start;
    --buf._gap_end;
}

template <typename T>
void gap_buffer<T>::right() {
    if (buf._gap_start == &buf._buffer[buf._buf_size]) return;

    *buf._gap_start = *(buf._gap_end + 1);
    ++buf._gap_end;
    ++buf._gap_start;
}

template <typename T>
void gap_buffer<T>::move_to(size_t pos) {
    size_t curr_pos = buf._gap_start - buf._buffer;
    if (pos == curr_pos) return;

    if (pos > curr_pos) {
        for (; pos != curr_pos; ++curr_pos) {
            right();
        }
    } else { //pos < curr_pos
        for (; pos != curr_pos; ++pos) {
            left();
        }
    }

    return;
}

//--------------------------------- DEBUG  -------------------------------------
template <typename T>
T* gap_buffer<T>::dump_buffer() {
    return buf._buffer;
}

template <typename T>
T* gap_buffer<T>::dump_gap_end() {
    return buf._gap_end;
}

template <typename T>
T* gap_buffer<T>::dump_gap_start() {
    return buf._gap_start;
}

template <typename T>
char * gap_buffer<T>::dump_gap_address() {
    char addr1[32];
    char addr2[32];

    // Convert the addresses to strings (platform-dependent format)
    std::snprintf(addr1, sizeof(addr1), "%p", (void*)buf._gap_start);
    std::snprintf(addr2, sizeof(addr2), "%p", (void*)buf._gap_end);

    // Final result size = strlen(addr1) + 1 (space) + strlen(addr2) + 1 (null terminator)
    size_t totalLength = strlen(addr1) + 1 + strlen(addr2) + 1 + 48;
    char* result = new char[totalLength];

    char calc[8];
    std::snprintf(calc, sizeof(calc), "%ld", buf._gap_end - buf._gap_start);

    // Concatenate addr1 + space + addr2
    std::strcpy(result, addr1);
    std::strcat(result, " ");
    std::strcat(result, addr2);
    std::strcat(result, " ");
    std::strcat(result, calc);

    return result;
}

template <typename T>
void gap_buffer<T>::fill_gap(T t) {
    for (T* ptr = buf._gap_start; ptr < buf._gap_end; ++ptr) {
        *ptr = t;
    }
}