#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <chrono>
#include <queue>
using namespace std;
using namespace std::chrono_literals;

// Global shared data
mutex mtx;
condition_variable cv;
queue<int> dataQueue;
atomic<bool> done{false};

// =============================================
// 1. Basic Thread Creation
// =============================================
void basicFunction() {
    cout << "Thread " << this_thread::get_id() << " is running\n";
}

// =============================================
// 2. Thread with Parameters
// =============================================
void printMessage(string msg, int times) {
    for (int i = 0; i < times; ++i) {
        lock_guard<mutex> lock(mtx);
        cout << "Thread " << this_thread::get_id() << ": " << msg << " #" << i << endl;
        this_thread::sleep_for(100ms);
    }
}

// =============================================
// 3. Lambda Thread
// =============================================
void lambdaExample() {
    thread t([]{
        cout << "Lambda thread running: " << this_thread::get_id() << endl;
    });
    t.join();
}

// =============================================
// 4. Join vs Detach
// =============================================
void joinDetachDemo() {
    thread t1([]{ cout << "Joined thread\n"; });
    thread t2([]{ cout << "Detached thread\n"; });

    t1.join();      // Wait for thread to finish (recommended)
    t2.detach();    // Let thread run independently (use carefully)
}

// =============================================
// 5. Mutex & Lock Guard (Synchronization)
// =============================================
void mutexExample() {
    vector<thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]{
            for (int j = 0; j < 3; ++j) {
                lock_guard<mutex> lock(mtx);   // RAII lock
                cout << "Thread " << i << " prints " << j << endl;
            }
        });
    }
    for (auto& t : threads) t.join();
}

// =============================================
// 6. Condition Variable
// =============================================
void producer() {
    for (int i = 0; i < 5; ++i) {
        {
            lock_guard<mutex> lock(mtx);
            dataQueue.push(i);
            cout << "Produced: " << i << endl;
        }
        cv.notify_one();
        this_thread::sleep_for(200ms);
    }
    done = true;
    cv.notify_all();
}

void consumer() {
    while (!done || !dataQueue.empty()) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []{ return !dataQueue.empty() || done; });

        while (!dataQueue.empty()) {
            cout << "Consumed: " << dataQueue.front() << endl;
            dataQueue.pop();
        }
    }
}

// =============================================
// 7. Atomic Variables
// =============================================
void atomicExample() {
    atomic<int> counter{0};

    vector<thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]{
            for (int j = 0; j < 1000; ++j) {
                counter.fetch_add(1, memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) t.join();

    cout << "Final counter = " << counter << endl;
}

// =============================================
// 8. std::async & Future (Higher level)
// =============================================
int compute(int x) {
    this_thread::sleep_for(500ms);
    return x * x;
}

void asyncExample() {
    future<int> f1 = async(launch::async, compute, 5);
    future<int> f2 = async(launch::async, compute, 8);

    cout << "5² = " << f1.get() << endl;
    cout << "8² = " << f2.get() << endl;
}

// =============================================
// 9. Thread Pool (Basic Example)
// =============================================
class ThreadPool {
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex poolMtx;
    condition_variable poolCv;
    bool stop = false;

public:
    ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]{
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(poolMtx);
                        poolCv.wait(lock, [this]{ return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& task) {
        {
            lock_guard<mutex> lock(poolMtx);
            tasks.emplace(forward<F>(task));
        }
        poolCv.notify_one();
    }

    ~ThreadPool() {
        {
            lock_guard<mutex> lock(poolMtx);
            stop = true;
        }
        poolCv.notify_all();
        for (auto& worker : workers) worker.join();
    }
};

// =============================================
// MAIN
// =============================================
int main() {
    cout << "=== C++ std::thread - All Major Usages ===\n\n";

    // 1. Basic
    thread t1(basicFunction);
    t1.join();

    // 2. With parameters
    thread t2(printMessage, "Hello", 3);
    t2.join();

    // 3. Lambda
    lambdaExample();

    // 4. Mutex
    cout << "\n--- Mutex Example ---\n";
    mutexExample();

    // 5. Producer-Consumer
    cout << "\n--- Producer Consumer ---\n";
    thread prod(producer);
    thread cons(consumer);
    prod.join();
    cons.join();

    // 6. Atomic
    cout << "\n--- Atomic Example ---\n";
    atomicExample();

    // 7. Async
    cout << "\n--- std::async Example ---\n";
    asyncExample();

    // 8. Simple Thread Pool
    cout << "\n--- Thread Pool Example ---\n";
    ThreadPool pool(4);
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i]{
            cout << "Task " << i << " executed by thread " 
                 << this_thread::get_id() << endl;
        });
    }

    cout << "\nAll thread examples completed!\n";
    return 0;
}