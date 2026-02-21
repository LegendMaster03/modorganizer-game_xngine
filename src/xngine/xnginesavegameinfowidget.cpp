#include "xnginesavegameinfowidget.h"
#include "ui_xnginesavegameinfowidget.h"

#include "xnginesavegameinfo.h"
#include "xnginesavegame.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QLocale>
#include <QPixmap>
#include <QRegularExpression>
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
  const QString normalizedPath = QDir::fromNativeSeparators(save.getFilepath());
  const QFileInfo saveInfo(normalizedPath);
  QString slotLabel = saveInfo.fileName();
  if (slotLabel.isEmpty()) {
    slotLabel = QDir(normalizedPath).dirName();
  }
  if (slotLabel.isEmpty()) {
    slotLabel = save.getName();
  }
  QString saveNumberLabel = slotLabel;
  const QRegularExpression slotRegex("(?i)^SAVE(\\d+)$");
  const QRegularExpressionMatch slotMatch = slotRegex.match(slotLabel);
  if (slotMatch.hasMatch()) {
    saveNumberLabel = slotMatch.captured(1);
  }
  ui->saveNumLabel->setText(saveNumberLabel);
  ui->characterLabel->setText("");
  ui->locationLabel->setText("");
  ui->levelLabel->setText("");

  QDateTime t = save.getCreationTime().toLocalTime();
  ui->dateLabel->setText(
      QLocale::system().toString(t.date(), QLocale::FormatType::ShortFormat) + " " +
      QLocale::system().toString(t.time()));
  ui->screenshotLabel->setPixmap(QPixmap());
  if (auto xngineSave = dynamic_cast<XngineSaveGame const*>(&save)) {
    ui->characterLabel->setText(xngineSave->getPCName());
    ui->locationLabel->setText(xngineSave->getPCLocation());
    if (xngineSave->getPCLevel() > 0) {
      ui->levelLabel->setText(QString::number(xngineSave->getPCLevel()));
    }

    const QImage screenshot = xngineSave->getScreenshot();
    if (!screenshot.isNull()) {
      const QPixmap pixmap = QPixmap::fromImage(screenshot);
      const QPixmap scaled =
          pixmap.scaled(320, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      ui->screenshotLabel->setPixmap(scaled);
    }

    const bool hasCharacter = !xngineSave->getPCName().trimmed().isEmpty();
    const bool hasLocation = !xngineSave->getPCLocation().trimmed().isEmpty();
    const bool hasLevel = xngineSave->getPCLevel() > 0;
    const bool hasScreenshot = !screenshot.isNull();
    if (!hasCharacter && !hasLocation && !hasLevel && !hasScreenshot) {
      ui->characterLabel->setText(tr("Empty"));
      ui->locationLabel->setText(tr("Empty"));
      ui->levelLabel->setText(tr("Empty"));
      ui->dateLabel->setText(tr("Empty"));
    }
  }
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

  QString detailText = slotLabel;
  if (auto xngineSave = dynamic_cast<XngineSaveGame const*>(&save)) {
    const QString extra = xngineSave->getGameDetails().trimmed();
    if (!extra.isEmpty()) {
      detailText = extra;
    }
  }
  QLabel* detail = new QLabel(detailText);
  detail->setWordWrap(true);
  detail->setIndent(10);
  layout->addWidget(detail);
}
