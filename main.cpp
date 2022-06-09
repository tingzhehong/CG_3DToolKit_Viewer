#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
	
	if (argc > 1)
	{
		QStringList filenames;
        int i = 1;
        while (i < argc)
        {
            QString argument = QString(argv[i++]);
            filenames << argument;
        }
		if (!filenames.empty())
        {
			w.addToDB(filenames);
		}
	}

    int result = a.exec();
	FileIOFilter::UnregisterAll();
    return result;
}
