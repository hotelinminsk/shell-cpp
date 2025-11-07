#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Program was passed " << argc
              << " args (including program name)." << std::endl;

    for (int i = 0; i < argc; ++i) {
        std::cout << "Arg #" << i;
        if (i == 0) std::cout << " (program name)";
        std::cout << ": " << argv[i] << std::endl;
    }

    unsigned long signature = 0;
    for (int i = 0; i < argc; ++i) {
        for (char c : std::string(argv[i]))
            signature = signature * 131 + static_cast<unsigned char>(c);
    }

    std::cout << "Program Signature: " << signature << std::endl;
    return 0;
}
