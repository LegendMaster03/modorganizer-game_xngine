#ifndef XNGINETBLFORMAT_H
#define XNGINETBLFORMAT_H

#include <QString>
#include <QVector>
#include <QtGlobal>

class XngineTblFormat
{
public:
  enum class Variant
  {
    Auto,
    ByteLut256
  };

  struct Traits
  {
    Variant variant = Variant::Auto;
    bool strictValidation = true;
  };

  struct ByteLut256
  {
    QVector<quint8> values;  // exactly 256
  };

  struct Document
  {
    Variant variant = Variant::Auto;
    ByteLut256 byteLut256;
    QString warning;
  };

public:
  static bool readFile(const QString& filePath, Document& outDocument,
                       QString* errorMessage = nullptr,
                       const Traits& traits = Traits{});

  static bool writeFile(const QString& filePath, const Document& document,
                        QString* errorMessage = nullptr,
                        const Traits& traits = Traits{});

  static bool parseByteLut256(const QByteArray& bytes, ByteLut256& outTable,
                              QString* errorMessage = nullptr);

  static bool writeByteLut256(const ByteLut256& table, QByteArray& outBytes,
                              QString* errorMessage = nullptr);
};

#endif  // XNGINETBLFORMAT_H
