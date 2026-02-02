#include "xnginesavegameinfowidget.h"
#include "ui_xnginesavegameinfowidget.h"

#include "xnginesavegameinfo.h"

#include <QDate>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLocale>
#include <QPixmap>
#include <QString>
#include <QStyle>
#include <QTime>
#include <QVBoxLayout>

#include <Qt>
#include <QtGlobal>

#include <memory>

XngineSaveGameInfoWidget::XngineSaveGameInfoWidget(XngineSaveGameInfo const* info,
                                                   QWidget* parent)
    : MOBase::ISaveGameInfoWidget(parent), ui(new Ui::XngineSaveGameInfoWidget),
      m_Info(info)
{
  ui->setupUi(this);
  this->setWindowFlags(Qt::ToolTip | Qt::BypassGraphicsProxyWidget);
  setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) /
                   qreal(255.0));
  ui->gameFrame->setStyleSheet("background-color: transparent;");

  QVBoxLayout* gameLayout = new QVBoxLayout();
  gameLayout->setContentsMargins(0, 0, 0, 0);
  gameLayout->setSpacing(2);
  ui->gameFrame->setLayout(gameLayout);
}

XngineSaveGameInfoWidget::~XngineSaveGameInfoWidget()
{
  delete ui;
}

void XngineSaveGameInfoWidget::setSave(MOBase::ISaveGame const& save)
{
  ui->saveNumLabel->setText(save.getName());
  ui->characterLabel->setText("");
  ui->locationLabel->setText("");
  ui->levelLabel->setText("");

  QDateTime t = save.getCreationTime().toLocalTime();
  ui->dateLabel->setText(
      QLocale::system().toString(t.date(), QLocale::FormatType::ShortFormat) + " " +
      QLocale::system().toString(t.time()));
  ui->screenshotLabel->setPixmap(QPixmap());
  if (ui->gameFrame->layout() != nullptr) {
    QLayoutItem* item = nullptr;
    while ((item = ui->gameFrame->layout()->takeAt(0)) != nullptr) {
      delete item->widget();
      delete item;
    }
    ui->gameFrame->layout()->setSizeConstraint(QLayout::SetFixedSize);
  }

  // Resize box to new content
  this->resize(0, 0);

  QLayout* layout = ui->gameFrame->layout();
  QLabel* header  = new QLabel(tr("Details"));
  QFont headerFont = header->font();
  headerFont.setItalic(true);
  header->setFont(headerFont);
  layout->addWidget(header);

  QLabel* detail = new QLabel(save.getFilepath());
  detail->setIndent(10);
  layout->addWidget(detail);
}
