#pragma once

/*
This is a standard Queue class that stores values of the same type.
Stored variable type has to support:
1. Default constructor
2. Operator = for assignments
The switch autoResize determines whether the Queue should
automatically resize itself if enqueue is called when full.
*/

template <class valueType>
class Queue {
private:
    valueType* _values;
    unsigned long long int _size;
    unsigned long long int _front;
    unsigned long long int _rear;
    bool _full;

public:
    bool autoResize = true;

    Queue<valueType>(unsigned long long int size = 0) {
        _values = new valueType[size];
        _size = size;
        _front = 0;
        _rear = 0;
        _full = false;
    }
    ~Queue<valueType>() { delete[] _values; }

    bool isEmpty() const { return _front == _rear && (!_full || _size == 0); }
    bool isFull() const { return _full || _size == 0; }

    unsigned long long int getSize() const {
        if (_full || _size == 0) return _size;
        else return (_size + _rear - _front) % _size;
    }
    unsigned long long int getMaxSize() const {
        return _size;
    }
    bool resize(unsigned long long int newSize) {
        if (newSize <= getSize()) return newSize == getSize();
        Queue<valueType> temp(newSize);
        while (!(isEmpty())) temp.enqueue(dequeue());
        delete[] _values;
        _values = temp._values;
        _size = temp._size;
        _front = temp._front;
        _rear = temp._rear;
        _full = temp._full;
        temp._values = new valueType[0];
        return true;
    }

    bool enqueue(const valueType& value) {
        if (isFull()) {
            if (autoResize) resize(_size + 1);
            else return false;
        }
        _values[_rear] = value;
        _rear = (_rear + 1) % _size;
        if (_rear == _front) _full = true;
        return true;
    }
    valueType dequeue() {
        if (isEmpty()) throw "Uncaught exception: calling dequeue() on an empty queue";
        unsigned long long int temp = _front;
        _front = (_front + 1) % _size;
        _full = false;
        return _values[temp];
    }
    valueType peek() const {
        if (isEmpty()) throw "Uncaught exception: calling peek() on an empty queue";
        return _values[_front];
    }
};