#include <QCoreApplication>
#include <QtNetwork>
#include "httpget.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList args = a.arguments();
    /*
if (args.count() != 2) {
        cerr << "Usage: ftpget url" << endl
             << "Example:" << endl
             << "    ftpget ftp://ftp.trolltech.com/mirrors" << endl;
        return 1;
    }
    //FtpGet getter;
    if (!getter.getFile(QUrl(args[1])))
        return 1;
    */
    printf("hallo2\n");
    //QObject::connect(&getter, SIGNAL(done()), &app, SLOT(quit()));

    return a.exec();
}
