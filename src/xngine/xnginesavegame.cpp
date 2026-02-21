#include "xnginesavegame.h"

#include "iplugingame.h"
#include "log.h"
#include "scriptextender.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedArrayPointer>
#include <QTime>

#include <Windows.h>
#include <lz4.h>
#include <zlib.h>

#include <stdexcept>
#include <vector>

#include "gamexngine.h"
#include "imoinfo.h"

#define CHUNK 16384

XngineSaveGame::XngineSaveGame(QString const& file, GameXngine const* game,
                               bool const lightEnabled, bool const mediumEnabled)
    : m_FileName(file), m_CreationTime(QFileInfo(file).lastModified()), m_Game(game),
      m_MediumEnabled(mediumEnabled), m_LightEnabled(lightEnabled),
      m_DataFields([this]() {
        return fetchDataFields();
      })
{
  QString normalizedPath = QDir::fromNativeSeparators(file);
  while (normalizedPath.endsWith('/')) {
    normalizedPath.chop(1);
  }

  QFileInfo info(normalizedPath);
  if (info.isDir()) {
    m_DisplayName = info.fileName();
    if (m_DisplayName.isEmpty()) {
      m_DisplayName = QDir(normalizedPath).dirName();
    }
    QDir saveDir(normalizedPath);
    const auto files = saveDir.entryInfoList(QDir::Files, QDir::Time);
    if (!files.isEmpty()) {
      m_CreationTime = files.front().lastModified();
    }
  } else {
    const QString baseName = info.completeBaseName();
    if (baseName.compare("SAVEGAME", Qt::CaseInsensitive) == 0) {
      const QString slotName = info.dir().dirName();
      m_DisplayName = slotName.isEmpty() ? info.fileName() : slotName;
    } else {
      m_DisplayName = baseName.isEmpty() ? info.fileName() : baseName;
    }
  }

  if (m_DisplayName.isEmpty()) {
    m_DisplayName = QFileInfo(normalizedPath).fileName();
  }
}

XngineSaveGame::~XngineSaveGame() {}

QString XngineSaveGame::getFilepath() const
{
  return m_FileName;
}

QDateTime XngineSaveGame::getCreationTime() const
{
  return m_CreationTime;
}

QString XngineSaveGame::getName() const
{
  if (m_PCName.isEmpty()) {
    return m_DisplayName;
  }

  QStringList parts;
  parts << m_PCName;
  parts << QString("#%1").arg(m_SaveNumber);
  if (m_PCLevel > 0) {
    parts << QObject::tr("Level %1").arg(m_PCLevel);
  }
  if (!m_PCLocation.isEmpty()) {
    parts << m_PCLocation;
  }

  return parts.join(", ");
}

QString XngineSaveGame::getSaveGroupIdentifier() const
{
  return m_PCName;
}

QStringList XngineSaveGame::allFiles() const
{
  QFileInfo saveInfo(m_FileName);
  if (saveInfo.isDir()) {
    QStringList files;
    const auto entries = QDir(m_FileName).entryInfoList(QDir::Files, QDir::Name);
    for (const auto& entry : entries) {
      files.push_back(entry.absoluteFilePath());
    }
    return files;
  }

  // This returns all valid files associated with this game
  QStringList res = {m_FileName};
  auto e = m_Game->m_Organizer->gameFeatures()->gameFeature<MOBase::ScriptExtender>();
  if (e != nullptr) {
    QFileInfo file(m_FileName);
    QFileInfo SEfile(file.absolutePath() + "/" + file.completeBaseName() + "." +
                     m_Game->savegameSEExtension());
    if (SEfile.exists()) {
      res.push_back(SEfile.absoluteFilePath());
    }
  }
  return res;
}

bool XngineSaveGame::hasScriptExtenderFile() const
{
  QFileInfo file(m_FileName);
  QFileInfo SEfile(file.absolutePath() + "/" + file.completeBaseName() + "." +
                   m_Game->savegameSEExtension());
  return SEfile.exists();
}

void XngineSaveGame::setCreationTime(_SYSTEMTIME const& ctime)
{
  QDate date;
  date.setDate(ctime.wYear, ctime.wMonth, ctime.wDay);
  QTime time;
  time.setHMS(ctime.wHour, ctime.wMinute, ctime.wSecond, ctime.wMilliseconds);

  m_CreationTime = QDateTime(date, time, Qt::UTC);
}

