#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

const int NUM_PROCESSES = 3;

class Clock {
public:
    virtual ~Clock() {}
    virtual void increment(int processId) = 0;
    virtual void update(const Clock* msgClock, int processId) = 0;
    virtual void print() const = 0;
    virtual Clock* clone() const = 0;
};

class LamportClock : public Clock {
private:
    int time;

public:
    LamportClock() : time(0) {}
    LamportClock(int t) : time(t) {}

    int getTime() const { return time; }

    void increment(int processId) override {
        time++;
    }

    void update(const Clock* msgClock, int processId) override {
        const LamportClock* lMsg = dynamic_cast<const LamportClock*>(msgClock);
        if (lMsg) {
            time = std::max(time, lMsg->time) + 1;
        }
    }

    void print() const override {
        std::cout << time;
    }

    Clock* clone() const override {
        return new LamportClock(time);
    }
};

class VectorClock : public Clock {
private:
    std::vector<int> vTime;

public:
    VectorClock() : vTime(NUM_PROCESSES, 0) {}
    VectorClock(const std::vector<int>& vt) : vTime(vt) {}

    std::vector<int> getVector() const { return vTime; }

    void increment(int processId) override {
        vTime[processId]++;
    }

    void update(const Clock* msgClock, int processId) override {
        const VectorClock* vMsg = dynamic_cast<const VectorClock*>(msgClock);
        if (vMsg) {
            for (int i = 0; i < NUM_PROCESSES; ++i) {
                vTime[i] = std::max(vTime[i], vMsg->vTime[i]);
            }
            vTime[processId]++;
        }
    }

    void print() const override {
        std::cout << "(";
        for (size_t i = 0; i < vTime.size(); ++i) {
            std::cout << vTime[i] << (i == vTime.size() - 1 ? "" : ", ");
        }
        std::cout << ")";
    }

    Clock* clone() const override {
        return new VectorClock(vTime);
    }

    bool isLessOrEqual(const VectorClock& other) const {
        for (int i = 0; i < NUM_PROCESSES; ++i) {
            if (this->vTime[i] > other.vTime[i]) {
                return false;
            }
        }
        return true;
    }
};

class Process {
private:
    int id;
    LamportClock lClock;
    VectorClock vClock;

public:
    Process(int processId) : id(processId) {}

    void localEvent(const std::string& name) {
        lClock.increment(id);
        vClock.increment(id);
        std::cout << "P" << id + 1 << " : " << name << " -> L: ";
        lClock.print();
        std::cout << " | V: ";
        vClock.print();
        std::cout << "\n";
    }

    void sendMessage(const std::string& name, Clock*& outL, Clock*& outV) {
        lClock.increment(id);
        vClock.increment(id);
        outL = lClock.clone();
        outV = vClock.clone();
        std::cout << "P" << id + 1 << " : Отправка " << name << " -> L: ";
        lClock.print();
        std::cout << " | V: ";
        vClock.print();
        std::cout << "\n";
    }

    void receiveMessage(const std::string& name, const Clock* msgL, const Clock* msgV) {
        lClock.update(msgL, id);
        vClock.update(msgV, id);
        std::cout << "P" << id + 1 << " : Получение " << name << " -> L: ";
        lClock.print();
        std::cout << " | V: ";
        vClock.print();
        std::cout << "\n";
    }

    VectorClock getVectorClock() const { return vClock; }
};

int main() {
    Process p1(0), p2(1), p3(2);
    Clock* msgLamport = nullptr;
    Clock* msgVector = nullptr;

    p1.localEvent("e1");
    VectorClock initialP1 = p1.getVectorClock(); 

    p2.localEvent("local_p2_1");
    p2.localEvent("local_p2_2");

    p1.sendMessage("M1", msgLamport, msgVector);
    p2.receiveMessage("M1", msgLamport, msgVector);
    delete msgLamport; 
    delete msgVector;

    p2.sendMessage("M2", msgLamport, msgVector);
    p3.receiveMessage("e''", msgLamport, msgVector);
    delete msgLamport;
    delete msgVector;

    std::cout << "\nРезультат теста:\n";
    VectorClock finalP3 = p3.getVectorClock();

    std::cout << "Вектор P1 (e1): "; initialP1.print(); std::cout << "\n";
    std::cout << "Вектор P3 (e''): "; finalP3.print(); std::cout << "\n";

    if (initialP1.isLessOrEqual(finalP3)) {
        std::cout << "Условие e <= e' выполняется (true)\n";
    } else {
        std::cout << "Условие e <= e' не выполняется (false)\n";
    }

    return 0;
}
