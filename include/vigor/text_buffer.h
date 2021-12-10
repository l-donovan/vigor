#pragma once

#include <functional>
#include <fstream>
#include <string>
#include <optional>
#include <vector>

class TextBuffer {
private:
    using callable_t = std::function<bool(std::filebuf::int_type)>;
    callable_t callback = nullptr;
    std::fstream stream;

    unsigned int start_line = 0;
    unsigned int stop_line = 0;
    unsigned int max_buffer_height = 24;

    std::vector<unsigned int> line_positions;
public:
    TextBuffer() {}
    ~TextBuffer();
    TextBuffer(callable_t cb) : callback(cb) {}

    void register_callback(callable_t cb);

    void load_file(std::string filepath);

    void seek_line(unsigned int line);
    void set_max_buffer_height(unsigned int height);
    void inc_start_line();
    void inc_stop_line();
    void dec_start_line();
    void dec_stop_line();

    void append_text(std::string text);
    void insert_text(std::string text);

    std::optional<std::string> read_prev_line();
    std::optional<std::string> read_next_line();
};
