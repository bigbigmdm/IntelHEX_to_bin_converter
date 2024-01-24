#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBar->setValue(0);
    ui->lineEdit_chipsize->setText("32768");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_open_bin_clicked()
{
     ui->statusBar->showMessage(tr("Opening file"));
     fileName = QFileDialog::getOpenFileName(this,
                                 QString(tr("Open file")),
                                 lastDirectory,
                                 "Data Images (*.bin *.BIN);;All files (*.*)");
     QFileInfo info(fileName);
     ui->statusBar->showMessage(tr("Current file: ") + info.fileName());
     lastDirectory = info.filePath();
     QFile file(fileName);

     if (!file.open(QIODevice::ReadOnly))
     {

         return;
     }
     buf.resize(int(info.size()));
     buf = file.readAll();
     file.close();
     fileName.clear();
}


QString MainWindow::bytePrint(unsigned char z)
{
    unsigned char s;
    s = z / 16;
    if (s > 0x9) s = s + 0x37;
    else s = s + 0x30;
    z = z % 16;
    if (z > 0x9) z = z + 0x37;
    else z = z + 0x30;
    return QString(s) + QString(z);
}

uint32_t MainWindow::hexToInt(QString str)
{
    unsigned char c;
    uint32_t len = static_cast<uint32_t>(str.length());
    QByteArray bstr = str.toLocal8Bit();
    if ((len > 0) && (len < 8))
    {
        uint32_t i, j = 1;
        uint32_t  addr = 0;
        for (i = len; i >0; i--)
        {
           c = static_cast<unsigned char>(bstr[i-1]);
           if ((c >= 0x30) && (c <=0x39)) addr =  addr + (c - 0x30) * j;
           if ((c >= 0x41) && (c <= 0x46)) addr = addr + (c - 0x37) * j;
           if ((c >= 0x61) && (c <= 0x66)) addr = addr + (c - 0x57) * j;
        j = j * 16;
        }
        return addr;
    }
    else return 0;
}

void MainWindow::on_pushButton_save_hex_clicked()
{
    int addr = 0, hi_addr =0;
     QString result = "";
     int currSize = buf.size();
     uint8_t i, counter = 0;
     int ostatok = 0;
     ui->progressBar->setRange(0,buf.size());
     lastDirectory.replace(".bin", ".hex");
     ui->statusBar->showMessage(tr("Saving file"));
     fileName = QFileDialog::getSaveFileName(this,
                                 QString(tr("Save file")),
                                 lastDirectory,
                                 "Intel HEX Images (*.hex *.HEX);;All files (*.*)");
     QFileInfo info(fileName);
     ui->statusBar->showMessage(tr("Current file: ") + info.fileName());
     lastDirectory = info.filePath();
     QFile file(fileName);
     QTextStream stream(&file);
     if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
     {

         return;
     }
     stream.seek(file.size());
     while ((addr + hi_addr * 0x10000) < (buf.size()))
     {

         if (addr < currSize - 0x20) ostatok = 0x20;
         else  ostatok =currSize - addr;

            result.append(":");
            result.append(bytePrint(static_cast<unsigned char>(ostatok)));              //number of bytes per string
            result.append(bytePrint(static_cast<unsigned char>((addr & 0xff00) >> 8))); //one address byte
            result.append(bytePrint(static_cast<unsigned char>(addr & 0xff)));          //zero address byte
            result.append("00");                                                        //type 00
            counter = counter + static_cast<unsigned char>(ostatok) + uint8_t((addr & 0xff00) >> 8) + uint8_t(addr & 0xff) ;
            for (i = 0; i < ostatok; i++)
            {
                result.append(bytePrint(static_cast<unsigned char>((buf[hi_addr * 256 * 256 + addr + i]))));
                counter = counter + static_cast<uint8_t>((buf[hi_addr * 256 * 256 + addr + i]));
            }
         result.append(bytePrint(0xff - counter + 1));
         counter = 0;
         stream << result << "\n";
         result.clear();

         addr = addr + 0x20;
         if (addr >= 0x10000)                                                            //command 04 - setting the high address
         {
             hi_addr ++;
             addr = addr - 0x10000;
             result.append(":02000004");
             result.append(bytePrint(uint8_t((hi_addr & 0xff00) >> 8)));
             result.append(bytePrint(uint8_t(hi_addr & 0xff)));
             counter = 0x06 + uint8_t((hi_addr & 0xff00) >> 8) + uint8_t(hi_addr & 0xff);
             result.append(bytePrint(0xff - counter + 1));
             counter = 0;
             stream << result << "\n";
             result.clear();
             ui->progressBar->setValue(hi_addr * 256 * 256);
         }
     }
     result = ":00000001FF\n";                                                          //end string
     stream << result;
     file.close();
     fileName.clear();
     ui->progressBar->setValue(0);
}

