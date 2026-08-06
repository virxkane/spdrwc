/***************************************************************************
 *   spdrwc                                                                *
 *   Copyright (C) 2026 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "spdrw-arduino.h"

#include <QtSerialPort/QSerialPort>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTextStream>

#define READ_BUFF_SZ 64

SpdRwArduino::SpdRwArduino(const QString& portName, struct ReaderSettings& settings) {
    m_port = new QSerialPort(portName);
    m_port->setBaudRate(settings.BaudRate);
    m_timeout = settings.timeout;
    m_valid = m_port->open(QIODevice::ReadWrite);
    m_portName = portName + QString(":") + QString::number(settings.BaudRate);
}

SpdRwArduino::~SpdRwArduino() {
    delete m_port;
}

const QString& SpdRwArduino::portName() const {
    return m_portName;
}

bool SpdRwArduino::executeCommandBool(const QByteArray& cmd) {
    QByteArray response = executeCommandRaw(cmd);
    if (response.isEmpty())
        throw SpdRwArduinoReadException(0, 1);
    qDebug() << "BOOL:" << response.toHex(' ');
    return response[0] != 0;
}

uint8_t SpdRwArduino::executeCommandByte(const QByteArray& cmd) {
    QByteArray response = executeCommandRaw(cmd);
    if (response.isEmpty())
        throw SpdRwArduinoReadException(0, 1);
    qDebug() << "BYTE:" << response.toHex(' ');
    return static_cast<uint8_t>(response[0]);
}

QByteArray SpdRwArduino::executeCommandBytes(const QByteArray& cmd) {
    QByteArray response = executeCommandRaw(cmd);
    if (response.isEmpty())
        throw SpdRwArduinoReadException();
    qDebug() << "BYTES:" << response.toHex(' ');
    return response;
}

uint16_t SpdRwArduino::executeCommandWORD(const QByteArray& cmd) {
    QByteArray response = executeCommandRaw(cmd);
    if (response.size() < 2)
        throw SpdRwArduinoReadException(static_cast<int>(response.size()), 2);
    qDebug() << "WORD:" << response.toHex(' ');
    return static_cast<uint8_t>(response[0]) | static_cast<uint8_t>(response[1]) << 8;
}

uint32_t SpdRwArduino::executeCommandDWORD(const QByteArray& cmd) {
    QByteArray response = executeCommandRaw(cmd);
    if (response.size() < 4)
        throw SpdRwArduinoReadException(static_cast<int>(response.size()), 4);
    qDebug() << "DWORD:" << response.toHex(' ');
    return static_cast<uint8_t>(response[0]) | static_cast<uint8_t>(response[1]) << 8 |
           static_cast<uint8_t>(response[2]) << 16 | static_cast<uint8_t>(response[3]) << 24;
}

QByteArray SpdRwArduino::executeCommandRaw(const QByteArray& cmd) {
    // Response structure:
    //  Header type : 1
    //  Body size: 1
    //  Body: 1 - 32
    //  CRC: 1
    const qint64 min_size = 1 + 1 + 1 + 1;
    const qint64 max_size = 1 + 1 + 32 + 1;
    qint64 req_sz = min_size;
    // Write to port
    m_port->write(cmd);
    m_port->flush();
    QByteArray response;
    QElapsedTimer timer;
    timer.start();
    qint64 remainingT = m_timeout;
    qint64 remainingR = req_sz;
    // Read response
    char buff[READ_BUFF_SZ];
    qint64 trb = 0; // total read bytes
    bool wait_res = true;
    bool alertFound = false;
    bool haveSize = false;
    while (remainingR > 0 && wait_res) {
        qint64 rb = m_port->read(buff, remainingR);
        if (rb >= 0) {
            // no error, or data not ready yet
            if (rb > 0) {
                trb += rb;
                response.append(buff, rb);
            } else {
                if (remainingT > 0) {
                    wait_res = m_port->waitForReadyRead(remainingT);
                    remainingT = m_timeout - timer.elapsed();
                } else {
                    // read timeout
                    break;
                }
            }
        } else {
            // error or EOF
            break;
        }
        // Analyze response in-place
        if (response.size() >= min_size && !haveSize) {
            // Check Alert
            int size_pos = 1;
            if (response[0] == '@' && !alertFound) {
                alertFound = true;
                size_pos = 3;
            }
            uint8_t size = response[size_pos];
            req_sz = size + 3;
            if (req_sz > max_size) {
                qDebug() << "Invalid packet size" << req_sz << "limiting to max size = " << max_size;
                req_sz = max_size;
            }
            if (alertFound)
                req_sz += 2;
            haveSize = true;
        }
        remainingR = req_sz - trb;
    }
    if (alertFound) {
        // TODO: Process alert
        QTextStream err(stderr);
        err << "  ALERT found: '" << response[1] << "' (" << static_cast<int>(response[1]) << ")" << Qt::endl;
        // Then skip alert body
        response = response.mid(2);
    }
    if (response.size() >= min_size && response.size() <= max_size) {
        if (response[0] == '&') {
            // response
            uint8_t size = response[1];
            uint8_t crc = response[response.size() - 1];
            if (response.size() == size + 3) {
                QByteArray body = response.mid(2, size);
                if (crc == calcCRC(body)) {
                    return body;
                }
            }
        }
    }
    return QByteArray();
}

uint8_t SpdRwArduino::calcCRC(const QByteArray& cmd) {
    uint8_t result = 0;
    for (const uint8_t b : cmd) {
        result += b;
    }
    return result;
}
