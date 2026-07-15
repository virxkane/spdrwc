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

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QVariantMap>
#include <QtCore/QTextStream>

#include <QtCore/QDebug>

#include "spdrw-controller.h"
#include "spdrw-worker.h"

#include <cstdio>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("spdrwc");
    QCoreApplication::setApplicationVersion(VERSION);

    // Parse command line options...
    QVariantMap params;
    QCommandLineParser parser;
    parser.setApplicationDescription("CLI program for Arduino based SPD-RW");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption commandOption(QStringList() << "c" << "command");
    commandOption.setDescription("Specify command to execute.");
    commandOption.setValueName("command");
    parser.addOption(commandOption);

    parser.addPositionalArgument("args", "Command arguments", "[args...]");

    parser.process(app);

    QString scmd = parser.value("command");
    SpdRwWorker::CommandType cmd = SpdRwWorker::CommandType::None;

    if ("find" == scmd) {
        cmd = SpdRwWorker::CommandType::Find;
    } else if ("scan" == scmd) {
        cmd = SpdRwWorker::CommandType::Scan;
    } else if ("checkwp" == scmd) {
        cmd = SpdRwWorker::CommandType::CheckWP;
    } else if ("enablewp" == scmd) {
        cmd = SpdRwWorker::CommandType::EnableWP;
    } else if ("disablewp" == scmd) {
        cmd = SpdRwWorker::CommandType::DisableWP;
    } else if ("enablepwp" == scmd) {
        cmd = SpdRwWorker::CommandType::EnablePWP;
    } else if ("read" == scmd) {
        cmd = SpdRwWorker::CommandType::Read;
    } else if ("write" == scmd) {
        cmd = SpdRwWorker::CommandType::Write;
    } else if ("firmware" == scmd) {
        cmd = SpdRwWorker::CommandType::SaveFirmware;
    } else {
        cmd = SpdRwWorker::CommandType::None;
    }

    if (SpdRwWorker::CommandType::None == cmd) {
        QTextStream out(stdout);
        out << "Invalid command:" << scmd << "\n";
        parser.showHelp(1);
    }

    params["command"] = cmd;
    params["args"] = parser.positionalArguments();

    //qDebug() << "params: " << params;

    auto* controller = new SpdRwController(nullptr);
    QObject::connect(controller, SIGNAL(exit(int)), &app, SLOT(exit(int)));
    emit controller->operate(params);

    // Starts the event loop
    int res = app.exec();

    // rewrite exit code
    res = controller->exitCode();

    // Any cleanup operations
    delete controller;

    return res;
}
