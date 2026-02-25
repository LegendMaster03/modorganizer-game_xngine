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
  auto setLabelTextPreserveFont = [](QLabel* label, const QString& text) {
    if (!Qt::mightBeRichText(text)) {
      label->setText(text);
      return;
    }

    const QFont font = label->font();
    QStringList styles;
    const QString family = font.family().toHtmlEscaped();
    if (!family.isEmpty()) {
      styles.push_back(QString("font-family:'%1'").arg(family));
    }
    if (font.pixelSize() > 0) {
      styles.push_back(QString("font-size:%1px").arg(font.pixelSize()));
    } else if (font.pointSizeF() > 0.0) {
      styles.push_back(QString("font-size:%1pt").arg(font.pointSizeF(), 0, 'f', 1));
    }
    styles.push_back(QString("font-weight:%1").arg(font.weight()));
    if (font.italic()) {
      styles.push_back("font-style:italic");
    }

    label->setText(QString("<span style=\"%1\">%2</span>")
                       .arg(styles.join("; "), text));
  };

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
  const QRegularExpression slotRegexClassic("(?i)^SAVE(\\d+)$");
  const QRegularExpression slotRegexRedguard("(?i)^SAVEGAME\\.(\\d+)$");
  QRegularExpressionMatch slotMatch = slotRegexClassic.match(slotLabel);
  if (!slotMatch.hasMatch()) {
    slotMatch = slotRegexRedguard.match(slotLabel);
  }
  if (slotMatch.hasMatch()) {
    bool ok = false;
    const int slot = slotMatch.captured(1).toInt(&ok);
    if (ok) {
      saveNumberLabel = QString::number(slot);
    } else {
      saveNumberLabel = slotMatch.captured(1);
    }
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
    setLabelTextPreserveFont(ui->locationLabel, xngineSave->getPCLocation());
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
  QString detailText;
  if (auto xngineSave = dynamic_cast<XngineSaveGame const*>(&save)) {
    const QString extra = xngineSave->getGameDetails().trimmed();
    if (!extra.isEmpty()) {
      detailText = extra;
    }
  }
  if (!detailText.isEmpty()) {
    QLabel* header  = new QLabel(tr("Details"));
    QFont headerFont = header->font();
    headerFont.setItalic(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    QLabel* detail = new QLabel(detailText);
    detail->setWordWrap(true);
    detail->setIndent(10);
    layout->addWidget(detail);
  }
}
