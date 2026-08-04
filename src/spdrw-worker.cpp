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
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDataStream>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <cstdio>

#include <QtCore/QDebug>

#define MAX_SPD_SZ 4096

SpdRwWorker::SpdRwWorker()
        : QObject(nullptr) { }

void SpdRwWorker::mainWork(const QVariantMap& params) {
    qDebug() << "params: " << params;
    int result = 1;
    CommandType type = static_cast<CommandType>(params.value("command").toInt());
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
        case CheckWP:
            result = cmdCheckWP(args);
            break;
        case EnableWP:
            result = cmdEnableWP(args);
            break;
        case DisableWP:
            result = cmdDisableWP(args);
            break;
        case EnablePWP:
            result = cmdEnablePWP(args);
            break;
        case Read:
            result = cmdRead(args);
            break;
        case Write:
            result = cmdWrite(args);
            break;
        case SaveFirmware:
            result = cmdSaveFirmware(args);
            break;
        default:
            qDebug() << "Unknown command type: " << type;
            break;
    }
    emit finishedWithCode(result);
}

int SpdRwWorker::cmdFind(const QStringList& args) {
    // Arguments: none
    QTextStream out(stdout);
    QTextStream err(stderr);

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
        bool have_errors = false;

        // Test communication with target device via this port...
        try {
            qDebug() << "Probing port" << port << "...";
            have_errors = !arduino.executeCommand<bool>(SpdRwArduino::Command::Test);
        } catch (const SpdRwArduino::SpdRwArduinoException& e) {
            err << e.what() << Qt::endl;
            have_errors = true;
        }
        if (!have_errors) {
            out << port << ":" << settings.BaudRate << Qt::endl;
            count++;
        }
    }
    return count > 0 ? 0 : -1;
}

int SpdRwWorker::cmdScanDevice(const QStringList& args) {
    // Arguments:
    //  [0] - <port>:<baudRate>
    QTextStream out(stdout);
    QTextStream err(stderr);

    SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
    ArduinoAddress address;
    if (!args.isEmpty())
        address = parseArduinoAddress(args[0]);
    if (address.portName.isEmpty()) {
        err << "invalid arguments: " << convertToString(args);
        return 1;
    }

    settings.BaudRate = address.baudRate;
    SpdRwArduino arduino(address.portName, settings);
    bool have_errors = false;
    QList<int> addresses;

    try {
        have_errors = !arduino.executeCommand<bool>(SpdRwArduino::Command::Test);
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << e.what() << Qt::endl;
        have_errors = true;
    }
    if (have_errors) {
        err << "Communication test failed!" << Qt::endl;
        return -1;
    }

    try {
        auto fw_version = arduino.executeCommand<uint32_t>(SpdRwArduino::Command::Version);
        out << "Firmware version: " << fw_version << Qt::endl;
        // TODO: Test firmware version
        // TODO: Firmware must be added into program resources
        auto addressMask = arduino.executeCommand<uint8_t>(SpdRwArduino::Command::ScanBus);
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t mask = 1 << i;
            if (addressMask & mask) {
                addresses.append(80 + i);
            }
        }
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "SpdRwArduinoException: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (!have_errors && !addresses.isEmpty()) {
        for (const int addr : addresses) {
            out << "Found EEPROM at address " << addr << Qt::endl;
        }
    } else {
        err << "No EEPROM found" << Qt::endl;
    }
    return !addresses.isEmpty() ? 0 : -1;
}

#define SPD_DATA_LENGTH_COUNT 4
static const uint16_t s_spd_length[SPD_DATA_LENGTH_COUNT] {
    0,   // Unknown
    256, // Minimal
    512, // DDR4
    1024 // DDR5
};

