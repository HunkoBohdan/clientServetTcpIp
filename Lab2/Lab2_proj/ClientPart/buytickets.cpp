#include "buytickets.h"
#include "ui_buytickets.h"
#include <UiHelperBuyTickets.cpp>

#define selectBuy "0"
#define selectReserve "1"

BuyTickets::BuyTickets(BackEnd* bckEnd, QWidget* parent) : QWidget(parent), ui(new Ui::BuyTickets)
{
    ui->setupUi(this);
    this->bckEnd = bckEnd;

    connect(bckEnd, SIGNAL(_cList(QStringList)), this, SLOT(aComplete(QStringList)));
    connect(bckEnd, SIGNAL(_trainsList(QVariantList)), this, SLOT(showTrainsList(QVariantList)));
    connect(bckEnd, SIGNAL(_availableSeats(QString, QStringList)), this, SLOT(showAvailableSeats(QString, QStringList)));
    connect(bckEnd, SIGNAL(_ticketPurchaseSuccess()), this, SLOT(ticketPurchaseDone()));
    connect(this, SIGNAL(_dataToSend(QByteArray)), bckEnd, SLOT(sendData(QByteArray)));

    ui->DateEdit->setMinimumDateTime(QDateTime::currentDateTime());

    trainModel = nullptr;
    depCompleter = nullptr;
    destCompleter = nullptr;
    seatsList = nullptr;
    wagonsList = nullptr;
    cityList = nullptr;
    takenSeatslist = nullptr;

    setShadowEff();

    ui->TrainsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->TrainsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->TrainsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->stackedWidget->setCurrentIndex(0);

    ui->TrainsTable->hide();

    this->currentWagon = 1;
}

BuyTickets::~BuyTickets()
{
    delete ui;
    delete trainModel;
    delete bckEnd;
    delete depCompleter;
    delete destCompleter;
    delete cityList;
    delete takenSeatslist;
}

void BuyTickets::on_SearchButton_clicked()
{
    depTxt = ui->DepartureLineEdit->text();
    destTxt = ui->DestinationLineEdit->text();

    if (!cityList->contains(depTxt, Qt::CaseInsensitive))
    {
        bckEnd->showErrorMsg(ui->DepartureLineEdit, ("There is no \"" + depTxt + "\" city in the system"));
    }
    else if (!cityList->contains(destTxt, Qt::CaseInsensitive))
    {
        bckEnd->showErrorMsg(ui->DestinationLineEdit, ("There is no \"" + destTxt + "\" city in the system"));
    }
    else
    {
        this->dateTxt = QString::number(ui->DateEdit->date().year()) + "-" + QString::number(ui->DateEdit->date().month()) + "-" +
                                        QString::number(ui->DateEdit->date().day());

        this->timeTxt = ui->TimeEdit->time().toString();

        if (ui->DateEdit->date() == QDate::currentDate() && ui->TimeEdit->time() < QTime::currentTime())
        {
            bckEnd->showErrorMsg(ui->TimeEdit, "This time already passed");
        }
        else
        {
            QString txtToSend = QString("{\"operation\":\"getTrainsList\", \"dep\":\"%1\", \"dest\":\"%2\", \"arrDate\":\"%3\", \"arrTime\":\"%4\"}")
                                                            .arg(this->depTxt)
                                                            .arg(this->destTxt)
                                                            .arg(this->dateTxt)
                                                            .arg(this->timeTxt);
            emit _dataToSend(txtToSend.toUtf8());
        }
    }
}

void BuyTickets::on_ReverseDepDest_clicked()
{
    QString depTmp = ui->DepartureLineEdit->text();
    ui->DepartureLineEdit->setText(ui->DestinationLineEdit->text());
    ui->DestinationLineEdit->setText(depTmp);
}

void BuyTickets::aComplete(QStringList cList)
{
    delete depCompleter;
    delete destCompleter;
    delete cityList;

    cityList = new QStringList(cList);

    depCompleter = new QCompleter(cList, this);
    destCompleter = new QCompleter(cList, this);

    depCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    destCompleter->setCaseSensitivity(Qt::CaseInsensitive);

    ui->DepartureLineEdit->setCompleter(depCompleter);
    ui->DestinationLineEdit->setCompleter(destCompleter);

    QAbstractItemView* popup = depCompleter->popup();

    setCompleterStyle(popup);
}

