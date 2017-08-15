#ifndef HTTPGET_H
#define HTTPGET_H

#include <QObject>
#include <QtNetwork>

class HttpGet : public QObject
{
    Q_OBJECT

    public:
        HttpGet(QObject *parent = 0);
        bool getFile(const QUrl &url);

    signals:
        void done();

    private slots:
        void httpDone(bool error);

    private:
        QHttp http;
        QUrl url;
        QFile file;
};

#endif // HTTPGET_H
