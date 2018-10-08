#pragma once

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>

struct DebugHelperClass {
    DebugHelperClass(QString funcInfo) : m_funcInfo(funcInfo) {
        //qDebug().noquote() << "start" << m_funcInfo << QThread::currentThreadId();
        m_timer.start();
    }

    ~DebugHelperClass() {
        if (m_timer.elapsed() > 50) {
            qDebug().noquote() /*<< QByteArray(debugHelper_indent, '\t')*/ << "\t" << m_funcInfo << m_timer.elapsed() << "ms";
        }
    }

    void tick(int line) {
        qDebug().noquote() << '=' << m_funcInfo << ":" << line << "\t" << m_timer.elapsed() << "ms";
    }

    QString m_funcInfo;
    QElapsedTimer m_timer;
};

#ifdef SLAQ_DEBUG
#define DEBUG_BLOCK DebugHelperClass debugHelper__Block(__PRETTY_FUNCTION__);
#define DEBUG_TICK debugHelper__Block.tick(__LINE__);
#else
#define DEBUG_BLOCK
#define DEBUG_TICK
#endif
