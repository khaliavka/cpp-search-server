// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
// 1000 - 9^3 = 271.
// Закомитьте изменения и отправьте их в свой репозиторий.
// Посчитаем перебором:
#include <iostream>

using namespace std;

bool IsRemainderThree(int x) {
    while (x > 0) {
        if (x % 10 == 3) {
            return true;
        } else {
            x /= 10;
        }
    }
    return false;
}

int main() {
    int x, count = 0;
    cout << "Please, input a positive integer:" << endl;
    cin >> x;
    while (x > 2) {
        if (IsRemainderThree(x)) {
            ++count;
        }
        --x;
    }
    cout << count << endl;
    return 0;
}