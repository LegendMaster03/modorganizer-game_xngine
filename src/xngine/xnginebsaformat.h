#ifndef XNGINEBSAFORMAT_H
#define XNGINEBSAFORMAT_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

class XngineBSAFormat
{
public:
  // These index tags select the shared DOS-era XnGine container footer
  // format; they do not describe the record's game-specific payload.
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

  enum class DescriptorLayout
  {
    Auto,
    Daggerfall,
    Battlespire
  };

  enum class ArchiveVariant
  {
    Standard,
    Snd,
    Sav
  };

  // Per-game capabilities. Passthrough flags authorize handling already
  // compressed bytes; they do not provide a compression encoder.
  struct Traits
  {
    bool allowCompressed = false;
    bool allowCompressedPassthroughRead = false;
    bool allowCompressedPassthroughWrite = false;
    bool enforceDos83Names = false;
    bool normalizeNameCase = false;
    // If the interpreted type tag is unknown, permit a count-only header
    // and the NameRecord fallback. This is not general format autodetection.
    bool allowMissingTypeHeader = false;
    bool writeTypeHeader = true;
    CompressionMode compressionMode = CompressionMode::None;
    DescriptorLayout descriptorLayout = DescriptorLayout::Auto;
    ArchiveVariant variantHint = ArchiveVariant::Standard;
  };

  struct Entry
  {
    QString name;
    quint32 recordId = 0;
    qint16 compressed = 0;
    QByteArray data;
  };

  struct Archive
  {
    IndexType type = IndexType::NameRecord;
    ArchiveVariant variant = ArchiveVariant::Standard;
    QVector<Entry> entries;
  };

  // Known archive filename and optional index mapping. Recognition by name
  // is weaker than knowing how to pack it: indexTypeKnown may be false.
  struct FileSpec
  {
    QString archiveName;
    bool indexTypeKnown = false;
    IndexType indexType = IndexType::NameRecord;
    bool optional = false;
    QString usage;
    ArchiveVariant archiveVariant = ArchiveVariant::Standard;
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

  // Uses manifest order and record metadata. It does not recompress bytes
  // when a manifest marks an entry as compressed.
  static bool packFromManifestFile(const QString& inputDirectory,
                                   const QString& manifestFilePath,
                                   const QString& filePath, IndexType type,
                                   QString* errorMessage = nullptr,
                                   const Traits& traits = Traits{});

  static ArchiveVariant detectArchiveVariant(const QString& filePath, const Archive& archive);

};

#endif  // XNGINEBSAFORMAT_H
