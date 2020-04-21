#include <iostream>
#include <vector>
#include <thread>

using namespace std;

void f1(int n)
{
    for (int i = 0; i < 5; ++i) {
        cout << "Thread " << n << " executing (" << i << ")" << endl;
        // this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void f2(int n, vector<int> &v)
{
    for (int i = 0; i < 5; ++i) {
        cout << "Thread " << n << " executing (" << i << ")" << endl;
        v.push_back(i);
        // this_thread::sleep_for(chrono::milliseconds(10));
    }
}

int main()
{
    int n = 0;
    vector<int> v;
    cout << "original v: " << v.size() << endl;
    // thread t1; // t1 is not a thread
    thread t1(f1, 1);
    thread t2(f1, 2); // pass by value
    thread t3(f2, 3, ref(v)); // pass by reference
    thread t4(f1, 4);
    thread t5(f1, 5);
    // thread t4(move(t3)); // t4 is now running f2(). t3 is no longer a thread
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    cout << "now v: " << v.size() << endl;
    
}