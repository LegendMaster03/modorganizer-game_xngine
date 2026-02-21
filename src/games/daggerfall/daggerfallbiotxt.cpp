#include "daggerfallbiotxt.h"
#include "daggerfallformatutils.h"

#include <QFile>
#include <QRegularExpression>

namespace {

DaggerfallBioTxt::Directive parseDirective(const QString& line)
{
  DaggerfallBioTxt::Directive d;
  d.rawLine = line;
  const QString t = line.trimmed();

  {
    static const QRegularExpression rx("^[#!]\\s*(\\d+)$");
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::TextRecordRef;
      d.textRecordId = m.captured(1).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^(\\d+)\\s*([+-]\\d+)$");
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::SkillDelta;
      d.skillCode = m.captured(1).toInt();
      d.skillDelta = m.captured(2).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^r([0-7])\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ReputationDemographic;
      d.demographicIndex = m.captured(1).toInt();
      d.reputationDelta = m.captured(2).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^rf(\\d+)\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ReputationFaction;
      d.factionId = m.captured(1).toInt();
      d.reputationDelta = m.captured(2).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^RR\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ReactionRoll;
      d.reactionRollDelta = m.captured(1).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^(RP|RD|MR)\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ResistanceDelta;
      d.resistanceCode = m.captured(1).toUpper();
      d.resistanceDelta = m.captured(2).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^GP\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::GoldDelta;
      d.amountDelta = m.captured(1).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^TH\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ToHitDelta;
      d.amountDelta = m.captured(1).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^FT\\s*([+-]\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::FatigueDelta;
      d.amountDelta = m.captured(1).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^IT\\s+(-?\\d+)\\s+(-?\\d+)\\s+(-?\\d+)$",
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::ItemGrant;
      d.itemX = m.captured(1).toInt();
      d.itemY = m.captured(2).toInt();
      d.itemZ = m.captured(3).toInt();
      return d;
    }
  }

  {
    static const QRegularExpression rx("^([A-Z]{2})\\b(.*)$");
    const QRegularExpressionMatch m = rx.match(t);
    if (m.hasMatch()) {
      d.kind = DaggerfallBioTxt::DirectiveKind::GenericCode;
      d.code = m.captured(1);
      d.args = m.captured(2).trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
      return d;
    }
  }

  d.kind = DaggerfallBioTxt::DirectiveKind::Unknown;
  return d;
}

}  // namespace

bool DaggerfallBioTxt::loadFile(const QString& filePath, File& outFile, QString* errorMessage)
{
  outFile = {};

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly)) {
    return Daggerfall::FormatUtil::setError(errorMessage,
                                            QString("Unable to open biography file: %1").arg(filePath));
  }
  const QString text = QString::fromLatin1(f.readAll());
  return parseText(text, outFile, errorMessage);
}

bool DaggerfallBioTxt::parseText(const QString& text, File& outFile, QString* errorMessage)
{
  outFile = {};
  if (text.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "Biography text is empty");
  }

  // Source docs specify CRLF line endings.
  if (text.contains('\n') && !text.contains("\r\n")) {
    Daggerfall::FormatUtil::appendWarning(outFile.warning,
                                          "File does not appear to use CRLF line endings");
  }

  const QStringList lines = text.split('\n');

  Question* currentQuestion = nullptr;
  Option* currentOption = nullptr;

  const QRegularExpression qRx("^\\s*(\\d+)\\.\\s*(.+?)\\s*$");
  const QRegularExpression oRx("^\\s*([a-zA-Z])\\.\\s*(.+?)\\s*$");

  for (int lineNo = 0; lineNo < lines.size(); ++lineNo) {
    QString line = lines.at(lineNo);
    if (line.endsWith('\r')) {
      line.chop(1);
    }
    if (line.trimmed().isEmpty()) {
      continue;
    }

    {
      const QRegularExpressionMatch m = qRx.match(line);
      if (m.hasMatch()) {
        Question q;
        q.number = m.captured(1).toInt();
        q.text = m.captured(2).trimmed();
        outFile.questions.push_back(q);
        currentQuestion = &outFile.questions.last();
        currentOption = nullptr;
        continue;
      }
    }

    {
      const QRegularExpressionMatch m = oRx.match(line);
      if (m.hasMatch()) {
        if (currentQuestion == nullptr) {
          Daggerfall::FormatUtil::appendWarning(outFile.warning,
                                                QString("Option before question at line %1").arg(lineNo + 1));
          continue;
        }
        Option o;
        o.label = m.captured(1).at(0).toLower();
        o.text = m.captured(2).trimmed();
        currentQuestion->options.push_back(o);
        currentOption = &currentQuestion->options.last();
        continue;
      }
    }

    if (currentOption == nullptr) {
      Daggerfall::FormatUtil::appendWarning(
          outFile.warning, QString("Unattached line at %1: %2").arg(lineNo + 1).arg(line.trimmed()));
      continue;
    }

    currentOption->directives.push_back(parseDirective(line));
  }

  if (outFile.questions.isEmpty()) {
    return Daggerfall::FormatUtil::setError(errorMessage, "No questions parsed from biography text");
  }

  for (const Question& q : outFile.questions) {
    if (q.options.isEmpty()) {
      Daggerfall::FormatUtil::appendWarning(outFile.warning,
                                            QString("Question %1 has no options").arg(q.number));
    }
  }

  return true;
}
