#include "vigor/global.h"
#include "vigor/text_buffer.h"

#include <fstream>
#include <sstream>
#include <string>

TextBuffer::~TextBuffer() {
    this->stream.flush();
    this->stream.close();
}

void TextBuffer::register_callback(callable_t cb) {
    this->callback = cb;
}

void TextBuffer::load_file(std::string filepath) {
    std::ifstream t;
    t.open(filepath);
    std::stringstream buffer;
    buffer << t.rdbuf();
    t.close();

    std::string s = buffer.str();

    this->stream = std::fstream(filepath + ".swap", std::fstream::in | std::fstream::out);

    //this->stream.open(filepath + ".swap", std::fstream::in | std::fstream::out);
    this->stream.write(s.c_str(), s.length());
    this->stream.seekp(0, this->stream.beg);

    this->line_positions.push_back(0);
}

void TextBuffer::inc_start_line() {
    this->start_line++;
}

void TextBuffer::inc_stop_line() {
    this->stop_line++;
}

void TextBuffer::dec_start_line() {
    this->start_line--;
}

void TextBuffer::dec_stop_line() {
    this->stop_line--;
}

void TextBuffer::append_text(std::string text) {
    long pos = this->stream.tellp();
    this->stream.seekp(0, this->stream.end);
    this->stream.write(text.c_str(), text.size());
    this->stream.seekp(pos);
}

void TextBuffer::insert_text(std::string text) {
    this->stream.write(text.c_str(), text.size());
}

std::optional<std::string> TextBuffer::read_prev_line() {
    return {};
}

std::optional<std::string> TextBuffer::read_next_line() {
    if (!this->stream.is_open()) {
        PLOGE << "Stream not open";
        return {};
    }

    std::string line;

    if (getline(this->stream, line)) {
        this->start_line++;

        if (this->line_positions.size() <= this->start_line) {
            this->line_positions.push_back(this->stream.tellg());
        } else {
            this->line_positions[this->start_line] = this->stream.tellg();
        }

        return line;
    }

    return {};
}

void TextBuffer::seek_line(unsigned int line_num) {
    PLOGD << "Seeking line " << line_num;
    this->stream.clear();

    if (this->line_positions.size() <= line_num) {
        this->start_line = this->line_positions.size() - 1;
        this->stream.seekg(this->line_positions[this->start_line]);
        while (this->read_next_line().has_value() && this->start_line < line_num);
    } else {
        this->start_line = line_num;
        this->stream.seekg(this->line_positions[this->start_line]);
    }
}

void TextBuffer::set_max_buffer_height(unsigned int height) {
    this->max_buffer_height = height;
}