int SpdRwWorker::cmdRead(const QStringList& args) {
    // Arguments:
    //  [0] - <port>:<baudRate>
    //  [1] - <I2C Address>
    //  [2] - <target file name>
    QTextStream out(stdout);
    QTextStream err(stderr);
    if (args.size() < 3) {
        err << "Not enough arguments!" << Qt::endl;
        return -1;
    }
    SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
    ArduinoAddress address = parseArduinoAddress(args[0]);
    bool ok = false;
    const int i2cAddress = args[1].toInt(&ok);
    if (!ok || i2cAddress < 1) {
        err << "Invalid I2C address: " << args[1] << Qt::endl;
        return -1;
    }
    if (address.portName.isEmpty()) {
        err << "invalid arguments: " << convertToString(args);
        return -1;
    }
    const QString& filename = args[2];

    settings.BaudRate = address.baudRate;
    SpdRwArduino arduino(address.portName, settings);
    bool have_errors = false;

    // TODO: Validate EEPROM address
    // TODO: Validate PMIC address

    // Test device communication
    // This is required step, without this EEPROM Read function result (and may be other) will be invalid!
    try {
        have_errors = !arduino.executeCommand<bool>(SpdRwArduino::Command::Test);
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "SpdRwArduinoException: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (have_errors) {
        err << "Communication test failed!" << Qt::endl;
        return -1;
    }

    // TODO: Check Firmware version

    // Get SPD size
    uint8_t sz_code = 0;
    uint16_t spd_sz = 0;
    QByteArray spd_data;
    QByteArray cmd_args;
    cmd_args.append(static_cast<char>(i2cAddress));
    try {
        sz_code = arduino.executeCommand<uint8_t>(SpdRwArduino::Command::Size, cmd_args);
        if (sz_code < SPD_DATA_LENGTH_COUNT)
            spd_sz = s_spd_length[sz_code];
        else
            have_errors = true;
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "Get EEPROM size failed: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (!have_errors && spd_sz > 0) {
        out << "Start to read " << spd_sz << " bytes..." << Qt::endl;
        // Read EEPROM bytes & combine into array
        cmd_args.resize(4);
        constexpr uint8_t block_sz = 32;
        cmd_args[0] = static_cast<char>(i2cAddress);
        for (uint16_t offset = 0; offset < spd_sz; offset += block_sz) {
            uint16_t part_sz = spd_sz - offset;
            if (part_sz > block_sz)
                part_sz = block_sz;
            cmd_args[1] = static_cast<char>(offset >> 8); // MSB
            cmd_args[2] = static_cast<char>(offset);      // LSB
            cmd_args[3] = static_cast<char>(part_sz);
            try {
                auto part = arduino.executeCommand<QByteArray>(SpdRwArduino::Command::ReadByte, cmd_args);
                if (part.size() == part_sz) {
                    spd_data.append(part);
                } else {
                    err << "SPD Read failed: count = " << part.size();
                    have_errors = true;
                    break;
                }
                out << "+" << part.size() << "bytes" << Qt::endl;
            } catch (const SpdRwArduino::SpdRwArduinoException& e) {
                err << "SPD Read failed: " << e.what() << Qt::endl;
                have_errors = true;
            }
            if (have_errors)
                break;
        }
        out << "Read " << spd_data.size() << " bytes" << Qt::endl;
    } else {
        err << "Invalid SPD size code: " << sz_code << Qt::endl;
        have_errors = true;
    }
    if (!have_errors) {
        // Save to file
        QFile file(filename);
        if (file.open(QFile::ReadWrite)) {
            QDataStream stream(&file);
            qint64 wb = stream.writeRawData(spd_data.constData(), spd_data.size());
            if (wb != spd_data.size()) {
                err << "Write to file failed: " << filename << Qt::endl;
                have_errors = true;
            }
            file.close();
        } else {
            err << "Failed to open file: " << filename << Qt::endl;
            have_errors = true;
        }
    }
    return !have_errors ? 0 : -1;
}

