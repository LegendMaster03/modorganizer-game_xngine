#include "xngine3dformat.h"

#include <QtEndian>

#include <cmath>
#include <cstring>
#include <utility>

namespace {

bool setError(QString* errorMessage, const QString& text)
{
  if (errorMessage != nullptr) {
    *errorMessage = text;
  }
  return false;
}

bool readLE16U(const QByteArray& data, qsizetype offset, quint16& value)
{
  if (offset < 0 || offset + 2 > data.size()) {
    return false;
  }
  quint16 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

bool readLE16S(const QByteArray& data, qsizetype offset, qint16& value)
{
  quint16 raw = 0;
  if (!readLE16U(data, offset, raw)) {
    return false;
  }
  value = static_cast<qint16>(raw);
  return true;
}

bool readLE32(const QByteArray& data, qsizetype offset, qint32& value)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }
  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  value = static_cast<qint32>(qFromLittleEndian(raw));
  return true;
}

bool readLE32U(const QByteArray& data, qsizetype offset, quint32& value)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }
  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  value = qFromLittleEndian(raw);
  return true;
}

bool readLE32F(const QByteArray& data, qsizetype offset, float& value)
{
  if (offset < 0 || offset + 4 > data.size()) {
    return false;
  }
  quint32 raw = 0;
  std::memcpy(&raw, data.constData() + offset, sizeof(raw));
  raw = qFromLittleEndian(raw);
  std::memcpy(&value, &raw, sizeof(value));
  return true;
}

Xngine3dFormat::TextureRef decodeV2xTextureRef(quint16 raw)
{
  Xngine3dFormat::TextureRef texture;
  const int textureId = static_cast<int>(raw >> 7);
  if (textureId < 2) {
    texture.isSolidColor = true;
    texture.colorIndex = static_cast<int>(raw & 0xFF);
    return texture;
  }

  texture.fileIndex = textureId;
  texture.imageIndex = static_cast<int>(raw & 0x7F);
  return texture;
}

Xngine3dFormat::TextureRef decodeV4V5TextureRef(quint32 raw)
{
  Xngine3dFormat::TextureRef texture;

  if ((raw >> 20) == 0x0FFFu) {
    texture.isSolidColor = true;
    texture.colorIndex = static_cast<int>((raw >> 8) & 0xFFu);
    return texture;
  }

  const quint32 texturePart = (raw >> 8);
  if (texturePart < 4000000u) {
    return texture;
  }

  const quint32 tempVal = texturePart - 4000000u;
  const quint32 ones = (tempVal / 250u) % 40u;
  const quint32 tens = ((tempVal - (ones * 250u)) / 1000u) % 100u;
  const quint32 hundreds =
      (tempVal - (ones * 250u) - (tens * 1000u)) / 4000u;
  texture.fileIndex = static_cast<int>(ones + tens + hundreds);

  const quint32 imageBits = raw & 0xFFu;
  const quint32 imageOnes = imageBits % 10u;
  const quint32 imageTens = (imageBits / 40u) * 10u;
  texture.imageIndex = static_cast<int>(imageOnes + imageTens);

  return texture;
}

qint16 decodeSigned12Bit(qint16 fromFile)
{
  qint16 v = static_cast<qint16>(fromFile & 0x0FFF);
  if ((v & 0x0800) != 0) {
    v = static_cast<qint16>(v | 0xF000);
  }
  return v;
}

int pointIndexForOffset(Xngine3dFormat::VersionTag versionTag, qint32 pointOffset)
{
  if (pointOffset < 0) {
    return -1;
  }
  if (versionTag == Xngine3dFormat::VersionTag::V2_5) {
    if ((pointOffset % 3) != 0) {
      return -1;
    }
    return pointOffset / 3;
  }
  if ((pointOffset % 12) != 0) {
    return -1;
  }
  return pointOffset / 12;
}

qsizetype v2xPointListOffset(const Xngine3dFormat::Header& h)
{
  return static_cast<qsizetype>(h.offsetVertexCoors);
}