void BuyTickets::showTrainsList(QVariantList trainsList)
{
    if (trainsList.length() == 0)
    {
        ui->TrainsTable->hide();
        bckEnd->showErrorMsg(ui->SearchButton, "Sorry but we can't find any trains from " + this->depTxt + " to " + this->destTxt);
        return;
    }

    delete trainModel;

    unsigned short columnCount = 4;
    trainModel = new QStandardItemModel(trainsList.length(), columnCount, this);

    int row = 0;

    for (auto& _train : trainsList)
    {
        QJsonObject train = _train.toJsonObject();

        QModelIndex modelIndex;

        modelIndex = trainModel->index(row, 0);
        trainModel->setData(modelIndex, train.value("trainId").toString(), Qt::DisplayRole);

        modelIndex = trainModel->index(row, 1);
        QString depDateInfo = train.value("depArriveTime").toString() + " " + train.value("depArriveDate").toString() + " - " +
                                                    train.value("dapDepTime").toString() + " " + train.value("dapDepDate").toString();

        trainModel->setData(modelIndex, depDateInfo, Qt::DisplayRole);

        modelIndex = trainModel->index(row, 2);
        QString destArriveTime = "  " + train.value("destArriveTime").toString() + " " + train.value("destArriveDate").toString();

        trainModel->setData(modelIndex, destArriveTime, Qt::DisplayRole);

        modelIndex = trainModel->index(row, 3);
        trainModel->setData(modelIndex, train.value("freeSeats").toString(), Qt::DisplayRole);
        ++row;
        qDebug() << destArriveTime;
    }

    trainModel->setHeaderData(0, Qt::Horizontal, "Train number");
    trainModel->setHeaderData(1, Qt::Horizontal, "Departure info");
    trainModel->setHeaderData(2, Qt::Horizontal, "Destination arrival time");
    trainModel->setHeaderData(3, Qt::Horizontal, "Tickets available");

    ui->TrainsTable->setModel(trainModel);

    ui->TrainsTable->show();
}

void BuyTickets::on_TrainsTable_pressed(const QModelIndex& index)
{
    this->currentWagon = 1;
    QModelIndex tNameIndex = trainModel->index(index.row(), 0);

    this->trainId = trainModel->data(tNameIndex).toString();

    QString txtToSend = QString("{\"operation\":\"getAvailableSeats\", \"trainDate\":\"%1\", \"trainId\":\"%2\", \"dep\":\"%3\", \"dest\":\"%4\"}")
                                                    .arg(dateTxt)
                                                    .arg(trainId)
                                                    .arg(depTxt)
                                                    .arg(destTxt);
    emit _dataToSend(txtToSend.toUtf8());
}

void BuyTickets::on_goToTrainSelect_clicked()
{
    QString txtToSend = QString("{\"operation\":\"getTrainsList\", \"dep\":\"%1\", \"dest\":\"%2\", \"arrDate\":\"%3\", \"arrTime\":\"%4\"}")
                                                    .arg(this->depTxt)
                                                    .arg(this->destTxt)
                                                    .arg(this->dateTxt)
                                                    .arg(this->timeTxt);
    emit _dataToSend(txtToSend.toUtf8());

    ui->stackedWidget->setCurrentIndex(0);
    deleteSeatsAndWagons();
}

void BuyTickets::showAvailableSeats(QString wagonCount, QStringList takenSeats)
{
    delete takenSeatslist;

    takenSeatslist = new QStringList(takenSeats);
    seatsList = new QVector<customButton*>;
    wagonsList = new QVector<customButton*>;

    for (int i = 1; i <= wagonCount.toInt(); ++i)
    {
        customButton* wagon = new customButton(ui->stackedWidget->currentWidget());

        int freeSpaces = countFreeSpaces(QString::number(i));

        wagon->setMinimumSize(50, 50);
        wagon->setText(QString::number(i) + "(" + QString::number(freeSpaces) + ")");
        wagon->number = i;
        wagon->show();
        wagon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(wagon, SIGNAL(cB_clicked(int)), this, SLOT(changeWagon(int)));

        ui->wagonLayout->addWidget(wagon, 0, i);
        wagonsList->push_back(wagon);
    }

    for (int i = 0; i < 56; ++i)
    {
        customButton* seat = new customButton(ui->stackedWidget->currentWidget());

        seat->setMinimumSize(30, 30);
        seat->setText(QString::number(i + 1));
        seat->number = i;
        seat->show();
        seat->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        connect(seat, SIGNAL(cB_clicked(int)), this, SLOT(showTicketPurchaseMenu(int)));

        ui->seatsLayout->addWidget(seat, i % 4, i / 4);
        seatsList->push_back(seat);
    }

    ui->seatsLayout->setVerticalSpacing(20);
    ui->seatsLayout->setHorizontalSpacing(20);

    changeWagon(this->currentWagon);

    ui->stackedWidget->setCurrentIndex(1);
}

void BuyTickets::changeWagon(int wagonNumber)
{
    this->currentWagon = wagonNumber;

    ui->ownerFnameLineEdit->hide();
    ui->ownerLnameLineEdit->hide();
    ui->buyTicketButton->setVisible((0));
    ui->reserveTicketButton->hide();
    ui->seatNumberLabel->hide();

    for (auto& wagon : *wagonsList)
    {
        wagon->setStyleSheet("background-color: Gainsboro;");
        wagon->setEnabled(1);
    }
    (*wagonsList)[wagonNumber - 1]->setStyleSheet("background-color:  SlateGrey;");
    (*wagonsList)[wagonNumber - 1]->setEnabled(0);

    showSeatsForWagon(wagonNumber);
}

