#include <iostream>
#include <vector>

int sum_vector(const std::vector<int>& values) {
    int sum = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        sum += values[i];
    }
    return sum;
}

int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

int main() {
    std::vector<int> data{3, 5, 2, 7, 4};
    int total = sum_vector(data);

    int n = 5;
    int fact = factorial(n);

    std::cout << "Sum = " << total << "\n";
    std::cout << "Factorial(" << n << ") = " << fact << "\n";
    return 0;
}