void MainWindow::on_pushButton_open_hex_clicked()
{
 int chipSize = ui->lineEdit_chipsize->text().toInt();
 uint_fast32_t lineLen, lo_addr, hi_addr, command, i;
 unsigned char currByte;
 uint8_t counter, checkSUM;
 QString currStr ="", strVal = "";
 buf.resize(chipSize);
 buf.fill(char(0xff));
 ui->statusBar->showMessage(tr("Opening file"));
 fileName = QFileDialog::getOpenFileName(this,
                             QString(tr("Open file")),
                             lastDirectory,
                             "Intel HEX Images (*.hex *.HEX);;All files (*.*)");
 QFileInfo info(fileName);
 ui->statusBar->showMessage(tr("Current file: ") + info.fileName());
 lastDirectory = info.filePath();
 QFile file(fileName);

 if (!file.open(QIODevice::ReadOnly))
 {

     return;
 }
 hi_addr = 0;
 ui->progressBar->setRange(0, chipSize);
 while (!file.atEnd())
 {
     currStr = file.readLine();
     counter = 0;
     //qDebug() << currStr;
     //parsing string
     if (currStr[0] != ':')
     {
         QMessageBox::about(this, tr("Error"), tr("Not valid HEX format!"));
         return;
     }
     strVal = currStr.mid(1,2); //Length of data in current string
     lineLen = hexToInt(strVal);
     counter = counter + static_cast<unsigned char>(lineLen);

     strVal.clear();            //low address
     strVal = currStr.mid(3,4);
     lo_addr = hexToInt(strVal);
     counter = counter + static_cast<unsigned char>((lo_addr) >> 8) + static_cast<unsigned char>(lo_addr & 0x00ff);

     if (hi_addr * 256 * 256 + lo_addr  > static_cast<unsigned long>(chipSize))
     {
         QMessageBox::about(this, tr("Error"), tr("The address is larger than the size of the chip!"));
         qDebug() << "ChipSize=" << chipSize << " Address=" << hi_addr * 256 * 256 + lo_addr;
         return;
     }

     strVal.clear();            //command
     strVal = currStr.mid(7,2);
     command = hexToInt(strVal);
     counter = counter + static_cast<unsigned char>(command);

     //qDebug() << "bytes=" << lineLen << "lo_addr=" << lo_addr << "command=" << command;

     if (command == 0) //reading bytes from current string
     {
         for (i = 0; i < lineLen; i++)
         {

             strVal.clear();            //get current byte of string
             strVal = currStr.mid(int(i) * 2 + 9, 2);
             currByte = static_cast<unsigned char>(hexToInt(strVal));
             //qDebug() << currByte;
             buf.data()[hi_addr * 256 * 256 + lo_addr + i] = char(currByte);
             counter = counter + static_cast<unsigned char>(hexToInt(strVal));

         }
             counter = 255 - counter + 1;
             checkSUM = static_cast<unsigned char>(hexToInt( currStr.mid(int(i) * 2 + 9, 2)));
             //qDebug() << "counter=" << counter << " checksum=" << checkSUM;

             if (counter != checkSUM)
             {
                 QMessageBox::about(this, tr("Error"), tr("Checksum error!"));
                 return;
             }
     }
     if (command == 4) //Changing the high address
     {
         strVal.clear();            //low address
         strVal = currStr.mid(9,4);
         hi_addr = hexToInt(strVal);
         counter = counter + static_cast<unsigned char>((hi_addr) >> 8) + static_cast<unsigned char>(hi_addr & 0x00ff);
         counter = 255 - counter + 1;
         checkSUM = static_cast<unsigned char>(hexToInt( currStr.mid(13, 2)));
         ui->progressBar->setValue(int(hi_addr * 256 * 256));
         if (counter != checkSUM)
         {
             QMessageBox::about(this, tr("Error"), tr("Checksum error!"));
             return;
         }
         qDebug() << hi_addr;
     }


 }
 qDebug() << "ChipSize=" << chipSize << " Address=" << hi_addr * 256 * 256 + lo_addr;
 file.close();
 fileName.clear();
}

void MainWindow::on_pushButton_save_bin_clicked()
{
    lastDirectory.replace(".hex", ".bin");
    ui->statusBar->showMessage(tr("Saving file"));
    fileName = QFileDialog::getSaveFileName(this,
                                QString(tr("Save file")),
                                lastDirectory,
                                "Binary Images (*.bin *.BIN);;All files (*.*)");
    QFileInfo info(fileName);
    ui->statusBar->showMessage(tr("Current file: ") + info.fileName());
    lastDirectory = info.filePath();
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::about(this, tr("Error"), tr("Error saving file!"));
            return;
        }
    file.write(buf);
    file.close();

}
