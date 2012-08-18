/*
 * Simple JACK Audio Meter
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

#include <QtCore/Qt>

#ifndef Q_COMPILER_LAMBDA
# define nullptr (0)
#endif

#define VERSION "0.5.0"

#include "../jack_utils.h"
#include "ui_xycontroller.h"

#include <QtCore/QMutex>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QGraphicsItem>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsSceneEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

// -------------------------------

float abs_f(const float value)
{
    return (value < 1.0f) ? -value : value;
}

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
            mutex.lock();

        return true;
    }

private:
    struct datatype {
        unsigned char d1, d2, d3;

        datatype()
            : d1(0), d2(0), d3(0) {}
    };

    static const unsigned short MAX_SIZE = 128;
    datatype data[MAX_SIZE];
    unsigned short index;
    bool empty, full;

    QMutex mutex;
};

jack_client_t* jClient = nullptr;
jack_port_t* jMidiInPort  = nullptr;
jack_port_t* jMidiOutPort = nullptr;

static Queue qMidiInData;
static Queue qMidiOutData;

QVector<QString> MIDI_CC_LIST;
void MIDI_CC_LIST__init()
{
    //MIDI_CC_LIST << "0x00 Bank Select";
    MIDI_CC_LIST << "0x01 Modulation";
    MIDI_CC_LIST << "0x02 Breath";
    MIDI_CC_LIST << "0x03 (Undefined)";
    MIDI_CC_LIST << "0x04 Foot";
    MIDI_CC_LIST << "0x05 Portamento";
    //MIDI_CC_LIST << "0x06 (Data Entry MSB)";
    MIDI_CC_LIST << "0x07 Volume";
    MIDI_CC_LIST << "0x08 Balance";
    MIDI_CC_LIST << "0x09 (Undefined)";
    MIDI_CC_LIST << "0x0A Pan";
    MIDI_CC_LIST << "0x0B Expression";
    MIDI_CC_LIST << "0x0C FX Control 1";
    MIDI_CC_LIST << "0x0D FX Control 2";
    MIDI_CC_LIST << "0x0E (Undefined)";
    MIDI_CC_LIST << "0x0F (Undefined)";
    MIDI_CC_LIST << "0x10 General Purpose 1";
    MIDI_CC_LIST << "0x11 General Purpose 2";
    MIDI_CC_LIST << "0x12 General Purpose 3";
    MIDI_CC_LIST << "0x13 General Purpose 4";
    MIDI_CC_LIST << "0x14 (Undefined)";
    MIDI_CC_LIST << "0x15 (Undefined)";
    MIDI_CC_LIST << "0x16 (Undefined)";
    MIDI_CC_LIST << "0x17 (Undefined)";
    MIDI_CC_LIST << "0x18 (Undefined)";
    MIDI_CC_LIST << "0x19 (Undefined)";
    MIDI_CC_LIST << "0x1A (Undefined)";
    MIDI_CC_LIST << "0x1B (Undefined)";
    MIDI_CC_LIST << "0x1C (Undefined)";
    MIDI_CC_LIST << "0x1D (Undefined)";
    MIDI_CC_LIST << "0x1E (Undefined)";
    MIDI_CC_LIST << "0x1F (Undefined)";
    //MIDI_CC_LIST << "0x20 *Bank Select";
    //MIDI_CC_LIST << "0x21 *Modulation";
    //MIDI_CC_LIST << "0x22 *Breath";
    //MIDI_CC_LIST << "0x23 *(Undefined)";
    //MIDI_CC_LIST << "0x24 *Foot";
    //MIDI_CC_LIST << "0x25 *Portamento";
    //MIDI_CC_LIST << "0x26 *(Data Entry MSB)";
    //MIDI_CC_LIST << "0x27 *Volume";
    //MIDI_CC_LIST << "0x28 *Balance";
    //MIDI_CC_LIST << "0x29 *(Undefined)";
    //MIDI_CC_LIST << "0x2A *Pan";
    //MIDI_CC_LIST << "0x2B *Expression";
    //MIDI_CC_LIST << "0x2C *FX *Control 1";
    //MIDI_CC_LIST << "0x2D *FX *Control 2";
    //MIDI_CC_LIST << "0x2E *(Undefined)";
    //MIDI_CC_LIST << "0x2F *(Undefined)";
    //MIDI_CC_LIST << "0x30 *General Purpose 1";
    //MIDI_CC_LIST << "0x31 *General Purpose 2";
    //MIDI_CC_LIST << "0x32 *General Purpose 3";
    //MIDI_CC_LIST << "0x33 *General Purpose 4";
    //MIDI_CC_LIST << "0x34 *(Undefined)";
    //MIDI_CC_LIST << "0x35 *(Undefined)";
    //MIDI_CC_LIST << "0x36 *(Undefined)";
    //MIDI_CC_LIST << "0x37 *(Undefined)";
    //MIDI_CC_LIST << "0x38 *(Undefined)";
    //MIDI_CC_LIST << "0x39 *(Undefined)";
    //MIDI_CC_LIST << "0x3A *(Undefined)";
    //MIDI_CC_LIST << "0x3B *(Undefined)";
    //MIDI_CC_LIST << "0x3C *(Undefined)";
    //MIDI_CC_LIST << "0x3D *(Undefined)";
    //MIDI_CC_LIST << "0x3E *(Undefined)";
    //MIDI_CC_LIST << "0x3F *(Undefined)";
    //MIDI_CC_LIST << "0x40 Damper On/Off"; // <63 off, >64 on
    //MIDI_CC_LIST << "0x41 Portamento On/Off"; // <63 off, >64 on
    //MIDI_CC_LIST << "0x42 Sostenuto On/Off"; // <63 off, >64 on
    //MIDI_CC_LIST << "0x43 Soft Pedal On/Off"; // <63 off, >64 on
    //MIDI_CC_LIST << "0x44 Legato Footswitch"; // <63 Normal, >64 Legato
    //MIDI_CC_LIST << "0x45 Hold 2"; // <63 off, >64 on
    MIDI_CC_LIST << "0x46 Control 1 [Variation]";
    MIDI_CC_LIST << "0x47 Control 2 [Timbre]";
    MIDI_CC_LIST << "0x48 Control 3 [Release]";
    MIDI_CC_LIST << "0x49 Control 4 [Attack]";
    MIDI_CC_LIST << "0x4A Control 5 [Brightness]";
    MIDI_CC_LIST << "0x4B Control 6 [Decay]";
    MIDI_CC_LIST << "0x4C Control 7 [Vib Rate]";
    MIDI_CC_LIST << "0x4D Control 8 [Vib Depth]";
    MIDI_CC_LIST << "0x4E Control 9 [Vib Delay]";
    MIDI_CC_LIST << "0x4F Control 10 [Undefined]";
    MIDI_CC_LIST << "0x50 General Purpose 5";
    MIDI_CC_LIST << "0x51 General Purpose 6";
    MIDI_CC_LIST << "0x52 General Purpose 8";
    MIDI_CC_LIST << "0x53 General Purpose 9";
    MIDI_CC_LIST << "0x54 Portamento Control";
    MIDI_CC_LIST << "0x5B FX 1 Depth [Reverb]";
    MIDI_CC_LIST << "0x5C FX 2 Depth [Tremolo]";
    MIDI_CC_LIST << "0x5D FX 3 Depth [Chorus]";
    MIDI_CC_LIST << "0x5E FX 4 Depth [Detune]";
    MIDI_CC_LIST << "0x5F FX 5 Depth [Phaser]";
}

// -------------------------------
// XY Controller Scene

class XYGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    XYGraphicsScene(QWidget* parent)
        : QGraphicsScene(parent),
          m_parent(parent)
    {
        cc_x = 1;
        cc_y = 2;

        m_mouseLock = false;
        m_smooth    = false;
        m_smooth_x  = 0.0f;
        m_smooth_y  = 0.0f;

        setBackgroundBrush(Qt::black);

        QPen   cursorPen(QColor(255, 255, 255), 2);
        QColor cursorBrush(255, 255, 255, 50);
        m_cursor = addEllipse(QRectF(-10, -10, 20, 20), cursorPen, cursorBrush);

        QPen linePen(QColor(200, 200, 200, 100), 1, Qt::DashLine);
        m_lineH = addLine(-9999, 0, 9999, 0, linePen);
        m_lineV = addLine(0, -9999, 0, 9999, linePen);

        p_size = QRectF(-100, -100, 100, 100);
    }

    void setControlX(int x)
    {
        cc_x = x;
    }

    void setControlY(int y)
    {
        cc_y = y;
    }

    void setChannels(QList<int> channels)
    {
        m_channels = channels;
    }

    void setPosX(float x, bool forward=true)
    {
        if (m_mouseLock)
            return;

        float posX = x * (p_size.x() + p_size.width());
        m_cursor->setPos(posX, m_cursor->y());
        m_lineV->setX(posX);

        if (forward)
        {
            float value = posX / (p_size.x() + p_size.width());
            sendMIDI(&value, nullptr);
        }
        else
            m_smooth_x = posX;
    }

    void setPosY(float y, bool forward=true)
    {
        if (m_mouseLock)
            return;

        float posY = y * (p_size.y() + p_size.height());
        m_cursor->setPos(m_cursor->x(), posY);
        m_lineH->setY(posY);

        if (forward)
        {
            float value = posY / (p_size.y() + p_size.height());
            sendMIDI(nullptr, &value);
        }
        else
            m_smooth_y = posY;
    }

    void setSmooth(bool smooth)
    {
        m_smooth = smooth;
    }

    void setSmoothValues(float x, float y)
    {
        m_smooth_x = x * (p_size.x() + p_size.width());
        m_smooth_y = y * (p_size.y() + p_size.height());
    }

    void handleCC(int param, int value)
    {
        bool sendUpdate = false;
        float xp, yp;
        xp = yp = 0.0f;

        if (param == cc_x)
        {
            sendUpdate = true;
            xp = float(value)/63 - 1.0f;
            yp = m_cursor->y() / (p_size.y() + p_size.height());

            setPosX(xp, false);
        }

        if (param == cc_y)
        {
            sendUpdate = true;
            xp = m_cursor->x() / (p_size.x() + p_size.width());
            yp = float(value)/63 - 1.0f;

            setPosY(yp, false);
        }

        if (xp < -1.0f)
            xp = -1.0f;
        else if (xp > 1.0f)
            xp = 1.0f;

        if (yp < -1.0f)
            yp = -1.0f;
        else if (yp > 1.0f)
            yp = 1.0f;

        if (sendUpdate)
            emit cursorMoved(xp, yp);
    }

    void updateSize(QSize size)
    {
        p_size.setRect(-(float(size.width())/2), -(float(size.height())/2), size.width(), size.height());
    }

    void updateSmooth()
    {
        if (! m_smooth)
            return;

        if (m_cursor->x() == m_smooth_x && m_cursor->y() == m_smooth_y)
            return;

        if (abs_f(m_cursor->x() - m_smooth_x) <= 0.0005f)
        {
            m_smooth_x = m_cursor->x();
            return;
        }
        if (abs_f(m_cursor->y() - m_smooth_y) <= 0.0005f)
        {
            m_smooth_y = m_cursor->y();
            return;
        }

        float newX = float(m_smooth_x + m_cursor->x()*7) / 8;
        float newY = float(m_smooth_y + m_cursor->y()*7) / 8;
        QPointF pos(newX, newY);

        m_cursor->setPos(pos);
        m_lineH->setY(pos.y());
        m_lineV->setX(pos.x());

        float xp = pos.x() / (p_size.x() + p_size.width());
        float yp = pos.y() / (p_size.y() + p_size.height());

        sendMIDI(&xp, &yp);
        emit cursorMoved(xp, yp);
    }

protected:
    void handleMousePos(QPointF pos)
    {
        if (! p_size.contains(pos))
        {
            if (pos.x() < p_size.x())
                pos.setX(p_size.x());
            else if (pos.x() > p_size.x() + p_size.width())
                pos.setX(p_size.x() + p_size.width());

            if (pos.y() < p_size.y())
                pos.setY(p_size.y());
            else if (pos.y() > p_size.y() + p_size.height())
                pos.setY(p_size.y() + p_size.height());
        }

        m_smooth_x = pos.x();
        m_smooth_y = pos.y();

        if (! m_smooth)
        {
            m_cursor->setPos(pos);
            m_lineH->setY(pos.y());
            m_lineV->setX(pos.x());

            float xp = pos.x() / (p_size.x() + p_size.width());
            float yp = pos.y() / (p_size.y() + p_size.height());

            sendMIDI(&xp, &yp);
            emit cursorMoved(xp, yp);
        }
    }

    void sendMIDI(float* xp=nullptr, float* yp=nullptr)
    {
        float rate = float(0xff) / 4;

        if (xp != nullptr)
        {
            int value = *xp * rate + rate;
            foreach (const int& channel, m_channels)
                qMidiOutData.put(0xB0 + channel - 1, cc_x, value);
        }

        if (yp != nullptr)
        {
            int value = *yp * rate + rate;
            foreach (const int& channel, m_channels)
                qMidiOutData.put(0xB0 + channel - 1, cc_y, value);
        }
    }

    void keyPressEvent(QKeyEvent* event)
    {
        event->accept();
    }

    void wheelEvent(QGraphicsSceneWheelEvent* event)
    {
        event->accept();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        m_mouseLock = true;
        handleMousePos(event->scenePos());
        parent()->setCursor(Qt::CrossCursor);
        QGraphicsScene::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event)
    {
        handleMousePos(event->scenePos());
        QGraphicsScene::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
        m_mouseLock = false;
        parent()->setCursor(Qt::ArrowCursor);
        QGraphicsScene::mouseReleaseEvent(event);
    }

signals:
    void cursorMoved(float, float);

private:
    int cc_x;
    int cc_y;
    QList<int> m_channels;

    bool  m_mouseLock;
    bool  m_smooth;
    float m_smooth_x;
    float m_smooth_y;

    QGraphicsEllipseItem* m_cursor;
    QGraphicsLineItem* m_lineH;
    QGraphicsLineItem* m_lineV;

    QRectF p_size;

    // fake parent
    QWidget* const m_parent;
    QWidget* parent() const
    {
        return m_parent;
    }
};

// -------------------------------
// XY Controller Window

namespace Ui {
class XYControllerW;
}

class XYControllerW : public QMainWindow
{
    Q_OBJECT

public:
    XYControllerW()
        : QMainWindow(nullptr),
          settings("Cadence", "XY-Controller"),
          scene(this),
          ui(new Ui::XYControllerW)
    {
        ui->setupUi(this);

        // -------------------------------------------------------------
        // Internal stuff

        cc_x = 1;
        cc_y = 2;

        // -------------------------------------------------------------
        // Set-up GUI stuff

        ui->dial_x->setPixmap(2);
        ui->dial_y->setPixmap(2);
        ui->dial_x->setLabel("X");
        ui->dial_y->setLabel("Y");
        ui->keyboard->setOctaves(6);

        ui->graphicsView->setScene(&scene);
        ui->graphicsView->setRenderHints(QPainter::Antialiasing);

        foreach (const QString& MIDI_CC, MIDI_CC_LIST)
        {
            ui->cb_control_x->addItem(MIDI_CC);
            ui->cb_control_y->addItem(MIDI_CC);
        }

        // -------------------------------------------------------------
        // Load Settings

        loadSettings();

        // -------------------------------------------------------------
        // Connect actions to functions

        connect(ui->keyboard, SIGNAL(noteOn(int)), SLOT(slot_noteOn(int)));
        connect(ui->keyboard, SIGNAL(noteOff(int)), SLOT(slot_noteOff(int)));

        connect(ui->cb_smooth, SIGNAL(clicked(bool)), SLOT(slot_setSmooth(bool)));

        connect(ui->dial_x, SIGNAL(valueChanged(int)), SLOT(slot_updateSceneX(int)));
        connect(ui->dial_y, SIGNAL(valueChanged(int)), SLOT(slot_updateSceneY(int)));

        connect(ui->cb_control_x, SIGNAL(currentIndexChanged(QString)), SLOT(slot_checkCC_X(QString)));
        connect(ui->cb_control_y, SIGNAL(currentIndexChanged(QString)), SLOT(slot_checkCC_Y(QString)));

        connect(&scene, SIGNAL(cursorMoved(float,float)), SLOT(slot_sceneCursorMoved(float,float)));

        connect(ui->act_ch_01, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_02, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_03, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_04, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_05, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_06, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_07, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_08, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_09, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_10, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_11, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_12, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_13, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_14, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_15, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_16, SIGNAL(triggered(bool)), SLOT(slot_checkChannel(bool)));
        connect(ui->act_ch_all, SIGNAL(triggered()), SLOT(slot_checkChannel_all()));
        connect(ui->act_ch_none, SIGNAL(triggered()), SLOT(slot_checkChannel_none()));

        connect(ui->act_show_keyboard, SIGNAL(triggered(bool)), SLOT(slot_showKeyboard(bool)));
        connect(ui->act_about, SIGNAL(triggered()), SLOT(slot_about()));

        // -------------------------------------------------------------
        // Final stuff

        m_midiInTimerId = startTimer(30);
        QTimer::singleShot(0, this, SLOT(slot_updateScreen()));
    }

    void updateScreen()
    {
        scene.updateSize(ui->graphicsView->size());
        ui->graphicsView->centerOn(0, 0);

        int dial_x = ui->dial_x->value();
        int dial_y = ui->dial_y->value();
        slot_updateSceneX(dial_x);
        slot_updateSceneY(dial_y);
        scene.setSmoothValues(float(dial_x) / 100, float(dial_y) / 100);
    }

protected slots:
    void slot_noteOn(int note)
    {
        foreach (const int& channel, m_channels)
            qMidiOutData.put(0x90 + channel - 1, note, 100);
    }

    void slot_noteOff(int note)
    {
        foreach (const int& channel, m_channels)
            qMidiOutData.put(0x80 + channel - 1, note, 0);
    }

    void slot_updateSceneX(int x)
    {
        scene.setSmoothValues(float(x) / 100, float(ui->dial_y->value()) / 100);
        scene.setPosX(float(x) / 100, bool(sender()));
    }

    void slot_updateSceneY(int y)
    {
        scene.setSmoothValues(float(ui->dial_x->value()) / 100, float(y) / 100);
        scene.setPosY(float(y) / 100, bool(sender()));
    }

    void slot_checkCC_X(QString text)
    {
        if (text.isEmpty())
            return;

        bool ok;
        int tmp_cc_x = text.split(" ").at(0).toInt(&ok, 16);

        if (ok)
        {
            cc_x = tmp_cc_x;
            scene.setControlX(cc_x);
        }
    }

    void slot_checkCC_Y(QString text)
    {
        if (text.isEmpty())
            return;

        bool ok;
        int tmp_cc_y = text.split(" ").at(0).toInt(&ok, 16);

        if (ok)
        {
            cc_y = tmp_cc_y;
            scene.setControlY(cc_y);
        }
    }

    void slot_checkChannel(bool clicked)
    {
        if (! sender())
            return;

        bool ok;
        int channel = ((QAction*)sender())->text().toInt(&ok);

        if (ok)
        {
            if (clicked && ! m_channels.contains(channel))
                m_channels.append(channel);
            else if ((! clicked) && m_channels.contains(channel))
                m_channels.removeOne(channel);
            scene.setChannels(m_channels);
        }
    }

    void slot_checkChannel_all()
    {
        ui->act_ch_01->setChecked(true);
        ui->act_ch_02->setChecked(true);
        ui->act_ch_03->setChecked(true);
        ui->act_ch_04->setChecked(true);
        ui->act_ch_05->setChecked(true);
        ui->act_ch_06->setChecked(true);
        ui->act_ch_07->setChecked(true);
        ui->act_ch_08->setChecked(true);
        ui->act_ch_09->setChecked(true);
        ui->act_ch_10->setChecked(true);
        ui->act_ch_11->setChecked(true);
        ui->act_ch_12->setChecked(true);
        ui->act_ch_13->setChecked(true);
        ui->act_ch_14->setChecked(true);
        ui->act_ch_15->setChecked(true);
        ui->act_ch_16->setChecked(true);

#ifdef Q_COMPILER_INITIALIZER_LISTS
        m_channels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
#else
        m_channels.clear();

        for (int i=1; i <= 16; i++)
            m_channels << i;
#endif
        scene.setChannels(m_channels);
    }

    void slot_checkChannel_none()
    {
        ui->act_ch_01->setChecked(false);
        ui->act_ch_02->setChecked(false);
        ui->act_ch_03->setChecked(false);
        ui->act_ch_04->setChecked(false);
        ui->act_ch_05->setChecked(false);
        ui->act_ch_06->setChecked(false);
        ui->act_ch_07->setChecked(false);
        ui->act_ch_08->setChecked(false);
        ui->act_ch_09->setChecked(false);
        ui->act_ch_10->setChecked(false);
        ui->act_ch_11->setChecked(false);
        ui->act_ch_12->setChecked(false);
        ui->act_ch_13->setChecked(false);
        ui->act_ch_14->setChecked(false);
        ui->act_ch_15->setChecked(false);
        ui->act_ch_16->setChecked(false);

        m_channels.clear();
        scene.setChannels(m_channels);
    }

    void slot_setSmooth(bool yesno)
    {
        scene.setSmooth(yesno);
    }

    void slot_sceneCursorMoved(float xp, float yp)
    {
        ui->dial_x->blockSignals(true);
        ui->dial_y->blockSignals(true);

        ui->dial_x->setValue(xp * 100);
        ui->dial_y->setValue(yp * 100);

        ui->dial_x->blockSignals(false);
        ui->dial_y->blockSignals(false);
    }

    void slot_showKeyboard(bool yesno)
    {
        ui->scrollArea->setVisible(yesno);
        QTimer::singleShot(0, this, SLOT(slot_updateScreen()));
    }

    void slot_about()
    {
        QMessageBox::about(this, tr("About XY Controller"), tr("<h3>XY Controller</h3>"
                                                               "<br>Version %1"
                                                               "<br>XY Controller is a simple XY widget that sends and receives data from Jack MIDI.<br>"
                                                               "<br>Copyright (C) 2012 falkTX").arg(VERSION));
    }

    void slot_updateScreen()
    {
        updateScreen();
    }

protected:
    void saveSettings()
    {
        QVariantList varChannelList;
        foreach (const int& channel, m_channels)
            varChannelList << channel;

        settings.setValue("Geometry", saveGeometry());
        settings.setValue("ShowKeyboard", ui->scrollArea->isVisible());
        settings.setValue("Smooth", ui->cb_smooth->isChecked());
        settings.setValue("DialX", ui->dial_x->value());
        settings.setValue("DialY", ui->dial_y->value());
        settings.setValue("ControlX", cc_x);
        settings.setValue("ControlY", cc_y);
        settings.setValue("Channels", varChannelList);
    }

    void loadSettings()
    {
        restoreGeometry(settings.value("Geometry").toByteArray());

        bool showKeyboard = settings.value("ShowKeyboard", false).toBool();
        ui->act_show_keyboard->setChecked(showKeyboard);
        ui->scrollArea->setVisible(showKeyboard);

        bool smooth = settings.value("Smooth", false).toBool();
        ui->cb_smooth->setChecked(smooth);
        scene.setSmooth(smooth);

        ui->dial_x->setValue(settings.value("DialX", 50).toInt());
        ui->dial_y->setValue(settings.value("DialY", 50).toInt());

        cc_x = settings.value("ControlX", 1).toInt();
        cc_y = settings.value("ControlY", 2).toInt();
        scene.setControlX(cc_x);
        scene.setControlY(cc_y);

        m_channels.clear();

        if (settings.contains("Channels"))
        {
            QVariantList channels = settings.value("Channels").toList();

            foreach (const QVariant& var, channels)
            {
                bool ok;
                int channel = var.toInt(&ok);

                if (ok)
                    m_channels.append(channel);
            }
        }
        else
#ifdef Q_COMPILER_INITIALIZER_LISTS
            m_channels = { 1 };
#else
            m_channels << 1;
#endif

        scene.setChannels(m_channels);

        for (int i=0; i < MIDI_CC_LIST.size(); i++)
        {
            bool ok;
            int cc = MIDI_CC_LIST[i].split(" ").at(0).toInt(&ok, 16);

            if (ok)
            {
                if (cc_x == cc)
                    ui->cb_control_x->setCurrentIndex(i);
                if (cc_y == cc)
                    ui->cb_control_y->setCurrentIndex(i);
            }
        }

        if (m_channels.contains(1))
            ui->act_ch_01->setChecked(true);
        if (m_channels.contains(2))
            ui->act_ch_02->setChecked(true);
        if (m_channels.contains(3))
            ui->act_ch_03->setChecked(true);
        if (m_channels.contains(4))
            ui->act_ch_04->setChecked(true);
        if (m_channels.contains(5))
            ui->act_ch_05->setChecked(true);
        if (m_channels.contains(6))
            ui->act_ch_06->setChecked(true);
        if (m_channels.contains(7))
            ui->act_ch_07->setChecked(true);
        if (m_channels.contains(8))
            ui->act_ch_08->setChecked(true);
        if (m_channels.contains(9))
            ui->act_ch_09->setChecked(true);
        if (m_channels.contains(10))
            ui->act_ch_10->setChecked(true);
        if (m_channels.contains(11))
            ui->act_ch_11->setChecked(true);
        if (m_channels.contains(12))
            ui->act_ch_12->setChecked(true);
        if (m_channels.contains(13))
            ui->act_ch_13->setChecked(true);
        if (m_channels.contains(14))
            ui->act_ch_14->setChecked(true);
        if (m_channels.contains(15))
            ui->act_ch_15->setChecked(true);
        if (m_channels.contains(16))
            ui->act_ch_16->setChecked(true);
    }

    void timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == m_midiInTimerId)
        {
            if (! qMidiInData.isEmpty())
            {
                unsigned char d1, d2, d3;
                qMidiInInternal.copyDataFrom(&qMidiInData);

                while (qMidiInInternal.get(&d1, &d2, &d3, false))
                {
                    int channel = (d1 & 0x0F) + 1;
                    int mode    = d1 & 0xF0;

                    if (m_channels.contains(channel))
                    {
                        if (mode == 0x80)
                            ui->keyboard->sendNoteOff(d2, false);
                        else if (mode == 0x90)
                            ui->keyboard->sendNoteOn(d2, false);
                        else if (mode == 0xB0)
                            scene.handleCC(d2, d3);
                    }
                }
            }

            scene.updateSmooth();
        }

        QMainWindow::timerEvent(event);
    }

    void resizeEvent(QResizeEvent* event)
    {
        updateScreen();
        QMainWindow::resizeEvent(event);
    }

    void closeEvent(QCloseEvent* event)
    {
        saveSettings();
        QMainWindow::closeEvent(event);
    }

private:
    int cc_x;
    int cc_y;
    QList<int> m_channels;

    int m_midiInTimerId;

    QSettings settings;
    XYGraphicsScene scene;
    Ui::XYControllerW* const ui;

    Queue qMidiInInternal;
};

#include "xycontroller.moc"

// -------------------------------

int process_callback(const jack_nframes_t nframes, void*)
{
    void* const midiInBuffer  = jack_port_get_buffer(jMidiInPort, nframes);
    void* const midiOutBuffer = jack_port_get_buffer(jMidiOutPort, nframes);

    if (! (midiInBuffer && midiOutBuffer))
        return 1;

    // MIDI In
    jack_midi_event_t midiEvent;
    uint32_t midiEventCount = jack_midi_get_event_count(midiInBuffer);

    qMidiInData.lock();

    for (uint32_t i=0; i < midiEventCount; i++)
    {
        if (jack_midi_event_get(&midiEvent, midiInBuffer, i) != 0)
            break;

        if (midiEvent.size == 1)
            qMidiInData.put(midiEvent.buffer[0], 0, 0, false);
        else if (midiEvent.size == 2)
            qMidiInData.put(midiEvent.buffer[0], midiEvent.buffer[1], 0, false);
        else if (midiEvent.size >= 3)
            qMidiInData.put(midiEvent.buffer[0], midiEvent.buffer[1], midiEvent.buffer[2], false);

        if (qMidiInData.isFull())
            break;
    }
    qMidiInData.unlock();

    // MIDI Out
    jack_midi_clear_buffer(midiOutBuffer);
    qMidiOutData.lock();

    if (! qMidiOutData.isEmpty())
    {
        unsigned char d1, d2, d3, data[3];

        while (qMidiOutData.get(&d1, &d2, &d3, false))
        {
            data[0] = d1;
            data[1] = d2;
            data[2] = d3;
            jack_midi_event_write(midiOutBuffer, 0, data, 3);
        }
    }
    qMidiOutData.unlock();

    return 0;
}

#ifdef HAVE_JACKSESSION
void session_callback(jack_session_event_t* const event, void* const arg)
{
#ifdef Q_OS_LINUX
    QString filepath("cadence_xycontroller");
    Q_UNUSED(arg);
#else
    QString filepath((char*)arg);
#endif

    event->command_line = strdup(filepath.toUtf8().constData());

    jack_session_reply(jClient, event);

    if (event->type == JackSessionSaveAndQuit)
        QApplication::instance()->quit();

    jack_session_event_free(event);
}
#endif

// -------------------------------

int main(int argc, char* argv[])
{
    MIDI_CC_LIST__init();

#ifdef Q_OS_WIN
    QApplication::setGraphicsSystem("raster");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("XY-Controller");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("Cadence");
    //app.setWindowIcon(QIcon(":/48x48/xy-controller.png"));

    // JACK initialization
    jack_status_t jStatus;
#ifdef HAVE_JACKSESSION
    jack_options_t jOptions = static_cast<JackOptions>(JackNoStartServer|JackSessionID);
#else
    jack_options_t jOptions = static_cast<JackOptions>(JackNoStartServer);
#endif
    jClient = jack_client_open("XY-Controller", jOptions, &jStatus);

    if (! jClient)
    {
        std::string errorString(jack_status_get_error_string(jStatus));
        QMessageBox::critical(nullptr, app.translate("XY-Controller", "Error"), app.translate("XY-Controller",
                                                                                              "Could not connect to JACK, possible reasons:\n"
                                                                                              "%1").arg(QString::fromStdString(errorString)));
        return 1;
    }

    jMidiInPort  = jack_port_register(jClient, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    jMidiOutPort = jack_port_register(jClient, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    jack_set_process_callback(jClient, process_callback, nullptr);
#ifdef HAVE_JACKSESSION
    jack_set_session_callback(jClient, session_callback, argv[0]);
#endif
    jack_activate(jClient);

    // Show GUI
    XYControllerW gui;
    gui.show();

    // App-Loop
    int ret = app.exec();

    jack_deactivate(jClient);
    jack_client_close(jClient);

    return ret;
}
