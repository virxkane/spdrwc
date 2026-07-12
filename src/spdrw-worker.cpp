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

#include "spdrw-worker.h"
#include "spdrw-arduino.h"

#include <QtCore/QThread>
#include <QtCore/QTextStream>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <cstdio>

#include <QtCore/QDebug>

SpdRwWorker::SpdRwWorker()
        : QObject(nullptr) { }

void SpdRwWorker::mainWork(const QVariantMap& params) {
    qDebug() << "params: " << params;
    int result = 1;
    CommandType type = (CommandType)params.value("command").toInt();
    auto args = params.value("args").toStringList();
    switch (type) {
        case None:
            result = -1;
            break;
        case Find:
            result = cmdFind(args);
            break;
        case Scan:
            result = cmdScanDevice(args);
            break;
        case EnableWP:
            // TODO:
            break;
        case DisableWP:
            // TODO:
            break;
        case EnablePWP:
            // TODO:
            break;
        case Read:
            // TODO:
            break;
        case Write:
            // TODO:
            break;
        case SaveFirmware:
            // TODO:
            break;
        default:
            qDebug() << "Unknown command type: " << type;
            break;
    }
    emit finishedWithCode(result);
}

int SpdRwWorker::cmdFind(const QStringList& args) {
    QTextStream out(stdout);
    const auto allPorts = QSerialPortInfo::availablePorts();
    int count = 0;
    for (const auto& info : allPorts) {
        QString port = info.systemLocation();
        // TODO: use threads to test in parallel...
#if 1
        if (!port.startsWith("/dev/ttyUSB")) {
            continue;
        }
#endif
        // TODO: probe with few baud rates...
        SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
        SpdRwArduino arduino(port, settings);
        // Test communication with target device via this port...
        qDebug() << "Probing port" << port << "...";
        bool ok = arduino.executeCommand<bool>(SpdRwArduino::Test);
        if (ok) {
            out << port << ":" << settings.BaudRate << Qt::endl;
            count++;
        }
    }
    return count > 0 ? 0 : -1;
}

int SpdRwWorker::cmdScanDevice(const QStringList& args) {
    QTextStream out(stdout);
    QString port;
    SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
    bool good_params = false;
    if (!args.isEmpty()) {
        QString arg0 = args[0];
        QStringList list = arg0.split(":");
        if (list.size() == 2) {
            port = list[0];
            QString baudRate_str = list[1];
            bool ok = false;
            int b = baudRate_str.toInt(&ok);
            if (ok) {
                settings.BaudRate = b;
                good_params = true;
            }
        }
    }
    if (!good_params) {
        out << "invalid arguments: " << convertToString(args);
        return 1;
    }
    SpdRwArduino arduino(port, settings);
    QList<int> addresses;
    bool testRes = arduino.executeCommand<bool>(SpdRwArduino::Test);
    if (testRes) {
        // TODO: Get & test firmware version
        // TODO: Firmware must be added into program resources
        uint32_t fw_version = arduino.executeCommand<uint32_t>(SpdRwArduino::Version);
        out << "Firmware version: " << fw_version << Qt::endl;
        uint8_t addressMask = arduino.executeCommand<uint8_t>(SpdRwArduino::ScanBus);
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t mask = 1 << i;
            if (addressMask & mask) {
                addresses.append(80 + i);
            }
        }
    }
    if (!addresses.isEmpty()) {
        for (const int address : addresses) {
            out << "Found EEPROM at address " << address << Qt::endl;
        }
    } else {
        out << "No EEPROM found" << Qt::endl;
    }
    return 0;
}

QString SpdRwWorker::convertToString(const QStringList& params) {
    QString str = "QStringList";
    str += QString("[") + QString::number(params.size()) + QString("]");
    str += "{";
    QStringList::const_iterator it = params.begin();
    while (it != params.end()) {
        const QString& s = *it;
        str += s;
        ++it;
        if (it != params.end())
            str += ", ";
    }
    str += "}";
    return str;
}
