#include <source_location>
#include <string>

void foo(const std::source_location &src = std::source_location::current());

int main()
{}
