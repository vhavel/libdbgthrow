#include <stdexcept>

void f()
{
    try {
        throw std::runtime_error("");
    }
    catch (...) {
    }
}

int main()
{
    f();
    return 0;
}

