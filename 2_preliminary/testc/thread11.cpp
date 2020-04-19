#include <iostream>
#include <vector>
#include <thread>


void f1(int n)
{
    for (int i = 0; i < 5; ++i) {
        std::cout << "Thread " << n << " executing (" << i << ")" << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f2(int n, std::vector<int> &v)
{
    for (int i = 0; i < 5; ++i) {
        std::cout << "Thread " << n << " executing (" << i << ")" << std::endl;
        v.push_back(i);
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main()
{
    int n = 0;
    std::vector<int> v;
    std::cout << "original v: " << v.size() << std::endl;
    // std::thread t1; // t1 is not a thread
    std::thread t1(f1, 1);
    std::thread t2(f1, 2); // pass by value
    std::thread t3(f2, 3, std::ref(v)); // pass by reference
    std::thread t4(f1, 4);
    std::thread t5(f1, 5);
    // std::thread t4(std::move(t3)); // t4 is now running f2(). t3 is no longer a thread
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    std::cout << "now v: " << v.size() << std::endl;
    
}