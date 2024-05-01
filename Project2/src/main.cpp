#include <cstring>
#include <iostream>

#include "ArgParser.h"
#include "Renderer.h"

/*
build\a2.exe -size 800 800 -input data/scene07_arch.txt -bounces 4 -shadows -output out\a07.png -normals out\a07n.png -depth 8 18 out\a07d.png
*/
// const char *myargs[] = {
//     "build\\a2.exe",
//     "-size",
//     "800",
//     "800",
//     "-input",
//     "..\\data\\scene07_arch.txt",
//     "-bounces",
//     "4",
//     "-shadows",
//     "-output",
//     "..\\out\\a07.png",
//     "-normals",
//     "..\\out\\a07n.png",
//     "-depth",
//     "8",
//     "18",
//     "..\\out\\a07d.png"};

// build\a2.exe -size 800 800 -input data/scene06_bunny_1k.txt -bounces 4 -output out\a06.png -normals out\a06n.png -depth 8 18 out\a06d.png
const char* myargs[] = {
    "a2.exe",
    "-size",
    "800",
    "800",
    "-input",
    "../data/scene06_bunny_1k.txt",
    "-bounces",
    "4",
    "-output",
    "../out/a06.png",
    "-normals",
    "../out/a06n.png",
    "-depth",
    "8",
    "18",
    "../out/a06d.png"
};

// %BIN% -size %SIZE% -input data/scene01_plane.txt  -output out\a01.png -normals out\a01n.png -depth 8 18 out\a01d.png
// const char *myargs[] = {
//     "a2.exe",
//     "-size",
//     "800",
//     "800",
//     "-input",
//     "../data/scene01_plane.txt",
//     "-output",
//     "../out/a01.png",
//     "-normals",
//     "../out/a01n.png",
//     "-depth",
//     "8",
//     "18",
//     "../out/a01d.png"
// };

int main(int argc, const char *argv[])
{
    // Report help usage if no args specified.
    // if (argc == 1) {
    //     std::cout << "Usage: a5 <args>\n"
    //         << "\n"
    //         << "Args:\n"
    //         << "\t-input <scene>\n"
    //         << "\t-size <width> <height>\n"
    //         << "\t-output <image.png>\n"
    //         << "\t[-depth <depth_min> <depth_max> <depth_image.png>\n]"
    //         << "\t[-normals <normals_image.png>]\n"
    //         << "\t[-bounces <max_bounces>\n]"
    //         << "\t[-shadows\n]"
    //         << "\n"
    //         ;
    //     return 1;
    // }

    ArgParser argsParser(argc, argv);
    Renderer renderer(argsParser);
    //Renderer renderer({sizeof(myargs) / sizeof(*myargs), myargs});
    renderer.Render();
    return 0;
}
