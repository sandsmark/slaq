/*
Copyright 2013 Antoine Lafarge qtwebsocket@gmail.com

This file is part of QtWebsocket.

QtWebsocket is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

QtWebsocket is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with QtWebsocket.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtCore/qmath.h>

#include <random>
#include <limits>

namespace QtWebsocket
{

static std::mt19937 &randomEngine()
{
    static std::random_device rd;
    static std::mt19937 engine(rd());
    return engine;
}

bool rand2()
{
    static std::uniform_int_distribution<uint8_t> gen(0, 1);
    return gen(randomEngine());
}

quint8 rand8(quint8 low, quint8 high)
{
    if (low == 0 && high == 0) {
        static std::uniform_int_distribution<quint8> gen;
        return gen(randomEngine());
	}

    std::uniform_int_distribution<quint8> gen(std::min(low, high), std::max(low, high));
    return gen(randomEngine());
}

quint16 rand16(quint16 low, quint16 high)
{
    if (low == 0 && high == 0) {
        static std::uniform_int_distribution<quint16> gen;
        return gen(randomEngine());
	}

    std::uniform_int_distribution<quint16> gen(std::min(low, high), std::max(low, high));
    return gen(randomEngine());
}

quint32 rand32(quint32 low, quint32 high)
{
    if (low == 0 && high == 0) {
        static std::uniform_int_distribution<quint32> gen;
        return gen(randomEngine());
    }

    std::uniform_int_distribution<quint32> gen(std::min(low, high), std::max(low, high));
    return gen(randomEngine());
}

quint64 rand64(quint64 low, quint64 high)
{
    if (low == 0 && high == 0) {
        static std::uniform_int_distribution<quint64> gen;
        return gen(randomEngine());
    }

    std::uniform_int_distribution<quint64> gen(std::min(low, high), std::max(low, high));
    return gen(randomEngine());
}

} // namespace QtWebsocket