struct DVec3
{
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

DVec3 toVec3(const Xngine3dFormat::Point& p)
{
  return {static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(p.z)};
}

DVec3 sub(const DVec3& a, const DVec3& b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

double dot(const DVec3& a, const DVec3& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool solvePlaneCoordinates(const Xngine3dFormat::Point& p0,
                           const Xngine3dFormat::Point& p1,
                           const Xngine3dFormat::Point& p2,
                           const Xngine3dFormat::Point& p,
                           double& outS, double& outT)
{
  const DVec3 a = toVec3(p0);
  const DVec3 b = toVec3(p1);
  const DVec3 c = toVec3(p2);
  const DVec3 q = toVec3(p);
  const DVec3 e1 = sub(b, a);
  const DVec3 e2 = sub(c, a);
  const DVec3 d = sub(q, a);

  const double a11 = dot(e1, e1);
  const double a12 = dot(e1, e2);
  const double a22 = dot(e2, e2);
  const double b1 = dot(d, e1);
  const double b2 = dot(d, e2);
  const double det = a11 * a22 - a12 * a12;
  if (std::abs(det) < 1.0e-12) {
    return false;
  }

  outS = (b1 * a22 - b2 * a12) / det;
  outT = (a11 * b2 - a12 * b1) / det;
  return true;
}

bool solveLinear4x4(double a[4][4], double b[4], double x[4])
{
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    double best = std::abs(a[col][col]);
    for (int r = col + 1; r < 4; ++r) {
      const double v = std::abs(a[r][col]);
      if (v > best) {
        best = v;
        pivot = r;
      }
    }
    if (best < 1.0e-12) {
      return false;
    }
    if (pivot != col) {
      for (int c = col; c < 4; ++c) {
        std::swap(a[col][c], a[pivot][c]);
      }
      std::swap(b[col], b[pivot]);
    }

    const double div = a[col][col];
    for (int c = col; c < 4; ++c) {
      a[col][c] /= div;
    }
    b[col] /= div;

    for (int r = 0; r < 4; ++r) {
      if (r == col) {
        continue;
      }
      const double factor = a[r][col];
      if (std::abs(factor) < 1.0e-20) {
        continue;
      }
      for (int c = col; c < 4; ++c) {
        a[r][c] -= factor * a[col][c];
      }
      b[r] -= factor * b[col];
    }
  }

  for (int i = 0; i < 4; ++i) {
    x[i] = b[i];
  }
  return true;
}

bool solveAffineUvFromFourAnchors(const Xngine3dFormat::Point& p0,
                                  const Xngine3dFormat::Point& p1,
                                  const Xngine3dFormat::Point& p2,
                                  const Xngine3dFormat::Point& p3,
                                  double u0, double u1, double u2, double u3,
                                  double v0, double v1, double v2, double v3,
                                  double uCoeff[4], double vCoeff[4])
{
  double mU[4][4] = {
      {static_cast<double>(p0.x), static_cast<double>(p0.y), static_cast<double>(p0.z), 1.0},
      {static_cast<double>(p1.x), static_cast<double>(p1.y), static_cast<double>(p1.z), 1.0},
      {static_cast<double>(p2.x), static_cast<double>(p2.y), static_cast<double>(p2.z), 1.0},
      {static_cast<double>(p3.x), static_cast<double>(p3.y), static_cast<double>(p3.z), 1.0},
  };
  double rhsU[4] = {u0, u1, u2, u3};
  if (!solveLinear4x4(mU, rhsU, uCoeff)) {
    return false;
  }

  double mV[4][4] = {
      {static_cast<double>(p0.x), static_cast<double>(p0.y), static_cast<double>(p0.z), 1.0},
      {static_cast<double>(p1.x), static_cast<double>(p1.y), static_cast<double>(p1.z), 1.0},
      {static_cast<double>(p2.x), static_cast<double>(p2.y), static_cast<double>(p2.z), 1.0},
      {static_cast<double>(p3.x), static_cast<double>(p3.y), static_cast<double>(p3.z), 1.0},
  };
  double rhsV[4] = {v0, v1, v2, v3};
  return solveLinear4x4(mV, rhsV, vCoeff);
}

void evalAffineUv(const Xngine3dFormat::Point& p, const double coeff[4], double& outValue)
{
  outValue = coeff[0] * static_cast<double>(p.x) +
             coeff[1] * static_cast<double>(p.y) +
             coeff[2] * static_cast<double>(p.z) + coeff[3];
}

void finalizePlaneUv(Xngine3dFormat::Plane& plane, const QVector<Xngine3dFormat::Point>& points,
                     QString& warning)
{
  const int count = plane.points.size();
  if (count <= 0) {
    return;
  }

  // Daggerfall/Battlespire v2.x interpretation:
  // 0 absolute, 1 delta-from-0, 2 delta-from-1, 3 absolute.
  plane.points[0].uAbsolute = plane.points[0].u;
  plane.points[0].vAbsolute = plane.points[0].v;

  if (count >= 2) {
    plane.points[1].uAbsolute = static_cast<qint32>(plane.points[0].uAbsolute) + plane.points[1].u;
    plane.points[1].vAbsolute = static_cast<qint32>(plane.points[0].vAbsolute) + plane.points[1].v;
  }
  if (count >= 3) {
    plane.points[2].uAbsolute = static_cast<qint32>(plane.points[1].uAbsolute) + plane.points[2].u;
    plane.points[2].vAbsolute = static_cast<qint32>(plane.points[1].vAbsolute) + plane.points[2].v;
  }
  if (count >= 4) {
    plane.points[3].uAbsolute = plane.points[3].u;
    plane.points[3].vAbsolute = plane.points[3].v;
  }

  bool usedAffine = false;
  if (count >= 5) {
    const int i0 = plane.points[0].pointIndex;
    const int i1 = plane.points[1].pointIndex;
    const int i2 = plane.points[2].pointIndex;
    const int i3 = plane.points[3].pointIndex;
    if (i0 >= 0 && i0 < points.size() &&
        i1 >= 0 && i1 < points.size() &&
        i2 >= 0 && i2 < points.size() &&
        i3 >= 0 && i3 < points.size()) {
      double uCoeff[4] = {};
      double vCoeff[4] = {};
      if (solveAffineUvFromFourAnchors(points.at(i0), points.at(i1), points.at(i2), points.at(i3),
                                       static_cast<double>(plane.points[0].uAbsolute),
                                       static_cast<double>(plane.points[1].uAbsolute),
                                       static_cast<double>(plane.points[2].uAbsolute),
                                       static_cast<double>(plane.points[3].uAbsolute),
                                       static_cast<double>(plane.points[0].vAbsolute),
                                       static_cast<double>(plane.points[1].vAbsolute),
                                       static_cast<double>(plane.points[2].vAbsolute),
                                       static_cast<double>(plane.points[3].vAbsolute),
                                       uCoeff, vCoeff)) {
        for (int i = 4; i < count; ++i) {
          const int pi = plane.points[i].pointIndex;
          if (pi < 0 || pi >= points.size()) {
            continue;
          }
          double u = 0.0;
          double v = 0.0;
          evalAffineUv(points.at(pi), uCoeff, u);
          evalAffineUv(points.at(pi), vCoeff, v);
          plane.points[i].uAbsolute = static_cast<qint32>(std::llround(u));
          plane.points[i].vAbsolute = static_cast<qint32>(std::llround(v));
          plane.points[i].uvReconstructed = true;
        }
        usedAffine = true;
        plane.uvMode = Xngine3dFormat::UvReconstructionMode::Affine4;
      }
    }
  }

  if (count >= 5 && !usedAffine) {
    const int i0 = plane.points[0].pointIndex;
    const int i1 = plane.points[1].pointIndex;
    const int i2 = plane.points[2].pointIndex;
    if (i0 >= 0 && i0 < points.size() &&
        i1 >= 0 && i1 < points.size() &&
        i2 >= 0 && i2 < points.size()) {
      const auto& p0 = points.at(i0);
      const auto& p1 = points.at(i1);
      const auto& p2 = points.at(i2);
      const double u0 = static_cast<double>(plane.points[0].uAbsolute);
      const double v0 = static_cast<double>(plane.points[0].vAbsolute);
      const double du1 = static_cast<double>(plane.points[1].uAbsolute - plane.points[0].uAbsolute);
      const double dv1 = static_cast<double>(plane.points[1].vAbsolute - plane.points[0].vAbsolute);
      const double du2 = static_cast<double>(plane.points[2].uAbsolute - plane.points[0].uAbsolute);
      const double dv2 = static_cast<double>(plane.points[2].vAbsolute - plane.points[0].vAbsolute);

      for (int i = 4; i < count; ++i) {
        const int pi = plane.points[i].pointIndex;
        if (pi < 0 || pi >= points.size()) {
          continue;
        }
        double s = 0.0;
        double t = 0.0;
        if (!solvePlaneCoordinates(p0, p1, p2, points.at(pi), s, t)) {
          continue;
        }
        const double u = u0 + s * du1 + t * du2;
        const double v = v0 + s * dv1 + t * dv2;
        plane.points[i].uAbsolute = static_cast<qint32>(std::llround(u));
        plane.points[i].vAbsolute = static_cast<qint32>(std::llround(v));
        plane.points[i].uvReconstructed = true;
      }
      plane.uvMode = Xngine3dFormat::UvReconstructionMode::Triangle3;
    } else if (warning.isEmpty()) {
      warning = "Some plane UVs could not be reconstructed (invalid point index)";
    }
  }

  for (int i = 0; i < count; ++i) {
    plane.points[i].uPixels = static_cast<double>(plane.points[i].uAbsolute) / 16.0;
    plane.points[i].vPixels = static_cast<double>(plane.points[i].vAbsolute) / 16.0;
  }
}

bool parseV2xRecord(const QByteArray& data, Xngine3dFormat::MeshRecord& outMesh,
                    QString* errorMessage)
{
  const auto& h = outMesh.header;
  if (h.numVertices < 0 || h.numFaces < 0 || h.section4Count < 0) {
    return setError(errorMessage, "3D record reports negative element counts");
  }
  if (h.offsetVertexCoors < 0 || h.offsetFaceData < 0 || h.offsetFaceNormals < 0 ||
      h.offsetFrameData < 0 || h.offsetSection4 < 0) {
    return setError(errorMessage, "3D record reports negative section offsets");
  }

  const qsizetype frameStart = static_cast<qsizetype>(h.offsetFrameData);
  if (frameStart >= 0 && frameStart + 16 <= data.size()) {
    readLE32U(data, frameStart + 0, outMesh.frameData.u1);
    readLE32U(data, frameStart + 4, outMesh.frameData.u2);
    readLE32U(data, frameStart + 8, outMesh.frameData.u3);
    readLE32U(data, frameStart + 12, outMesh.frameData.u4);
  }

  const qsizetype pointListOffset = v2xPointListOffset(h);
  const qsizetype pointBytes = static_cast<qsizetype>(h.numVertices) * 12;
  if (pointListOffset + pointBytes > data.size()) {
    return setError(errorMessage, "3D point list overflows record");
  }
  outMesh.points.reserve(h.numVertices);
  for (qint32 i = 0; i < h.numVertices; ++i) {
    const qsizetype off = pointListOffset + static_cast<qsizetype>(i) * 12;
    Xngine3dFormat::Point p;
    if (!readLE32(data, off + 0, p.x) || !readLE32(data, off + 4, p.y) ||
        !readLE32(data, off + 8, p.z)) {
      return setError(errorMessage, "Failed reading 3D point");
    }
    outMesh.points.push_back(p);
  }

  const qsizetype normalListOffset = static_cast<qsizetype>(h.offsetFaceNormals);
  const qsizetype normalBytes = static_cast<qsizetype>(h.numFaces) * 12;
  if (normalListOffset + normalBytes > data.size()) {
    return setError(errorMessage, "3D normal list overflows record");
  }
  outMesh.normals.reserve(h.numFaces);
  for (qint32 i = 0; i < h.numFaces; ++i) {
    const qsizetype off = normalListOffset + static_cast<qsizetype>(i) * 12;
    Xngine3dFormat::Point n;
    if (!readLE32(data, off + 0, n.x) || !readLE32(data, off + 4, n.y) ||
        !readLE32(data, off + 8, n.z)) {
      return setError(errorMessage, "Failed reading 3D normal");
    }
    outMesh.normals.push_back(n);
  }

  const qsizetype planeDataOffset = static_cast<qsizetype>(h.numUVOffsets);
  const qsizetype planeDataBytes = static_cast<qsizetype>(h.numFaces) * 24;
  if (planeDataOffset + planeDataBytes <= data.size()) {
    outMesh.planeData.reserve(h.numFaces);
    for (qint32 i = 0; i < h.numFaces; ++i) {
      const qsizetype off = planeDataOffset + static_cast<qsizetype>(i) * 24;
      outMesh.planeData.push_back(data.mid(off, 24));
    }
  } else {
    outMesh.warning = "PlaneDataList range is invalid for this record";
  }

  qsizetype planePos = static_cast<qsizetype>(h.offsetFaceData);
  outMesh.planes.reserve(h.numFaces);
  for (qint32 i = 0; i < h.numFaces; ++i) {
    if (planePos + 8 > data.size()) {
      return setError(errorMessage, "3D plane header overflows record");
    }

    Xngine3dFormat::Plane plane;
    plane.pointCount = static_cast<quint8>(data.at(planePos + 0));
    plane.unknown1 = static_cast<quint8>(data.at(planePos + 1));
    if (!readLE16U(data, planePos + 2, plane.textureRaw) ||
        !readLE32U(data, planePos + 4, plane.unknown2)) {
      return setError(errorMessage, "Failed reading 3D plane header");
    }
    plane.texture = decodeV2xTextureRef(plane.textureRaw);
    planePos += 8;

    const qsizetype planePointBytes = static_cast<qsizetype>(plane.pointCount) * 8;
    if (planePos + planePointBytes > data.size()) {
      return setError(errorMessage, "3D plane point list overflows record");
    }
    plane.points.reserve(plane.pointCount);
    for (quint8 p = 0; p < plane.pointCount; ++p) {
      Xngine3dFormat::PlanePoint pp;
      if (!readLE32(data, planePos + 0, pp.pointOffset) ||
          !readLE16S(data, planePos + 4, pp.u) ||
          !readLE16S(data, planePos + 6, pp.v)) {
        return setError(errorMessage, "Failed reading 3D plane point");
      }
      pp.vertexIndexRaw = static_cast<quint32>(pp.pointOffset);
      pp.u = decodeSigned12Bit(pp.u);
      pp.v = decodeSigned12Bit(pp.v);
      pp.pointIndex = pointIndexForOffset(h.versionTag, pp.pointOffset);
      plane.points.push_back(pp);
      planePos += 8;
    }

    finalizePlaneUv(plane, outMesh.points, outMesh.warning);
    outMesh.planes.push_back(plane);
  }

  qsizetype objectPos = static_cast<qsizetype>(h.offsetSection4);
  if (objectPos > data.size()) {
    outMesh.warning = "ObjectDataList offset points outside record";
    return true;
  }

  outMesh.objectData.reserve(h.section4Count);
  for (qint32 i = 0; i < h.section4Count; ++i) {
    if (objectPos + 18 > data.size()) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "ObjectDataList appears truncated in this record";
      }
      break;
    }

    Xngine3dFormat::ObjectData obj;
    if (!readLE32(data, objectPos + 0, obj.number0) ||
        !readLE32(data, objectPos + 4, obj.number1) ||
        !readLE32(data, objectPos + 8, obj.number2) ||
        !readLE32(data, objectPos + 12, obj.number3)) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "Failed decoding ObjectData header values";
      }
      break;
    }
    quint16 count = 0;
    if (!readLE16U(data, objectPos + 16, count)) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "Failed decoding ObjectData subrecord count";
      }
      break;
    }
    obj.subrecordCount = count;
    objectPos += 18;

    const qsizetype valuesBytes = static_cast<qsizetype>(obj.subrecordCount) * 6;
    if (objectPos + valuesBytes > data.size()) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "ObjectData subrecord list overflows record";
      }
      break;
    }

    obj.values.reserve(obj.subrecordCount);
    for (quint16 v = 0; v < obj.subrecordCount; ++v) {
      obj.values.push_back(data.mid(objectPos, 6));
      objectPos += 6;
    }
    outMesh.objectData.push_back(obj);
  }

  return true;
}

