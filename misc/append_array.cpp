#include <iostream>
#include <limits>
namespace {
  void appendNumber(int*& array, unsigned& size, int number);
  // static appendNumber(). i.e., local function in this compiling unit
} // Anonymous namespace
int main() {
  int min = std::numeric_limits<int>::max();
  int max = std::numeric_limits<int>::min();
  long long sum = 0;
  // Pointer to the dynamically allocated array
  int* numbers = nullptr;
  unsigned numbersSize = 0;
  do {
    std::cout << "Please, enter a number: ";
    int number;
    if (std::cin >> number) {
      sum += number;
      appendNumber(numbers /* array */, numbersSize /* size */, number);
      if (number > max) {
        max = number;
      }
      if (number < min) {
        min = number;
      }
    } else {
      // Error in parsing an int,
      // flush the buffer from possible garbage characters
      std::cin.clear();
      // Consume all characters in standard input until new line
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
          std::cin.widen('\n'));
    }
  } while (!std::cin.eof());
  if (numbersSize != 0) {
    constexpr auto NUMBERS_PER_ROW = 5U;
    std::cout << "\nNumbers entered:" << '\n';
    for (decltype(numbersSize) /* i.e., unsigned */ i = 0U; i < numbersSize;
         ++i) {
      std::cout << numbers[i] << ' ';
      if (((i + 1) % NUMBERS_PER_ROW) == 0) {
        std::cout << '\n';
      }
    }
    if ((numbersSize % NUMBERS_PER_ROW) != 0) {
      std::cout << '\n';
    }
    std::cout << numbersSize << " numbers with average "
        << double(sum) / numbersSize << '\n';
    std::cout << "min=" << min << ", max=" << max << '\n';
    delete[] numbers;
  }
}
namespace {
void appendNumber(int*& array, unsigned& size, const int number) {
  int* newArray = new int[size + 1];
  for (unsigned i{} /* = 0 */; i < size; ++i) {
    newArray[i] = array[i];
  }
  // Above loop conceptually same as: std::copy(array, array + size, newArray);
  // Append the new number, and update size
  newArray[size++] = number;
  delete[] array; // OK, even if array is nullptr
  array = newArray;
}
}