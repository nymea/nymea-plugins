/*****************************************************************************
 * Copyright: 2013 Michael Zanetti <michael_zanetti@gmx.net>                 *
 *                                                                           *
 * This file is part of ubuntu-authenticator                                 *
 *                                                                           *
 * This prject is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This project is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 ****************************************************************************/
/*
#include "qrcodereader.h"

#include <QDebug>
#include <QUrlQuery>
#include <QSettings>
#include <QStandardPaths>

QRCodeReader::QRCodeReader(QObject *parent) :
    QObject(parent)
{
    connect(&m_readerThread, &QThread::started, this, &QRCodeReader::scanningChanged);
    connect(&m_readerThread, &QThread::finished, this, &QRCodeReader::scanningChanged);
}

bool QRCodeReader::valid() const
{
    return !m_type.isEmpty() && !m_text.isEmpty();
}

QString QRCodeReader::type() const
{
    return m_type;
}

QString QRCodeReader::text() const
{
    return m_text;
}

QImage QRCodeReader::image() const
{
    return m_image;
}

QRect QRCodeReader::scanRect() const
{
    return m_scanRect;
}

void QRCodeReader::setScanRect(const QRect &rect)
{
    if (m_scanRect != rect) {
        m_scanRect = rect;
        emit scanRectChanged();
    }
}

bool QRCodeReader::scanning() const
{
    return m_readerThread.isRunning();
}

void QRCodeReader::grab(QImage image)
{
    m_type.clear();
    m_text.clear();
    emit validChanged();

    if (m_scanRect.isValid()) {
        image = image.copy(m_scanRect);
    }

    Reader *reader = new Reader;
    reader->moveToThread(&m_readerThread);
    connect(reader, SIGNAL(finished()), reader, SLOT(deleteLater()));
    connect(reader, SIGNAL(finished()), &m_readerThread, SLOT(quit()));
    connect(reader, SIGNAL(resultReady(QString, QString, QImage)), this, SLOT(handleResults(QString, QString, QImage)));
    m_readerThread.start();

    //QMetaObject::invokeMethod(reader, "doWork", Q_ARG(QImage, img), Q_ARG(bool, false));
}

void QRCodeReader::processImage(const QUrl &url)
{
    QImage image;
    if (!image.load(url.path())) {
        qWarning() << "can't open" << url.path();
        return;
    }

    Reader *reader = new Reader;
    reader->moveToThread(&m_readerThread);
    connect(reader, SIGNAL(finished()), reader, SLOT(deleteLater()));
    connect(reader, SIGNAL(finished()), &m_readerThread, SLOT(quit()));
    connect(reader, SIGNAL(resultReady(QString, QString, QImage)), this, SLOT(handleResults(QString, QString, QImage)));
    m_readerThread.start();

    QMetaObject::invokeMethod(reader, "doWork", Q_ARG(QImage, image), Q_ARG(bool, false));
}

void QRCodeReader::handleResults(const QString &type, const QString &text, const QImage &codeImage)
{
    m_type = type;
    m_text = text;
    m_image = codeImage;
    m_imageUuid = QUuid::createUuid();
    emit validChanged();
}

void Reader::doWork(const QImage &image, bool invert)
{
    // Prepare image
    QImage copy = image;
    if (invert) {
        copy.invertPixels();
    }
    zbar::QZBarImage img(copy.convertToFormat(QImage::Format_RGB32));
    zbar::Image tmp = img.convert(*(long*)"Y800");

    // create a reader
    zbar::ImageScanner scanner;

    // configure the reader
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_POSITION, 1);
    scanner.set_config(zbar::ZBAR_PARTIAL, zbar::ZBAR_CFG_ENABLE, 0);

    // scan the image for barcodes
    int n = scanner.scan(tmp);
//    qDebug() << "scanned. have" << n << "symbols";
    if (!invert && n == 0) {
        // Nothing found... try again inverted
        doWork(image, true);
        return;
    }

    img.set_symbols(tmp.get_symbols());

    // extract results
    for(zbar::Image::SymbolIterator symbol = img.symbol_begin(); symbol != img.symbol_end(); ++symbol) {

        QString typeName = QString::fromStdString(symbol->get_type_name());
        QString symbolString = QString::fromStdString(symbol->get_data());

        int x0 = 999999;
        int y0 = 999999;
        int x1 = 0;
        int y1 = 0;

        for (int i = 0; i < symbol->get_location_size(); ++i) {
            int x = symbol->get_location_x(i);
            int y = symbol->get_location_y(i);
            qDebug() << "got point" << x << y;
            if (x < x0) x0 = x;
            if (y < y0) y0 = y;
            if (x > x1) x1 = x;
            if (y > y1) y1 = y;
        }

        int width = x1 - x0;
        int height = y1 - y0;

        // Workaround for zBar sometimes only giving us the first bar in a barcode.
        if (width < 10) width = img.get_width() - x0;
        if (height < 10) height = img.get_height() - y0;

        qDebug() << "extracting code image (" << x0 << y0 << ") ("<< x1 << y1 << ")";
        QImage codeImage = image.copy(x0, y0, width, height);

        qDebug() << "Code recognized:" << typeName << ", Text:" << symbolString;

        emit resultReady(typeName, symbolString, codeImage);
    }

    tmp.set_data(NULL, 0);
    img.set_data(NULL, 0);

    emit finished();
}
*/
