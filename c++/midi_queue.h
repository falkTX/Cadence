/*
 * Simple Queue, specially developed for MIDI messages
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include <QtCore/QMutex>

class Queue
{
public:
    Queue()
    {
        index = 0;
        empty = true;
        full  = false;
    }

    void copyDataFrom(Queue* queue)
    {
        // lock mutexes
        queue->mutex.lock();
        mutex.lock();

        // copy data from queue
        memcpy(data, queue->data, sizeof(datatype)*MAX_SIZE);
        index = queue->index;
        empty = queue->empty;
        full  = queue->full;

        // unlock our mutex, no longer needed
        mutex.unlock();

        // reset queque
        memset(queue->data, 0, sizeof(datatype)*MAX_SIZE);
        queue->index = 0;
        queue->empty = true;
        queue->full  = false;

        // unlock queque mutex
        queue->mutex.unlock();
    }

    bool isEmpty()
    {
        return empty;
    }

    bool isFull()
    {
        return full;
    }

    void lock()
    {
        mutex.lock();
    }

    void unlock()
    {
        mutex.unlock();
    }

    void put(unsigned char d1, unsigned char d2, unsigned char d3, bool lock = true)
    {
        Q_ASSERT(d1 != 0);

        if (full || d1 == 0)
            return;

        if (lock)
            mutex.lock();

        for (unsigned short i=0; i < MAX_SIZE; i++)
        {
            if (data[i].d1 == 0)
            {
                data[i].d1 = d1;
                data[i].d2 = d2;
                data[i].d3 = d3;
                empty = false;
                full  = (i == MAX_SIZE-1);
                break;
            }
        }

        if (lock)
            mutex.unlock();
    }

    bool get(unsigned char* d1, unsigned char* d2, unsigned char* d3, bool lock = true)
    {
        Q_ASSERT(d1 && d2 && d3);

        if (empty || ! (d1 && d2 && d3))
            return false;

        if (lock)
            mutex.lock();

        full = false;

        if (data[index].d1 == 0)
        {
            index = 0;
            empty = true;

            if (lock)
                mutex.lock();

            return false;
        }

        *d1 = data[index].d1;
        *d2 = data[index].d2;
        *d3 = data[index].d3;

        data[index].d1 = data[index].d2 = data[index].d3 = 0;
        index++;
        empty = false;

        if (lock)
            mutex.unlock();

        return true;
    }

private:
    struct datatype {
        unsigned char d1, d2, d3;

        datatype()
            : d1(0), d2(0), d3(0) {}
    };

    static const unsigned short MAX_SIZE = 512;
    datatype data[MAX_SIZE];
    unsigned short index;
    bool empty, full;

    QMutex mutex;
};