XngineSaveGame::FileWrapper::FileWrapper(QString const& filepath,
                                         QString const& expected)
    : m_File(filepath), m_HasFieldMarkers(false),
      m_PluginString(StringType::TYPE_WSTRING),
      m_PluginStringFormat(StringFormat::UTF8), m_NextChunk(0)
{
  if (!m_File.open(QIODevice::ReadOnly)) {
    throw std::runtime_error(
        QObject::tr("failed to open %1").arg(filepath).toUtf8().constData());
  }

  std::vector<char> fileID(expected.length() + 1);
  m_File.read(fileID.data(), expected.length());
  fileID[expected.length()] = '\0';

  QString id(fileID.data());
  if (expected != id) {
    throw std::runtime_error(
        QObject::tr("wrong file format - expected %1 got \'%2\' for %3")
            .arg(expected)
            .arg(id)
            .arg(filepath)
            .toUtf8()
            .constData());
  }
}

void XngineSaveGame::FileWrapper::setHasFieldMarkers(bool state)
{
  m_HasFieldMarkers = state;
}

void XngineSaveGame::FileWrapper::setPluginString(StringType type)
{
  m_PluginString = type;
}

void XngineSaveGame::FileWrapper::setPluginStringFormat(StringFormat type)
{
  m_PluginStringFormat = type;
}

void XngineSaveGame::FileWrapper::readQDataStream(QDataStream& data, void* buff,
                                                  std::size_t length)
{
  int read    = data.readRawData(static_cast<char*>(buff), static_cast<int>(length));
  bool result = true;
  if (read != length && m_CompressionType == 1) {
    bool result = readNextChunk();
    if (result) {
      read += data.readRawData(static_cast<char*>(buff) + read,
                               static_cast<int>(length - read));
    }
  }
  if (read != length || !result) {
    throw std::runtime_error("unexpected end of file");
  }
}

template <typename T>
void XngineSaveGame::FileWrapper::readQDataStream(QDataStream& data, T& value)
{
  static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);
  readQDataStream(data, &value, sizeof(T));
}

void XngineSaveGame::FileWrapper::skipQDataStream(QDataStream& data,
                                                  std::size_t length)
{
  int skip    = data.skipRawData(static_cast<int>(length));
  bool result = true;
  if (skip != length && m_CompressionType == 1) {
    result = readNextChunk();
    if (result) {
      skip += data.skipRawData(static_cast<int>(length - skip));
    }
  }
  if (skip != length || !result) {
    throw std::runtime_error("unexpected end of file");
  }
}

template <>
void XngineSaveGame::FileWrapper::read<QString>(QString& value)
{
  if (m_CompressionType == 0) {
    unsigned short length;
    if (m_PluginString == StringType::TYPE_BSTRING ||
        m_PluginString == StringType::TYPE_BZSTRING) {
      unsigned char len;
      read(len);
      length = m_PluginString == StringType::TYPE_BZSTRING ? len + 1 : len;
    } else {
      read(length);
    }

    if (m_HasFieldMarkers) {
      skip<char>();
    }

    QByteArray buffer;
    buffer.resize(length);

    read(buffer.data(),
         m_PluginString == StringType::TYPE_BZSTRING ? length - 1 : length);

    if (m_PluginString == StringType::TYPE_BZSTRING)
      buffer[length - 1] = '\0';

    if (m_HasFieldMarkers) {
      skip<char>();
    }

    if (m_PluginStringFormat == StringFormat::UTF8)
      value = QString::fromUtf8(buffer.constData());
    else
      value = QString::fromLocal8Bit(buffer.constData());
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    unsigned short length;
    if (m_PluginString == StringType::TYPE_BSTRING ||
        m_PluginString == StringType::TYPE_BZSTRING) {
      unsigned char len;
      readQDataStream(*m_Data, len);
      length = m_PluginString == StringType::TYPE_BZSTRING ? len + 1 : len;
    } else {
      readQDataStream(*m_Data, length);
    }

    if (m_HasFieldMarkers) {
      skip<char>();
    }

    QByteArray buffer;
    buffer.resize(length);

    readQDataStream(*m_Data, buffer.data(),
                    m_PluginString == StringType::TYPE_BZSTRING ? length - 1 : length);

    if (m_PluginString == StringType::TYPE_BZSTRING)
      buffer[length - 1] = '\0';

    if (m_HasFieldMarkers) {
      m_Data->skipRawData(1);
    }

    if (m_PluginStringFormat == StringFormat::UTF8)
      value = QString::fromUtf8(buffer.constData());
    else
      value = QString::fromLocal8Bit(buffer.constData());
  } else {
    // TODO: Disabled due to uibase.lib not exporting ireplace_all function
    // // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown "
    //                   "Compressed\" with your savefile attached");
  }
}

void XngineSaveGame::FileWrapper::read(void* buff, std::size_t length)
{
  int read = m_File.read(static_cast<char*>(buff), length);
  if (read != length) {
    throw std::runtime_error("unexpected end of file");
  }
}

