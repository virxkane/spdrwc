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

#ifndef SPDRW_ARDUINO_H
#define SPDRW_ARDUINO_H

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QVariant>

#include <cstdint>
#include <type_traits>

class QSerialPort;

class SpdRwArduino
{
public:
    /// Device commands
    enum Command
    {
        /// Gets current variable value
        Get = -1,
        /// Resets variable to default value
        Disable = 0,
        /// Modifies variable value
        Enable,
        /// Read byte
        ReadByte,
        /// Write byte
        WriteByte,
        /// Write page
        WritePage,
        /// Write protection test
        WriteTest,
        /// DDR4 detection
        Ddr4Detect,
        /// DDR5 detection
        Ddr5Detect,
        /// Access SPD5 Hub register space
        Spd5HubReg,
        /// Get EEPROM size
        Size,
        /// Scan I2C bus
        ScanBus,
        /// I2C clock control
        BusClock,
        /// Probe I2C address
        ProbeAddress,
        /// Config pin control
        PinControl,
        /// Reset config pins state to defaults
        PinReset,
        /// RSWP operation
        Rswp,
        /// PSWP operation
        Pswp,
        /// Report current RSWP capabilities
        RswpReport,
        /// Get Firmware version
        Version,
        /// Device Communication Test
        Test,
        /// Name controls
        Name,
        /// Restore device settings to default
        FactoryReset,
    };
    struct ReaderSettings
    {
        int BaudRate; // Port baud rate, number
        int timeout;  // I/O operations timeout, ms
    };
public:
    SpdRwArduino(const QString& portName, struct ReaderSettings& settings);
    virtual ~SpdRwArduino();
    template <typename T>
    T executeCommand(Command cmd, const QByteArray& args = QByteArray()) {
        QByteArray query;
        query.append((char)cmd);
        query.append(args);
        if constexpr (std::is_same_v<T, bool>)
            return executeCommandBool(query);
        if constexpr (std::is_same_v<T, uint8_t>)
            return executeCommandByte(query);
        if constexpr (std::is_same_v<T, uint16_t>)
            return executeCommandWORD(query);
        if constexpr (std::is_same_v<T, uint32_t>)
            return executeCommandDWORD(query);
        return static_cast<T>(0);
    }
protected:
    bool executeCommandBool(const QByteArray& cmd);
    uint8_t executeCommandByte(const QByteArray& cmd);
    uint16_t executeCommandWORD(const QByteArray& cmd);
    uint32_t executeCommandDWORD(const QByteArray& cmd);
    QByteArray executeCommandRaw(const QByteArray& cmd);
private:
    static uint8_t calcCRC(const QByteArray& cmd);
    QSerialPort* m_port;
    int m_timeout;
    bool m_valid;
};

#endif // SPDRW_ARDUINO_H