int SpdRwWorker::cmdWrite(const QStringList& args) {
    // Arguments:
    //  [0] - <port>:<baudRate>
    //  [1] - <I2C Address>
    //  [2] - <source file name>
    QTextStream out(stdout);
    QTextStream err(stderr);
    if (args.size() < 3) {
        err << "Not enough arguments!" << Qt::endl;
        return -1;
    }
    SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
    ArduinoAddress address = parseArduinoAddress(args[0]);
    bool ok = false;
    const int i2cAddress = args[1].toInt(&ok);
    if (!ok || i2cAddress < 1) {
        err << "Invalid I2C address: " << args[1] << Qt::endl;
        return -1;
    }
    if (address.portName.isEmpty()) {
        err << "invalid arguments: " << convertToString(args);
        return -1;
    }
    const QString& filename = args[2];
    QFile file(filename);
    if (file.size() < 128) {
        err << "SPD file size is too small: " << file.size() << Qt::endl;
        return -1;
    }
    if (file.size() >= MAX_SPD_SZ) {
        err << "SPD file size is too big: " << file.size() << Qt::endl;
        return -1;
    }
    QByteArray spd_data;
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        char buff[MAX_SPD_SZ];
        qint64 rb = stream.readRawData(buff, MAX_SPD_SZ);
        if (rb > 0 && rb == file.size()) {
            spd_data.append(buff, rb);
        }
    }
    if (spd_data.isEmpty()) {
        err << "Failed to read SPD data from file!" << Qt::endl;
        return -1;
    }

    // TODO: Validate SPD data

    // TODO: Validate EEPROM address
    // TODO: Validate PMIC address

    settings.BaudRate = address.baudRate;
    SpdRwArduino arduino(address.portName, settings);
    bool have_errors = false;

    // Test device communication
    // This is required step, without this EEPROM Read function result (and may be other) will be invalid!
    try {
        have_errors = !arduino.executeCommand<bool>(SpdRwArduino::Command::Test);
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "SpdRwArduinoException: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (have_errors) {
        err << "Communication test failed!" << Qt::endl;
        return -1;
    }

    // TODO: Check Firmware version

    // Get SPD size
    QByteArray cmd_args;
    uint8_t sz_code = 0;
    uint16_t spd_sz = 0;
    cmd_args.append(static_cast<char>(i2cAddress));
    try {
        sz_code = arduino.executeCommand<uint8_t>(SpdRwArduino::Command::Size, cmd_args);
        if (sz_code < SPD_DATA_LENGTH_COUNT)
            spd_sz = s_spd_length[sz_code];
        else
            have_errors = true;
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "Get EEPROM size failed: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (!have_errors && spd_sz > 0) {
        if (spd_sz == spd_data.size()) {
            // Write SPD data to EEPROM
            out << "Start writing SPD data to EEPROM..." << Qt::endl;
            cmd_args.resize(4);
            cmd_args[0] = static_cast<char>(i2cAddress);
            for (uint16_t offset = 0; offset < static_cast<uint16_t>(spd_data.size()); offset++) {
                cmd_args[1] = static_cast<char>(offset >> 8); // MSB
                cmd_args[2] = static_cast<char>(offset);      // LSB
                cmd_args[3] = static_cast<char>(1);
                try {
                    const auto b = arduino.executeCommand<uint8_t>(SpdRwArduino::Command::ReadByte, cmd_args);
                    const auto v = spd_data[offset];
                    if (b != v) {
                        cmd_args[3] = static_cast<char>(v);
                        auto write_res = arduino.executeCommand<bool>(SpdRwArduino::Command::WriteByte, cmd_args);
                        if (!write_res) {
                            err << "Failed to write SPD data to EEPROM at offset=" << offset << "!" << Qt::endl;
                            have_errors = true;
                            break;
                        }
                    }
                } catch (const SpdRwArduino::SpdRwArduinoException& e) {
                    err << "SpdRwArduinoException: " << e.what() << Qt::endl;
                    have_errors = true;
                    break;
                }
            }
            if (!have_errors) {
                out << "Done without error" << Qt::endl;
            }
        } else {
            err << "SPD EEPROM size != SPD data in file" << sz_code << Qt::endl;
            have_errors = true;
        }
    } else {
        err << "Invalid SPD size code: " << sz_code << Qt::endl;
        have_errors = true;
    }
    return !have_errors ? 0 : -1;
}