QImage XngineSaveGame::FileWrapper::readImage(int scale, bool alpha)
{
  unsigned long width;
  read(width);
  unsigned long height;
  read(height);
  return readImage(width, height, scale, alpha);
}

QImage XngineSaveGame::FileWrapper::readImage(unsigned long width,
                                              unsigned long height, int scale,
                                              bool alpha)
{
  int bpp = alpha ? 4 : 3;
  QScopedArrayPointer<unsigned char> buffer(new unsigned char[width * height * bpp]);
  read(buffer.data(), width * height * bpp);
  QImage image(buffer.data(), width, height,
               alpha ? QImage::Format_RGBA8888_Premultiplied : QImage::Format_RGB888);

  // We need to copy the image here because QImage does not make a copy of the
  // buffer when constructed.
  if (scale != 0) {
    return image.copy().scaledToWidth(scale);
  } else {
    return image.copy();
  }
}

void XngineSaveGame::FileWrapper::setCompressionType(uint16_t compressionType)
{
  m_CompressionType = compressionType;
}

void XngineSaveGame::FileWrapper::closeCompressedData()
{
  if (m_CompressionType == 0) {
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    m_NextChunk        = 0;
    m_UncompressedSize = 0;
    m_Data->device()->close();
    delete m_Data;
  } else {
    // Disabled due to uibase.lib not exporting ireplace_all function
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown "
    //                   "Compressed\" with your savefile attached");
  }
}

bool XngineSaveGame::FileWrapper::openCompressedData(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    return false;
  } else if (m_CompressionType == 1) {
    read(m_NextChunk);
    read(m_UncompressedSize);
    QByteArray placeholder;
    m_Data      = new QDataStream(placeholder);
    bool result = readNextChunk();
    if (result)
      skipQDataStream(*m_Data, bytesToIgnore);
    return result;
  } else if (m_CompressionType == 2) {
    uint32_t uncompressedSize;
    read(uncompressedSize);
    uint32_t compressedSize;
    read(compressedSize);
    QByteArray compressed;
    compressed.resize(compressedSize);
    read(compressed.data(), compressedSize);
    QByteArray decompressed;
    decompressed.resize(uncompressedSize);
    LZ4_decompress_safe_partial(compressed.data(), decompressed.data(), compressedSize,
                                uncompressedSize, uncompressedSize);
    compressed.clear();

    m_Data = new QDataStream(decompressed);
    skipQDataStream(*m_Data, bytesToIgnore);

    return true;
  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown "
    //                   "Compressed\" with your savefile attached");
    return false;
  }
}

bool XngineSaveGame::FileWrapper::readNextChunk()
{
  uint32_t have;
  uint64_t read                     = 0;
  std::unique_ptr<char[]> inBuffer  = std::make_unique<char[]>(CHUNK);
  std::unique_ptr<char[]> outBuffer = std::make_unique<char[]>(CHUNK);
  QByteArray finalData;
  m_Data->device()->close();
  delete m_Data;
  z_stream stream{};
  try {
    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = 0;
    stream.next_in  = Z_NULL;
    if (m_NextChunk >= m_File.size() || finalData.size() == m_UncompressedSize)
      return false;
    m_File.seek(m_NextChunk);
    int zlibRet = inflateInit2(&stream, 15 + 32);
    if (zlibRet != Z_OK) {
      return false;
    }
    do {
      stream.avail_in = m_File.read(inBuffer.get(), CHUNK);
      read += stream.avail_in;
      if (!m_File.isReadable()) {
        (void)inflateEnd(&stream);
        return false;
      }
      if (stream.avail_in == 0)
        break;
      stream.next_in = reinterpret_cast<Bytef*>(inBuffer.get());
      do {
        stream.avail_out = CHUNK;
        stream.next_out  = reinterpret_cast<Bytef*>(outBuffer.get());
        zlibRet          = inflate(&stream, Z_NO_FLUSH);
        if ((zlibRet != Z_OK) && (zlibRet != Z_STREAM_END) &&
            (zlibRet != Z_BUF_ERROR)) {
          return false;
        }
        have = CHUNK - stream.avail_out;
        finalData += QByteArray::fromRawData(outBuffer.get(), have);
      } while (stream.avail_out == 0);
      read -= stream.avail_in;
    } while (zlibRet != Z_STREAM_END);
    inflateEnd(&stream);
    uint64_t remainder = (m_NextChunk + read) % 16;
    uint64_t next      = m_NextChunk + read + 16 - (remainder == 0 ? 16 : remainder);
    m_NextChunk        = next;
  } catch (const std::exception&) {
    inflateEnd(&stream);
    return false;
  }
  m_Data = new QDataStream(finalData);
  return true;
}