void BuyTickets::showTicketPurchaseMenu(int seatNumber)
{
    this->currentSeat = seatNumber;

    ui->ownerFnameLineEdit->show();
    ui->ownerLnameLineEdit->show();
    ui->buyTicketButton->show();
    ui->reserveTicketButton->show();
    ui->seatNumberLabel->show();

    ui->seatNumberLabel->setText("Seat number: " + QString::number(seatNumber + 1) + "\nprice 250 uah");

    showSeatsForWagon(currentWagon);

    seatsList->at(seatNumber)->setStyleSheet("background-color: #F7F9F9; color: #1b2327;");
}

void BuyTickets::deleteSeatsAndWagons()
{
    for (auto wagon : *wagonsList)
    {
        ui->seatsLayout->removeWidget(wagon);
        delete wagon;
    }
    for (auto seat : *seatsList)
    {
        ui->seatsLayout->removeWidget(seat);
        delete seat;
    }
    delete seatsList;
    delete wagonsList;
}

void BuyTickets::showSeatsForWagon(int wagonNumber)
{
    for (auto& seat : *seatsList)
    {
        seat->setEnabled(1);
        seat->setStyleSheet("background-color: green;");
    }

    for (int i = 0; i < takenSeatslist->length() / 2; ++i)
    {
        if ((*takenSeatslist)[i * 2].toInt() == wagonNumber)
        {
            (*seatsList)[(*takenSeatslist)[i * 2 + 1].toInt() - 1]->setEnabled(0);
            (*seatsList)[(*takenSeatslist)[i * 2 + 1].toInt() - 1]->setStyleSheet("background-color: red;");
        }
    }
}

int BuyTickets::countFreeSpaces(QString wagonNumber)
{
    int freeSpaces = 56;
    for (int i = 0; i < takenSeatslist->length(); i += 2)
    {
        if ((*takenSeatslist)[i] == wagonNumber)
        {
            --freeSpaces;
        }
    }
    return freeSpaces;
}

void BuyTickets::buyOrReserveTicket(QString buyOrReserve)
{
    QString fName = ui->ownerFnameLineEdit->text();
    QString lName = ui->ownerLnameLineEdit->text();

    QRegularExpression rgx("[\\p{Cyrillic}[a-zA-z]{1,50}");

    if (fName.length() == 0)
    {
        bckEnd->showErrorMsg(ui->ownerFnameLineEdit, "First name can't be empty");
        return;
    }
    if (lName.length() == 0)
    {
        bckEnd->showErrorMsg(ui->ownerLnameLineEdit, "Last name can't be empty");
        return;
    }
    if (rgx.match(fName).capturedLength() != fName.length())
    {
        bckEnd->showErrorMsg(ui->ownerFnameLineEdit, "first name can only contain latin and cyrylic characters");
        return;
    }
    if (rgx.match(lName).capturedLength() != lName.length())
    {
        bckEnd->showErrorMsg(ui->ownerLnameLineEdit, "last name can only contain latin and cyrylic characters");
        return;
    }

    QString txtToSend = QString("{\"operation\":\"buyTicket\", \"trainDate\":\"%1\", \"trainId\":\"%2\", \"wagonNumber\":\"%3\", "
                                                            "\"placeNumber\":\"%4\", \"dep\":\"%5\", \"dest\":\"%6\", \"buyOrReserve\":\"%7\", \"ownerInfo\":\"%8\", "
                                                            "\"fName\":\"%9\", "
                                                            "\"lName\":\"%10\"}")
                                                    .arg(this->dateTxt)
                                                    .arg(this->trainId)
                                                    .arg(this->currentWagon)
                                                    .arg(this->currentSeat + 1)
                                                    .arg(this->depTxt)
                                                    .arg(this->destTxt)
                                                    .arg(buyOrReserve)
                                                    .arg(bckEnd->getCurUserame())
                                                    .arg(fName)
                                                    .arg(lName);

    emit _dataToSend(txtToSend.toUtf8());
}

void BuyTickets::on_buyTicketButton_clicked()
{
    buyOrReserveTicket(selectBuy);
}

void BuyTickets::on_reserveTicketButton_clicked()
{
    // 0 stands for by ticket 1 is reserve
    buyOrReserveTicket(selectReserve);
}

void BuyTickets::ticketPurchaseDone()
{
    deleteSeatsAndWagons();
    QString txtToSend = QString("{\"operation\":\"getAvailableSeats\", \"trainDate\":\"%1\", \"trainId\":\"%2\", \"dep\":\"%3\", \"dest\":\"%4\"}")
                                                    .arg(dateTxt)
                                                    .arg(trainId)
                                                    .arg(depTxt)
                                                    .arg(destTxt);

    emit _dataToSend(txtToSend.toUtf8());

    bckEnd->showErrorMsg(ui->ownerFnameLineEdit, "Operation done succesfully");
}
