#ifndef XNGINEBSAFORMAT_H
#define XNGINEBSAFORMAT_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class XngineBSAFormat
{
public:
  enum class IndexType : quint16
  {
    NameRecord = 0x0100,
    NumberRecord = 0x0200
  };

  enum class CompressionMode
  {
    None,
    BattlespireLzss
  };

  struct Traits
  {
    bool allowCompressed = false;
    bool allowCompressedPassthroughWrite = false;
    bool enforceDos83Names = false;
    bool normalizeNameCase = false;
    bool allowMissingTypeHeader = false;
    bool writeTypeHeader = true;
    CompressionMode compressionMode = CompressionMode::None;
  };

  struct Entry
  {
    QString name;
    quint16 recordId = 0;
    qint16 compressed = 0;
    QByteArray data;
  };

  struct Archive
  {
    IndexType type = IndexType::NameRecord;
    QVector<Entry> entries;
  };

  struct FileSpec
  {
    QString archiveName;
    bool indexTypeKnown = false;
    IndexType indexType = IndexType::NameRecord;
    bool optional = false;
    QString usage;
  };

public:
  static bool readArchive(const QString& filePath, Archive& outArchive,
                          QString* errorMessage = nullptr,
                          const Traits& traits = Traits{});

  static bool writeArchive(const QString& filePath, const Archive& archive,
                           QString* errorMessage = nullptr,
                           const Traits& traits = Traits{});

  static bool unpackToDirectory(const QString& filePath, const QString& outputDirectory,
                                QString* errorMessage = nullptr,
                                const Traits& traits = Traits{});

  static bool packFromDirectory(const QString& inputDirectory, const QString& filePath,
                                IndexType type, QString* errorMessage = nullptr,
                                const Traits& traits = Traits{});

  static bool packFromManifestFile(const QString& inputDirectory,
                                   const QString& manifestFilePath,
                                   const QString& filePath, IndexType type,
                                   QString* errorMessage = nullptr,
                                   const Traits& traits = Traits{});
};

#endif  // XNGINEBSAFORMAT_H