uint8_t XngineSaveGame::FileWrapper::readChar(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint8_t version;
    read(version);
    return version;
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    // decompression already done by readSaveGameVersion
    skipQDataStream(*m_Data, bytesToIgnore);

    uint8_t version;
    readQDataStream(*m_Data, version);
    return version;

  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown "
    //                   "Stats\" with your savefile attached");
    return 0;
  }
}

uint16_t XngineSaveGame::FileWrapper::readShort(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint16_t size;
    read(size);
    return size;
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    // decompression already done by readSaveGameVersion
    skipQDataStream(*m_Data, bytesToIgnore);

    uint16_t size;
    readQDataStream(*m_Data, size);
    return size;
  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown `Compressed with your savefile attached");
    return 0;
  }
}

uint32_t XngineSaveGame::FileWrapper::readInt(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint32_t size;
    read(size);
    return size;
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    // decompression already done by readSaveGameVersion
    skipQDataStream(*m_Data, bytesToIgnore);

    uint32_t size;
    readQDataStream(*m_Data, size);
    return size;
  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown `Compressed with your savefile attached");
    return 0;
  }
}

uint64_t XngineSaveGame::FileWrapper::readLong(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint64_t size;
    read(size);
    return size;
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    // decompression already done by readSaveGameVersion
    skipQDataStream(*m_Data, bytesToIgnore);

    uint64_t size;
    readQDataStream(*m_Data, size);
    return size;
  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown `Compressed with your savefile attached");
    return 0;
  }
}

float_t XngineSaveGame::FileWrapper::readFloat(int bytesToIgnore)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    float_t value;
    read(value);
    return value;
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    // decompression already done by readSaveGameVersion
    skipQDataStream(*m_Data, bytesToIgnore);

    float_t value;
    readQDataStream(*m_Data, value);
    return value;
  } else {
    // MOBase::log::warn("Please create an issue on the MO github labeled \"Found unknown `Compressed with your savefile attached");
    return 0;
  }
}

QStringList XngineSaveGame::FileWrapper::readPlugins(int bytesToIgnore, int extraData,
                                                       const QStringList& corePlugins)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint8_t count;
    read(count);
    return readPluginData(count, extraData, corePlugins);
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    skipQDataStream(*m_Data, bytesToIgnore);
    uint8_t count;
    readQDataStream(*m_Data, count);
    return readPluginData(count, extraData, corePlugins);
  }
  return {};
}

QStringList
XngineSaveGame::FileWrapper::readLightPlugins(int bytesToIgnore, int extraData,
                                                const QStringList& corePlugins)
{
  if (m_CompressionType == 0) {
    if (bytesToIgnore > 0)  // Just to make certain
      skip<char>(bytesToIgnore);
    uint16_t count;
    read(count);
    return readPluginData(count, extraData, corePlugins);
  } else if (m_CompressionType == 1 || m_CompressionType == 2) {
    skipQDataStream(*m_Data, bytesToIgnore);
    uint16_t count;
    readQDataStream(*m_Data, count);
    return readPluginData(count, extraData, corePlugins);
  }
  return {};
}

QStringList
XngineSaveGame::FileWrapper::readMediumPlugins(int bytesToIgnore, int extraData,
                                                 const QStringList& corePlugins)
{
  if (m_CompressionType != 1) {
    return {};
  } else {
    skipQDataStream(*m_Data, bytesToIgnore);
    uint32_t count;
    readQDataStream(*m_Data, count);
    return readPluginData(count, extraData, corePlugins);
  }
}

QStringList XngineSaveGame::FileWrapper::readPluginData(uint32_t count, int extraData,
                                                          const QStringList corePlugins)
{
  QStringList plugins;
  plugins.reserve(count);
  if (m_CompressionType == 0) {
    for (std::size_t i = 0; i < count; ++i) {
      QString name;
      read(name);
      plugins.push_back(name);
    }
  } else {
    for (std::size_t i = 0; i < count; ++i) {
      QString name;
      read(name);
      plugins.push_back(name);
      bool isCustomPlugin;
      if (extraData) {
        if (extraData > 1) {
          readQDataStream(*m_Data, isCustomPlugin);
        } else {
          isCustomPlugin = !corePlugins.contains(name);
        }
        if (isCustomPlugin) {
          QString creationName;
          QString creationId;
          uint16_t flagsSize;
          uint8_t isCreation;
          read(creationName);
          read(creationId);
          readQDataStream(*m_Data, flagsSize);
          skipQDataStream(*m_Data, flagsSize);
          readQDataStream(*m_Data, isCreation);
        }
      }
    }
  }
  return plugins;
}

void XngineSaveGame::FileWrapper::close()
{
  m_File.close();
}



