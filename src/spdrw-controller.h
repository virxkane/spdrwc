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

#ifndef SPDRW_CONTROLLER_H
#define SPDRW_CONTROLLER_H

#include <QtCore/QObject>
#include <QtCore/QMap>

class QThread;
class SpdRwWorker;

class SpdRwController: public QObject
{
    Q_OBJECT
public:
    explicit SpdRwController(QObject* parent = nullptr);
    ~SpdRwController() override;
    int exitCode() const {
        return m_exitCode;
    }
public slots:
    void handleResults(int exitCode);
signals:
    void operate(const QVariantMap& args);
    void exit(int exitCode);
private:
    SpdRwWorker* m_worker;
    QThread* m_workerThread;
    int m_exitCode;
};

#endif // SPDRW_CONTROLLER_H