bool parseV4V5Record(const QByteArray& data, Xngine3dFormat::MeshRecord& outMesh,
                     QString* errorMessage)
{
  const auto& h = outMesh.header;
  if (h.numVertices < 0 || h.numFaces < 0) {
    return setError(errorMessage, "3D record reports negative element counts");
  }
  if (h.offsetVertexCoors < 0 || h.offsetFaceData < 0 || h.offsetFaceNormals < 0 ||
      h.offsetFrameData < 0 || h.offsetUVOffsets < 0 || h.offsetUVData < 0) {
    return setError(errorMessage, "3D record reports negative section offsets");
  }

  const qsizetype frameStart = static_cast<qsizetype>(h.offsetFrameData);
  if (frameStart + 16 <= data.size()) {
    if (!readLE32U(data, frameStart + 0, outMesh.frameData.u1) ||
        !readLE32U(data, frameStart + 4, outMesh.frameData.u2) ||
        !readLE32U(data, frameStart + 8, outMesh.frameData.u3) ||
        !readLE32U(data, frameStart + 12, outMesh.frameData.u4)) {
      return setError(errorMessage, "Failed reading frame data header");
    }
  } else {
    return setError(errorMessage, "Frame data header overflows record");
  }

  const qsizetype pointListOffset = static_cast<qsizetype>(h.offsetVertexCoors);
  const qsizetype pointBytes = static_cast<qsizetype>(h.numVertices) * 12;
  if (pointListOffset + pointBytes > data.size()) {
    return setError(errorMessage, "3D point list overflows record");
  }
  outMesh.points.reserve(h.numVertices);
  for (qint32 i = 0; i < h.numVertices; ++i) {
    const qsizetype off = pointListOffset + static_cast<qsizetype>(i) * 12;
    Xngine3dFormat::Point p;
    if (!readLE32(data, off + 0, p.x) || !readLE32(data, off + 4, p.y) ||
        !readLE32(data, off + 8, p.z)) {
      return setError(errorMessage, "Failed reading 3D point");
    }
    outMesh.points.push_back(p);
  }

  const qsizetype normalListOffset = static_cast<qsizetype>(h.offsetFaceNormals);
  const qsizetype normalBytes = static_cast<qsizetype>(h.numFaces) * 12;
  if (normalListOffset + normalBytes > data.size()) {
    return setError(errorMessage, "3D normal list overflows record");
  }
  outMesh.normals.reserve(h.numFaces);
  for (qint32 i = 0; i < h.numFaces; ++i) {
    const qsizetype off = normalListOffset + static_cast<qsizetype>(i) * 12;
    Xngine3dFormat::Point n;
    if (!readLE32(data, off + 0, n.x) || !readLE32(data, off + 4, n.y) ||
        !readLE32(data, off + 8, n.z)) {
      return setError(errorMessage, "Failed reading 3D normal");
    }
    outMesh.normals.push_back(n);
  }

  qsizetype facePos = static_cast<qsizetype>(h.offsetFaceData);
  outMesh.planes.reserve(h.numFaces);
  for (qint32 i = 0; i < h.numFaces; ++i) {
    if (facePos + 10 > data.size()) {
      return setError(errorMessage, "3D face header overflows record");
    }

    Xngine3dFormat::Plane plane;
    plane.textureIs32Bit = true;
    plane.pointCount = static_cast<quint8>(data.at(facePos + 0));
    plane.unknown1 = static_cast<quint8>(data.at(facePos + 1));
    if (!readLE32U(data, facePos + 2, plane.textureRaw32) ||
        !readLE32U(data, facePos + 6, plane.unknown2)) {
      return setError(errorMessage, "Failed reading 3D face header");
    }
    plane.texture = decodeV4V5TextureRef(plane.textureRaw32);
    facePos += 10;

    const qsizetype faceVertexBytes = static_cast<qsizetype>(plane.pointCount) * 8;
    if (facePos + faceVertexBytes > data.size()) {
      return setError(errorMessage, "3D face vertex list overflows record");
    }

    plane.points.reserve(plane.pointCount);
    for (quint8 p = 0; p < plane.pointCount; ++p) {
      Xngine3dFormat::PlanePoint pp;
      if (!readLE32U(data, facePos + 0, pp.vertexIndexRaw) ||
          !readLE16S(data, facePos + 4, pp.u) ||
          !readLE16S(data, facePos + 6, pp.v)) {
        return setError(errorMessage, "Failed reading 3D face vertex");
      }
      pp.pointOffset = static_cast<qint32>(pp.vertexIndexRaw);
      pp.pointIndex = static_cast<int>(pp.vertexIndexRaw);
      pp.uAbsolute = pp.u;
      pp.vAbsolute = pp.v;
      pp.uPixels = static_cast<double>(pp.uAbsolute) / 16.0;
      pp.vPixels = static_cast<double>(pp.vAbsolute) / 16.0;
      plane.points.push_back(pp);
      facePos += 8;
    }
    outMesh.planes.push_back(plane);
  }

  const qsizetype uvOffsetPos = static_cast<qsizetype>(h.offsetUVOffsets);
  if (h.numUVOffsets >= 0 &&
      uvOffsetPos + static_cast<qsizetype>(h.numUVOffsets) * 4 <= data.size()) {
    outMesh.uvOffsets.reserve(h.numUVOffsets);
    for (qint32 i = 0; i < h.numUVOffsets; ++i) {
      quint32 value = 0;
      if (!readLE32U(data, uvOffsetPos + static_cast<qsizetype>(i) * 4, value)) {
        return setError(errorMessage, "Failed reading UV offset");
      }
      outMesh.uvOffsets.push_back(value);
    }
  }

  const qsizetype uvDataPos = static_cast<qsizetype>(h.offsetUVData);
  if (uvDataPos >= 0 && uvDataPos + 12 <= data.size()) {
    qsizetype cursor = uvDataPos;
    while (cursor + 12 <= data.size()) {
      if (cursor == static_cast<qsizetype>(h.offsetVertexCoors) ||
          cursor == static_cast<qsizetype>(h.offsetFaceNormals) ||
          cursor == static_cast<qsizetype>(h.offsetFaceData)) {
        break;
      }
      Xngine3dFormat::UvCoordinate uv;
      if (!readLE32F(data, cursor + 0, uv.x) ||
          !readLE32F(data, cursor + 4, uv.y) ||
          !readLE32F(data, cursor + 8, uv.z)) {
        break;
      }
      outMesh.uvCoordinates.push_back(uv);
      cursor += 12;
      if (outMesh.uvCoordinates.size() > 65536) {
        break;
      }
    }
  }

  return true;
}

}  // namespace

