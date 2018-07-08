#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>

struct DebugHelperClass {
    DebugHelperClass(QString funcInfo) : m_funcInfo(funcInfo) {
        qDebug().noquote() << "start" << m_funcInfo << QThread::currentThreadId();
        m_timer.start();
    }

    ~DebugHelperClass() {
        if (m_timer.elapsed() > 0) {
            qDebug().noquote() /*<< QByteArray(debugHelper_indent, '\t')*/ << "\t" << m_funcInfo << m_timer.elapsed() << "ms";
        }
    }

    void tick(int line) {
        qDebug().noquote() << '=' << m_funcInfo << ":" << line << "\t" << m_timer.elapsed() << "ms";
    }

    QString m_funcInfo;
    QElapsedTimer m_timer;
};

#define DEBUG_BLOCK DebugHelperClass debugHelper__Block(__PRETTY_FUNCTION__);
#define DEBUG_TICK debugHelper__Block.tick(__LINE__);

#endif // DEBUG_H