int SpdRwWorker::cmdCheckWP(const QStringList& args) {
    // Arguments:
    //  [0] - <port>:<baudRate>
    //  [1] - <I2C Address>
    QTextStream out(stdout);
    QTextStream err(stderr);
    if (args.size() < 2) {
        err << "Not enough arguments!" << Qt::endl;
        return -1;
    }
    SpdRwArduino::ReaderSettings settings = { 115200, 3000 };
    ArduinoAddress address = parseArduinoAddress(args[0]);
    bool ok = false;
    const int i2cAddress = args[1].toInt(&ok);
    if (!ok || i2cAddress < 1) {
        err << "Invalid I2C address: " << args[1] << Qt::endl;
        return -1;
    }
    if (address.portName.isEmpty()) {
        err << "invalid arguments: " << convertToString(args);
        return -1;
    }

    // TODO: Validate EEPROM address
    // TODO: Validate PMIC address

    settings.BaudRate = address.baudRate;
    SpdRwArduino arduino(address.portName, settings);
    bool have_errors = false;

    // Test device communication
    // This is required step, without this EEPROM Read function result (and may be other) will be invalid!
    try {
        if (!arduino.executeCommand<bool>(SpdRwArduino::Command::Test))
            have_errors = true;
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "SpdRwArduinoException: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (have_errors) {
        err << "Communication test failed!" << Qt::endl;
        return -1;
    }

    // TODO: Check Firmware version

    QByteArray cmd_args;
    cmd_args.append(static_cast<char>(i2cAddress));

    // Read SPD EEPROM byte #2 (module type code)
    ModuleType moduleType = Unknown;
    cmd_args.resize(4);
    try {
        cmd_args[1] = 0; // SPD offset: MSB
        cmd_args[2] = 2; // SPD offset: LSB
        cmd_args[3] = 1; // count
        auto byte02_data = arduino.executeCommand<QByteArray>(SpdRwArduino::Command::ReadByte, cmd_args);
        if (byte02_data.size() == 1) {
            switch (byte02_data[0]) {
                case 0x0B:
                    moduleType = DDR3;
                    break;
                case 0x0C:
                    moduleType = DDR4;
                    break;
                case 0x12:
                    moduleType = DDR5;
                    break;
                default:
                    err << "Unknown module type: code=" << static_cast<int>(byte02_data[0]) << Qt::endl;
                    have_errors = true;
                    break;
            }
        } else {
            err << "Read SPD byte #2 failed: read bytes = " << byte02_data.size() << Qt::endl;
            have_errors = true;
        }
    } catch (const SpdRwArduino::SpdRwArduinoException& e) {
        err << "Failed to read SPD Data byte #2: " << e.what() << Qt::endl;
        have_errors = true;
    }
    if (have_errors) {
        return -1;
    }

    int eeprom_block_count = -1;
    int eeprom_block_size = -1;
    QList<bool> eeprom_block_status;
    switch (moduleType) {
        case DDR3:
            // DDR3 - 2 blocks by 128 bytes
            eeprom_block_count = 2;
            eeprom_block_size = 128;
            break;
        case DDR4:
            // DDR4 - 4 blocks by 128 bytes
            eeprom_block_count = 4;
            eeprom_block_size = 128;
            break;
        case DDR5:
            // DDR5 - 16 blocks by 64 bytes
            eeprom_block_count = 16;
            eeprom_block_size = 64;
            break;
    }

    // Check each block for WP
    eeprom_block_status.resize(eeprom_block_count);
    cmd_args.resize(3);
    cmd_args[2] = SpdRwArduino::Command::Get;
    for (int i = 0; i < eeprom_block_count; i++) {
        cmd_args[1] = static_cast<char>(i); // block #
        try {
            auto status = arduino.executeCommand<bool>(SpdRwArduino::Command::Rswp, cmd_args);
            eeprom_block_status[i] = status;
        } catch (const SpdRwArduino::SpdRwArduinoException& e) {
            err << e.what() << Qt::endl;
            have_errors = true;
            break;
        }
    }

    if (!have_errors) {
        // Print check result
        int start = 0;
        int end = eeprom_block_size - 1;
        for (int i = 0; i < eeprom_block_count; i++) {
            std::string status = eeprom_block_status[i] ? "protected" : "writable";
            out << "block #" << i << " (" << start << "-" << end << "): " << status.c_str() << Qt::endl;
            start += eeprom_block_size;
            end += eeprom_block_size;
        }
        return 0;
    }
    return -1;
}

int SpdRwWorker::cmdEnableWP(const QStringList& args) {
    // TODO: implement this
    return -1;
}

int SpdRwWorker::cmdDisableWP(const QStringList& args) {
    // TODO: implement this
    return -1;
}

int SpdRwWorker::cmdEnablePWP(const QStringList& args) {
    // TODO: implement this
    return -1;
}

int SpdRwWorker::cmdSaveFirmware(const QStringList& args) {
    // TODO: implement this
    return -1;
}

struct SpdRwWorker::ArduinoAddress SpdRwWorker::parseArduinoAddress(const QString& str) {
    SpdRwWorker::ArduinoAddress address;
    address.portName = "";
    address.baudRate = 0;
    QStringList list = str.split(":");
    QString port;
    int baudRate = 0;
    bool good_params = false;
    if (list.size() == 2) {
        port = list[0];
        const QString& baudRate_str = list[1];
        bool ok = false;
        int b = baudRate_str.toInt(&ok);
        if (ok) {
            baudRate = b;
            good_params = true;
        }
    }
    if (good_params) {
        address.portName = port;
        address.baudRate = baudRate;
    }
    return address;
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
