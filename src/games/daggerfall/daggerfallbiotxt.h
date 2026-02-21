#ifndef DAGGERFALL_BIOTXT_H
#define DAGGERFALL_BIOTXT_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

class DaggerfallBioTxt
{
public:
  enum class DirectiveKind : quint8
  {
    Unknown = 0,
    TextRecordRef,         // #NNNN or !NNNN
    SkillDelta,            // NN +/-NN
    ReputationDemographic, // rN +/-NN
    ReputationFaction,     // rfN +/-NN
    ReactionRoll,          // RR +/-NN
    ResistanceDelta,       // RP|RD|MR +/-NN
    GoldDelta,             // GP +/-NN
    ToHitDelta,            // TH +/-NN
    FatigueDelta,          // FT +/-NN
    ItemGrant,             // IT X Y Z
    GenericCode     // XX ...
  };

  struct Directive
  {
    DirectiveKind kind = DirectiveKind::Unknown;
    QString rawLine;

    // TextRecordRef
    int textRecordId = -1;

    // SkillDelta
    int skillCode = -1;
    int skillDelta = 0;

    // ReputationDemographic / ReputationFaction
    int demographicIndex = -1;
    int factionId = -1;
    int reputationDelta = 0;

    // ReactionRoll
    int reactionRollDelta = 0;

    // ResistanceDelta
    QString resistanceCode;  // RP, RD, MR
    int resistanceDelta = 0;

    // GoldDelta / ToHitDelta / FatigueDelta
    int amountDelta = 0;

    // ItemGrant
    int itemX = -1;
    int itemY = -1;
    int itemZ = -1;

    // GenericCode
    QString code;
    QStringList args;
  };

  struct Option
  {
    QChar label;
    QString text;
    QVector<Directive> directives;
  };

  struct Question
  {
    int number = -1;
    QString text;
    QVector<Option> options;
  };

  struct File
  {
    QVector<Question> questions;
    QString warning;
  };

  static bool loadFile(const QString& filePath, File& outFile, QString* errorMessage = nullptr);
  static bool parseText(const QString& text, File& outFile, QString* errorMessage = nullptr);
};

#endif  // DAGGERFALL_BIOTXT_H
