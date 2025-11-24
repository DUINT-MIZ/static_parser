#include <cstdlib>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include "ArgParser/iterators.hpp"

static constexpr size_t buff_size = 1024;

struct termios old_settings, new_settings;

void stdin_deactivate_raw() { 
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings); 
    std::cout << "termios resetted" << std::endl;
}

void stdin_activate_raw()
{
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ECHO | ICANON); // Deactivate Echo and Canon mode then apply the mask
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    std::atexit(stdin_deactivate_raw);
}

template <typename T>
void map_iter_array(iterator_array<T>& arr)
{
    iterator_viewer<T> viewer = arr.get_viewer();
    std::cout << "[ ";
    while(!viewer.end_reached())
    {
        std::cout << "\'" << viewer.value() << "\', ";
        ++viewer;
    }
    std::cout << "\b\b ]" << std::endl;
}

int main()
{
    char buffer[buff_size];
    char c;
    char backspace[3] = {'\b', ' ', '\b'};
    bool keep_running = true;
    std::memset((void*)buffer, 0, buff_size);

    iterator_array<char> buff;
    buff.assign(buffer, buff_size);
    stdin_activate_raw();

    while (keep_running)
    {
        read(STDIN_FILENO, &c, 1);
        switch (c)
        {
        case 127 :
            buff.pull_back();
            write(STDOUT_FILENO, backspace, 3);
            break;
        
        case '\n' :
            // ECHO disabled, we need to write the newline so the ouput won't be messed up
            write(STDOUT_FILENO, &c, 1);
            buff.push_back('\0');
            map_iter_array<char>(buff);
            if(std::strcmp(buff.begin(), "exit") == 0) {
                std::cout << "Minishell stopped..." << std::endl;
                keep_running = false;
                break;
            } else if (std::strlen(buffer) > 0) {
                std::cout << "Running a command..." << std::endl;
            }
            
            buff.assign(buffer, 1024);
            std::cout << "Minishell > " << std::flush;
            break;
        
        default:
            buff.push_back(c);
            write(STDOUT_FILENO, &c, 1);
            break;
        }
    }
}