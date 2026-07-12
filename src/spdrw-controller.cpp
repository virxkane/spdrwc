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

#include "spdrw-controller.h"
#include "spdrw-worker.h"

#include <QtCore/QThread>

SpdRwController::SpdRwController(QObject* parent)
        : QObject(parent)
        , m_exitCode(255) {
    m_workerThread = new QThread(this);
    m_worker = new SpdRwWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, SIGNAL(operate(QVariantMap)), m_worker, SLOT(mainWork(QVariantMap)));
    connect(m_worker, SIGNAL(finishedWithCode(int)), this, SLOT(handleResults(int)));

    m_workerThread->start();
}

SpdRwController::~SpdRwController() {
    if (m_workerThread->isRunning()) {
        m_workerThread->exit(m_exitCode);
        m_workerThread->wait();
    }
    delete m_workerThread;
    delete m_worker;
}

void SpdRwController::handleResults(int exitCode) {
    // When the worker thread emits the `finished` signal,
    // it is assumed that all work is complete, and we should stop the thread.
    m_exitCode = exitCode;
    m_workerThread->exit(m_exitCode);
    emit exit(m_exitCode);
}
