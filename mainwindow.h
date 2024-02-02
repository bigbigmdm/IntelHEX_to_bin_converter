#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString fileName, lastDirectory;
    QByteArray buf;
    QString bytePrint(unsigned char z);
    uint32_t hexToInt(QString str);

private slots:
    void on_pushButton_open_bin_clicked();
    void on_pushButton_save_hex_clicked();
    void on_pushButton_open_hex_clicked();

    void on_pushButton_save_bin_clicked();

    void on_pushButton_open_cap_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
