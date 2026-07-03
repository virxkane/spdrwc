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
            // TODO:
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
    for (const auto& info : allPorts) {
        out << "Port: " << info.portName() << Qt::endl;
        out << "Location: " << info.systemLocation() << Qt::endl;
        out << "Description: " << info.systemLocation() << Qt::endl;
        out << "Manufacturer: " << info.manufacturer() << Qt::endl;
        out << "Serial number: " << info.serialNumber() << Qt::endl;

        QSerialPort port(info);
        // TODO: set serial port options
        // TODO: test communication with target device via this port
    }
    return 1;
}
