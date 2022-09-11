/*
 * Pixmap Keyboard, a custom Qt4 widget
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "pixmapkeyboard.hpp"

#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#ifndef Q_COMPILER_LAMBDA
static std::map<int, QRectF> kMidiKey2RectMapHorizontal;
#else
static std::map<int, QRectF> kMidiKey2RectMapHorizontal = {
    {0,  QRectF(0,   0, 18, 64)}, // C
    {1,  QRectF(13,  0, 11, 42)}, // C#
    {2,  QRectF(18,  0, 25, 64)}, // D
    {3,  QRectF(37,  0, 11, 42)}, // D#
    {4,  QRectF(42,  0, 18, 64)}, // E
    {5,  QRectF(60,  0, 18, 64)}, // F
    {6,  QRectF(73,  0, 11, 42)}, // F#
    {7,  QRectF(78,  0, 25, 64)}, // G
    {8,  QRectF(97,  0, 11, 42)}, // G#
    {9,  QRectF(102, 0, 25, 64)}, // A
    {10, QRectF(121, 0, 11, 42)}, // A#
    {11, QRectF(126, 0, 18, 64)}  // B
};
#endif

#ifndef Q_COMPILER_LAMBDA
static std::map<int, QRectF> kMidiKey2RectMapVertical;
#else
static std::map<int, QRectF> kMidiKey2RectMapVertical = {
    {11, QRectF(0,  0,  64, 18)}, // B
    {10, QRectF(0, 14,  42,  7)}, // A#
    {9,  QRectF(0, 18,  64, 24)}, // A
    {8,  QRectF(0, 38,  42,  7)}, // G#
    {7,  QRectF(0, 42,  64, 24)}, // G
    {6,  QRectF(0, 62,  42,  7)}, // F#
    {5,  QRectF(0, 66,  64, 18)}, // F
    {4,  QRectF(0, 84,  64, 18)}, // E
    {3,  QRectF(0, 98,  42,  7)}, // D#
    {2,  QRectF(0, 102, 64, 24)}, // D
    {1,  QRectF(0, 122, 42,  7)}, // C#
    {0,  QRectF(0, 126, 64, 18)}  // C
};
#endif

#ifndef Q_COMPILER_LAMBDA
static std::map<int, int> kMidiKeyboard2KeyMap;
#else
static const std::map<int, int> kMidiKeyboard2KeyMap = {
    // bottom row, 3rd octave
    {Qt::Key_Z, 48},
    {Qt::Key_S, 49},
    {Qt::Key_X, 50},
    {Qt::Key_D, 51},
    {Qt::Key_C, 52},
    {Qt::Key_V, 53},
    {Qt::Key_G, 54},
    {Qt::Key_B, 55},
    {Qt::Key_H, 56},
    {Qt::Key_N, 57},
    {Qt::Key_J, 58},
    {Qt::Key_M, 59},
    // top row, 4th octave
    {Qt::Key_Q, 60},
    {Qt::Key_2, 61},
    {Qt::Key_W, 62},
    {Qt::Key_3, 63},
    {Qt::Key_E, 64},
    {Qt::Key_R, 65},
    {Qt::Key_5, 66},
    {Qt::Key_T, 67},
    {Qt::Key_6, 68},
    {Qt::Key_Y, 69},
    {Qt::Key_7, 70},
    {Qt::Key_U, 71},
    // continue the top row in to the 5th octave
    {Qt::Key_I, 72},
    {Qt::Key_9, 73},
    {Qt::Key_O, 74},
    {Qt::Key_0, 75},
    {Qt::Key_P, 76}
};
#endif

#ifndef Q_COMPILER_LAMBDA
static QVector<int> kBlackNotes;
#else
static const QVector<int> kBlackNotes = {1, 3, 6, 8, 10};
#endif

#ifndef Q_COMPILER_LAMBDA
static const struct PixmapKeyboardInit {
    PixmapKeyboardInit() {
        // kMidiKey2RectMapHorizontal
        kMidiKey2RectMapHorizontal[0]  = QRectF(0,   0, 18, 64); // C
        kMidiKey2RectMapHorizontal[1]  = QRectF(13,  0, 11, 42); // C#
        kMidiKey2RectMapHorizontal[2]  = QRectF(18,  0, 25, 64); // D
        kMidiKey2RectMapHorizontal[3]  = QRectF(37,  0, 11, 42); // D#
        kMidiKey2RectMapHorizontal[4]  = QRectF(42,  0, 18, 64); // E
        kMidiKey2RectMapHorizontal[5]  = QRectF(60,  0, 18, 64); // F
        kMidiKey2RectMapHorizontal[6]  = QRectF(73,  0, 11, 42); // F#
        kMidiKey2RectMapHorizontal[7]  = QRectF(78,  0, 25, 64); // G
        kMidiKey2RectMapHorizontal[8]  = QRectF(97,  0, 11, 42); // G#
        kMidiKey2RectMapHorizontal[9]  = QRectF(102, 0, 25, 64); // A
        kMidiKey2RectMapHorizontal[10] = QRectF(121, 0, 11, 42); // A#
        kMidiKey2RectMapHorizontal[11] = QRectF(126, 0, 18, 64); // B

        // kMidiKey2RectMapVertical
        kMidiKey2RectMapVertical[11] = QRectF(0,  0,  64, 18); // B
        kMidiKey2RectMapVertical[10] = QRectF(0, 14,  42,  7); // A#
        kMidiKey2RectMapVertical[9]  = QRectF(0, 18,  64, 24); // A
        kMidiKey2RectMapVertical[8]  = QRectF(0, 38,  42,  7); // G#
        kMidiKey2RectMapVertical[7]  = QRectF(0, 42,  64, 24); // G
        kMidiKey2RectMapVertical[6]  = QRectF(0, 62,  42,  7); // F#
        kMidiKey2RectMapVertical[5]  = QRectF(0, 66,  64, 18); // F
        kMidiKey2RectMapVertical[4]  = QRectF(0, 84,  64, 18); // E
        kMidiKey2RectMapVertical[3]  = QRectF(0, 98,  42,  7); // D#
        kMidiKey2RectMapVertical[2]  = QRectF(0, 102, 64, 24); // D
        kMidiKey2RectMapVertical[1]  = QRectF(0, 122, 42,  7); // C#
        kMidiKey2RectMapVertical[0]  = QRectF(0, 126, 64, 18); // C

        // kMidiKeyboard2KeyMap, 3th octave
        kMidiKeyboard2KeyMap[Qt::Key_Z] = 48;
        kMidiKeyboard2KeyMap[Qt::Key_S] = 49;
        kMidiKeyboard2KeyMap[Qt::Key_X] = 50;
        kMidiKeyboard2KeyMap[Qt::Key_D] = 51;
        kMidiKeyboard2KeyMap[Qt::Key_C] = 52;
        kMidiKeyboard2KeyMap[Qt::Key_V] = 53;
        kMidiKeyboard2KeyMap[Qt::Key_G] = 54;
        kMidiKeyboard2KeyMap[Qt::Key_B] = 55;
        kMidiKeyboard2KeyMap[Qt::Key_H] = 56;
        kMidiKeyboard2KeyMap[Qt::Key_N] = 57;
        kMidiKeyboard2KeyMap[Qt::Key_J] = 58;
        kMidiKeyboard2KeyMap[Qt::Key_M] = 59;
        // kMidiKeyboard2KeyMap, 4th octave
        kMidiKeyboard2KeyMap[Qt::Key_Q] = 60;
        kMidiKeyboard2KeyMap[Qt::Key_2] = 61;
        kMidiKeyboard2KeyMap[Qt::Key_W] = 62;
        kMidiKeyboard2KeyMap[Qt::Key_3] = 63;
        kMidiKeyboard2KeyMap[Qt::Key_E] = 64;
        kMidiKeyboard2KeyMap[Qt::Key_R] = 65;
        kMidiKeyboard2KeyMap[Qt::Key_5] = 66;
        kMidiKeyboard2KeyMap[Qt::Key_T] = 67;
        kMidiKeyboard2KeyMap[Qt::Key_6] = 68;
        kMidiKeyboard2KeyMap[Qt::Key_Y] = 69;
        kMidiKeyboard2KeyMap[Qt::Key_7] = 70;
        kMidiKeyboard2KeyMap[Qt::Key_U] = 71;
        // kMidiKeyboard2KeyMap, 5th octave
        kMidiKeyboard2KeyMap[Qt::Key_I] = 72;
        kMidiKeyboard2KeyMap[Qt::Key_9] = 73;
        kMidiKeyboard2KeyMap[Qt::Key_O] = 74;
        kMidiKeyboard2KeyMap[Qt::Key_0] = 75;
        kMidiKeyboard2KeyMap[Qt::Key_P] = 76;

        // kBlackNotes
        kBlackNotes << 1;
        kBlackNotes << 3;
        kBlackNotes << 6;
        kBlackNotes << 8;
        kBlackNotes << 10;
    }
} _pixmapKeyboardInitInit;
#endif

PixmapKeyboard::PixmapKeyboard(QWidget* parent)
    : QWidget(parent),
      fPixmap(""),
      fPixmapMode(HORIZONTAL),
      fColorStr("orange"),
      fFont("Monospace", 7, QFont::Normal),
      fOctaves(6),
      fLastMouseNote(-1),
      fWidth(0),
      fHeight(0),
      fMidiMap(kMidiKey2RectMapHorizontal)
{
    setCursor(Qt::PointingHandCursor);
    setMode(HORIZONTAL);
}

void PixmapKeyboard::allNotesOff()
{
    fEnabledKeys.clear();

    emit notesOff();
    update();
}

void PixmapKeyboard::sendNoteOn(int note, bool sendSignal)
{
    if (0 <= note && note <= 127 && ! fEnabledKeys.contains(note))
    {
        fEnabledKeys.append(note);

        if (sendSignal)
            emit noteOn(note);

        update();
    }

    if (fEnabledKeys.count() == 1)
        emit notesOn();
}

void PixmapKeyboard::sendNoteOff(int note, bool sendSignal)
{
    if (note >= 0 && note <= 127 && fEnabledKeys.contains(note))
    {
        fEnabledKeys.removeOne(note);

        if (sendSignal)
            emit noteOff(note);

        update();
    }

    if (fEnabledKeys.count() == 0)
        emit notesOff();
}

void PixmapKeyboard::setMode(Orientation mode, Color color)
{
    if (color == COLOR_CLASSIC)
    {
        fColorStr = "classic";
    }
    else if (color == COLOR_ORANGE)
    {
        fColorStr = "orange";
    }
    else
    {
        qCritical("PixmapKeyboard::setMode(%i, %i) - invalid color", mode, color);
        return setMode(mode);
    }

    if (mode == HORIZONTAL)
    {
        fMidiMap = kMidiKey2RectMapHorizontal;
        fPixmap.load(QString(":/bitmaps/kbd_h_%1.png").arg(fColorStr));
        fPixmapMode = HORIZONTAL;
        fWidth  = fPixmap.width();
        fHeight = fPixmap.height() / 2;
    }
    else if (mode == VERTICAL)
    {
        fMidiMap = kMidiKey2RectMapVertical;
        fPixmap.load(QString(":/bitmaps/kbd_v_%1.png").arg(fColorStr));
        fPixmapMode = VERTICAL;
        fWidth  = fPixmap.width() / 2;
        fHeight = fPixmap.height();
    }
    else
    {
        qCritical("PixmapKeyboard::setMode(%i, %i) - invalid mode", mode, color);
        return setMode(HORIZONTAL);
    }

    setOctaves(fOctaves);
}

void PixmapKeyboard::setOctaves(int octaves)
{
    Q_ASSERT(octaves >= 1 && octaves <= 10);

    if (octaves < 1)
        octaves = 1;
    else if (octaves > 10)
        octaves = 10;

    fOctaves = octaves;

    if (fPixmapMode == HORIZONTAL)
    {
        setMinimumSize(fWidth * fOctaves, fHeight);
        setMaximumSize(fWidth * fOctaves, fHeight);
    }
    else if (fPixmapMode == VERTICAL)
    {
        setMinimumSize(fWidth, fHeight * fOctaves);
        setMaximumSize(fWidth, fHeight * fOctaves);
    }

    update();
}

void PixmapKeyboard::handleMousePos(const QPoint& pos)
{
    int note, octave;
    QPointF keyPos;

    if (fPixmapMode == HORIZONTAL)
    {
        if (pos.x() < 0 or pos.x() > fOctaves * 144)
            return;
        int posX = pos.x() - 1;
        octave = posX / fWidth;
        keyPos = QPointF(posX % fWidth, pos.y());
    }
    else if (fPixmapMode == VERTICAL)
    {
        if (pos.y() < 0 or pos.y() > fOctaves * 144)
            return;
        int posY = pos.y() - 1;
        octave = fOctaves - posY / fHeight;
        keyPos = QPointF(pos.x(), posY % fHeight);
    }
    else
        return;

    if (fMidiMap[1].contains(keyPos))      // C#
        note = 1;
    else if (fMidiMap[3].contains(keyPos)) // D#
        note = 3;
    else if (fMidiMap[6].contains(keyPos)) // F#
        note = 6;
    else if (fMidiMap[8].contains(keyPos)) // G#
        note = 8;
    else if (fMidiMap[10].contains(keyPos))// A#
        note = 10;
    else if (fMidiMap[0].contains(keyPos)) // C
        note = 0;
    else if (fMidiMap[2].contains(keyPos)) // D
        note = 2;
    else if (fMidiMap[4].contains(keyPos)) // E
        note = 4;
    else if (fMidiMap[5].contains(keyPos)) // F
        note = 5;
    else if (fMidiMap[7].contains(keyPos)) // G
        note = 7;
    else if (fMidiMap[9].contains(keyPos)) // A
        note = 9;
    else if (fMidiMap[11].contains(keyPos))// B
        note = 11;
    else
        note = -1;

    if (note != -1)
    {
        note += octave * 12;

        if (fLastMouseNote != note)
        {
            sendNoteOff(fLastMouseNote);
            sendNoteOn(note);
        }
    }
    else if (fLastMouseNote != -1)
        sendNoteOff(fLastMouseNote);

    fLastMouseNote = note;
}

void PixmapKeyboard::keyPressEvent(QKeyEvent* event)
{
    if (! event->isAutoRepeat())
    {
        int qKey = event->key();
        std::map<int, int>::const_iterator it = kMidiKeyboard2KeyMap.find(qKey);
        if (it != kMidiKeyboard2KeyMap.end())
            sendNoteOn(it->second);
    }
    QWidget::keyPressEvent(event);
}

void PixmapKeyboard::keyReleaseEvent(QKeyEvent* event)
{
    if (! event->isAutoRepeat())
    {
        int qKey = event->key();
        std::map<int, int>::const_iterator it = kMidiKeyboard2KeyMap.find(qKey);
        if (it != kMidiKeyboard2KeyMap.end())
            sendNoteOff(it->second);
    }
    QWidget::keyReleaseEvent(event);
}

void PixmapKeyboard::mousePressEvent(QMouseEvent* event)
{
    fLastMouseNote = -1;
    handleMousePos(event->pos());
    setFocus();
    QWidget::mousePressEvent(event);
}

void PixmapKeyboard::mouseMoveEvent(QMouseEvent* event)
{
    handleMousePos(event->pos());
    QWidget::mouseMoveEvent(event);
}

void PixmapKeyboard::mouseReleaseEvent(QMouseEvent* event)
{
    if (fLastMouseNote != -1)
    {
        sendNoteOff(fLastMouseNote);
        fLastMouseNote = -1;
    }
    QWidget::mouseReleaseEvent(event);
}

void PixmapKeyboard::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    event->accept();

    // -------------------------------------------------------------
    // Paint clean keys (as background)

    for (int octave=0; octave < fOctaves; ++octave)
    {
        QRectF target;

        if (fPixmapMode == HORIZONTAL)
            target = QRectF(fWidth * octave, 0, fWidth, fHeight);
        else if (fPixmapMode == VERTICAL)
            target = QRectF(0, fHeight * octave, fWidth, fHeight);
        else
            return;

        QRectF source = QRectF(0, 0, fWidth, fHeight);
        painter.drawPixmap(target, fPixmap, source);
    }

    // -------------------------------------------------------------
    // Paint (white) pressed keys

    bool paintedWhite = false;

    for (int i=0, count=fEnabledKeys.count(); i < count; ++i)
    {
        int octave, note = fEnabledKeys[i];
        const QRectF& pos(_getRectFromMidiNote(note));

        if (_isNoteBlack(note))
            continue;

        if (note < 12)
            octave = 0;
        else if (note < 24)
            octave = 1;
        else if (note < 36)
            octave = 2;
        else if (note < 48)
            octave = 3;
        else if (note < 60)
            octave = 4;
        else if (note < 72)
            octave = 5;
        else if (note < 84)
            octave = 6;
        else if (note < 96)
            octave = 7;
        else if (note < 108)
            octave = 8;
        else if (note < 120)
            octave = 9;
        else if (note < 132)
            octave = 10;
        else
            // cannot paint this note
            continue;

        if (fPixmapMode == VERTICAL)
            octave = fOctaves - octave - 1;

        QRectF target, source;

        if (fPixmapMode == HORIZONTAL)
        {
            target = QRectF(pos.x() + (fWidth * octave), 0, pos.width(), pos.height());
            source = QRectF(pos.x(), fHeight, pos.width(), pos.height());
        }
        else if (fPixmapMode == VERTICAL)
        {
            target = QRectF(pos.x(), pos.y() + (fHeight * octave), pos.width(), pos.height());
            source = QRectF(fWidth, pos.y(), pos.width(), pos.height());
        }
        else
            return;

        paintedWhite = true;
        painter.drawPixmap(target, fPixmap, source);
    }

    // -------------------------------------------------------------
    // Clear white keys border

    if (paintedWhite)
    {
        for (int octave=0; octave < fOctaves; ++octave)
        {
            foreach (int note, kBlackNotes)
            {
                QRectF target, source;
                const QRectF& pos(_getRectFromMidiNote(note));

                if (fPixmapMode == HORIZONTAL)
                {
                    target = QRectF(pos.x() + (fWidth * octave), 0, pos.width(), pos.height());
                    source = QRectF(pos.x(), 0, pos.width(), pos.height());
                }
                else if (fPixmapMode == VERTICAL)
                {
                    target = QRectF(pos.x(), pos.y() + (fHeight * octave), pos.width(), pos.height());
                    source = QRectF(0, pos.y(), pos.width(), pos.height());
                }
                else
                    return;

                painter.drawPixmap(target, fPixmap, source);
            }
        }
    }

    // -------------------------------------------------------------
    // Paint (black) pressed keys

    for (int i=0, count=fEnabledKeys.count(); i < count; ++i)
    {
        int octave, note = fEnabledKeys[i];
        const QRectF& pos(_getRectFromMidiNote(note));

        if (! _isNoteBlack(note))
            continue;

        if (note < 12)
            octave = 0;
        else if (note < 24)
            octave = 1;
        else if (note < 36)
            octave = 2;
        else if (note < 48)
            octave = 3;
        else if (note < 60)
            octave = 4;
        else if (note < 72)
            octave = 5;
        else if (note < 84)
            octave = 6;
        else if (note < 96)
            octave = 7;
        else if (note < 108)
            octave = 8;
        else if (note < 120)
            octave = 9;
        else if (note < 132)
            octave = 10;
        else
            // cannot paint this note
            continue;

        if (fPixmapMode == VERTICAL)
            octave = fOctaves - octave - 1;

        QRectF target, source;

        if (fPixmapMode == HORIZONTAL)
        {
            target = QRectF(pos.x() + (fWidth * octave), 0, pos.width(), pos.height());
            source = QRectF(pos.x(), fHeight, pos.width(), pos.height());
        }
        else if (fPixmapMode == VERTICAL)
        {
            target = QRectF(pos.x(), pos.y() + (fHeight * octave), pos.width(), pos.height());
            source = QRectF(fWidth, pos.y(), pos.width(), pos.height());
        }
        else
            return;

        painter.drawPixmap(target, fPixmap, source);
    }

    // Paint C-number note info
    painter.setFont(fFont);
    painter.setPen(Qt::black);

    for (int i=0; i < fOctaves; ++i)
    {
        if (fPixmapMode == HORIZONTAL)
            painter.drawText(i * 144, 48, 18, 18, Qt::AlignCenter, QString("C%1").arg(i-1));
        else if (fPixmapMode == VERTICAL)
            painter.drawText(45, (fOctaves * 144) - (i * 144) - 16, 18, 18, Qt::AlignCenter, QString("C%1").arg(i-1));
    }
}

bool PixmapKeyboard::_isNoteBlack(int note) const
{
    const int baseNote = note % 12;
    return kBlackNotes.contains(baseNote);
}

const QRectF& PixmapKeyboard::_getRectFromMidiNote(int note) const
{
    const int baseNote = note % 12;
    return fMidiMap[baseNote];
}
