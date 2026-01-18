/********************************************************************************
* @Author : hexne
* @Date   : 2026/01/16 15:40:52
********************************************************************************/

import modforge.directory;
import std;


bool test_directory() {
    Directory directory(".", true, 1);
    for (auto dir : directory.directorys(false))
        std::println("dir: {}", dir);

    for (auto file : directory.files("", false, true))
        std::println("file: {}", file);


    return 0;
}