QString Xngine3dFormat::versionTagToString(VersionTag versionTag)
{
  switch (versionTag) {
    case VersionTag::V2_5: return "v2.5";
    case VersionTag::V2_6: return "v2.6";
    case VersionTag::V2_7: return "v2.7";
    case VersionTag::V4_0: return "v4.0";
    case VersionTag::V5_0: return "v5.0";
    default: return {};
  }
}

Xngine3dFormat::VersionTag Xngine3dFormat::versionTagFromString(const QString& versionString)
{
  if (versionString == "v2.5") {
    return VersionTag::V2_5;
  }
  if (versionString == "v2.6") {
    return VersionTag::V2_6;
  }
  if (versionString == "v2.7") {
    return VersionTag::V2_7;
  }
  if (versionString == "v4.0") {
    return VersionTag::V4_0;
  }
  if (versionString == "v5.0") {
    return VersionTag::V5_0;
  }
  return VersionTag::Unknown;
}

bool Xngine3dFormat::parseHeader(const QByteArray& data, Header& outHeader, QString* errorMessage)
{
  outHeader = {};
  if (data.size() < 64) {
    return setError(errorMessage, "3D record is smaller than 64-byte header");
  }

  outHeader.versionString = QString::fromLatin1(data.constData(), 4);
  outHeader.versionTag = versionTagFromString(outHeader.versionString);

  if (!readLE32(data, 4, outHeader.numVertices) ||
      !readLE32(data, 8, outHeader.numFaces) ||
      !readLE32(data, 12, outHeader.radius) ||
      !readLE32(data, 16, outHeader.numFrames) ||
      !readLE32(data, 20, outHeader.offsetFrameData) ||
      !readLE32(data, 24, outHeader.numUVOffsets) ||
      !readLE32(data, 28, outHeader.offsetSection4) ||
      !readLE32(data, 32, outHeader.section4Count) ||
      !readLE32U(data, 36, outHeader.unknown4) ||
      !readLE32(data, 40, outHeader.offsetUVOffsets) ||
      !readLE32(data, 44, outHeader.offsetUVData) ||
      !readLE32(data, 48, outHeader.offsetVertexCoors) ||
      !readLE32(data, 52, outHeader.offsetFaceNormals) ||
      !readLE32U(data, 56, outHeader.numUVOffsets2) ||
      !readLE32(data, 60, outHeader.offsetFaceData)) {
    return setError(errorMessage, "Failed reading 3D header fields");
  }

  return true;
}

bool Xngine3dFormat::parseRecord(const QByteArray& data, MeshRecord& outMesh,
                                 QString* errorMessage)
{
  outMesh = {};
  if (!parseHeader(data, outMesh.header, errorMessage)) {
    return false;
  }

  switch (outMesh.header.versionTag) {
    case VersionTag::V2_5:
    case VersionTag::V2_6:
    case VersionTag::V2_7:
      return parseV2xRecord(data, outMesh, errorMessage);
    case VersionTag::V4_0:
    case VersionTag::V5_0:
      return parseV4V5Record(data, outMesh, errorMessage);
    default:
      return setError(errorMessage,
                      QString("Unsupported 3D version tag: '%1'")
                          .arg(outMesh.header.versionString));
  }
}
