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

#ifndef SPDRW_WORKER_H
#define SPDRW_WORKER_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

class SpdRwWorker: public QObject
{
    Q_OBJECT
public:
    enum CommandType
    {
        None,        // NOOP
        Find,        // Find Device
        Scan,        // Scan Device
        EnableWP,    // Enable Write Protection
        DisableWP,   // Disable Write Protection
        EnablePWP,   // Enable Permanent Write Protection
        Read,        // Read SPD EEPROM
        Write,       // Write SPD EEPROM
        SaveFirmware // Save Firmware
    };
public:
    explicit SpdRwWorker();
    ~SpdRwWorker() override = default;
public slots:
    void mainWork(const QVariantMap& params);
signals:
    void finishedWithCode(int code);
protected:
    int cmdFind(const QStringList& args);
};

#endif
