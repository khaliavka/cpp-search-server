// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
<<<<<<< HEAD
// 10^3 - 9^3 = 271.
=======
// 1000 - 9^3 = 271.
>>>>>>> 99eb892d491ff3ce1602c2b0066d8f1126bdfb97
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