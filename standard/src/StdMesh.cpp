/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <StdMesh.h>

StdMeshError::StdMeshError(const StdStrBuf& message, const char* file, unsigned int line)
{
  Buf.Format("%s[%u]: %s", file, line, message.getData());
}

void StdMeshMatrix::SetIdentity()
{
  a[0][0] = 1.0f; a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = 0.0f;
  a[1][0] = 0.0f; a[1][1] = 1.0f; a[1][2] = 0.0f; a[1][3] = 0.0f;
  a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = 1.0f; a[2][3] = 0.0f;
}

void StdMeshMatrix::SetTranslate(float dx, float dy, float dz)
{
  a[0][0] = 1.0f; a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = dx;
  a[1][0] = 0.0f; a[1][1] = 1.0f; a[1][2] = 0.0f; a[1][3] = dy;
  a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = 1.0f; a[2][3] = dz;
}

void StdMeshMatrix::SetScale(float sx, float sy, float sz)
{
  a[0][0] = sx;   a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = 0.0f;
  a[1][0] = 0.0f; a[1][1] = sy;   a[1][2] = 0.0f; a[1][3] = 0.0f;
  a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = sz;   a[2][3] = 0.0f;
}

void StdMeshMatrix::SetRotate(float angle, float rx, float ry, float rz)
{
  // We do normalize the rx,ry,rz vector here: This is only required for
  // precalculations anyway, thus not time-critical.
  float abs = sqrt(rx*rx+ry*ry+rz*rz);
  rx/=abs; ry/=abs; rz/=abs;
  float c = cos(angle), s = sin(angle);

  a[0][0] = rx*rx*(1-c)+c;    a[0][1] = rx*ry*(1-c)-rz*s; a[0][2] = rx*rz*(1-c)+ry*s; a[0][3] = 0.0f;
  a[1][0] = ry*rx*(1-c)+rz*s; a[1][1] = ry*ry*(1-c)+c;    a[1][2] = ry*rz*(1-c)-rx*s; a[1][3] = 0.0f;
  a[2][0] = rz*rx*(1-c)-ry*s; a[2][1] = ry*rz*(1-c)+rx*s; a[2][2] = rz*rz*(1-c)+c;    a[2][3] = 0.0f;
}

void StdMeshMatrix::Mul(const StdMeshMatrix& other)
{
  StdMeshMatrix old(*this);

  a[0][0] = old.a[0][0]*other.a[0][0] + old.a[0][1]*other.a[1][0] + old.a[0][2]*other.a[2][0];
  a[1][0] = old.a[1][0]*other.a[0][0] + old.a[1][1]*other.a[1][0] + old.a[1][2]*other.a[2][0];
  a[2][0] = old.a[2][0]*other.a[0][0] + old.a[2][1]*other.a[1][0] + old.a[2][2]*other.a[2][0];

  a[0][1] = old.a[0][0]*other.a[0][1] + old.a[0][1]*other.a[1][1] + old.a[0][2]*other.a[2][1];
  a[1][1] = old.a[1][0]*other.a[0][1] + old.a[1][1]*other.a[1][1] + old.a[1][2]*other.a[2][1];
  a[2][1] = old.a[2][0]*other.a[0][1] + old.a[2][1]*other.a[1][1] + old.a[2][2]*other.a[2][1];

  a[0][2] = old.a[0][0]*other.a[0][2] + old.a[0][1]*other.a[1][2] + old.a[0][2]*other.a[2][2];
  a[1][2] = old.a[1][0]*other.a[0][2] + old.a[1][1]*other.a[1][2] + old.a[1][2]*other.a[2][2];
  a[2][2] = old.a[2][0]*other.a[0][2] + old.a[2][1]*other.a[1][2] + old.a[2][2]*other.a[2][2];

  a[0][3] = old.a[0][0]*other.a[0][3] + old.a[0][1]*other.a[1][3] + old.a[0][2]*other.a[2][3] + old.a[0][3];
  a[1][3] = old.a[1][0]*other.a[0][3] + old.a[1][1]*other.a[1][3] + old.a[1][2]*other.a[2][3] + old.a[1][3];
  a[2][3] = old.a[2][0]*other.a[0][3] + old.a[2][1]*other.a[1][3] + old.a[2][2]*other.a[2][3] + old.a[2][3];
}

void StdMeshMatrix::Mul(float f)
{
  a[0][0] *= f;
  a[0][1] *= f;
  a[0][2] *= f;
  a[0][3] *= f;
  a[1][0] *= f;
  a[1][1] *= f;
  a[1][2] *= f;
  a[1][3] *= f;
  a[2][0] *= f;
  a[2][1] *= f;
  a[2][2] *= f;
  a[2][3] *= f;
}

void StdMeshMatrix::Add(const StdMeshMatrix& other)
{
  a[0][0] += other.a[0][0];
  a[0][1] += other.a[0][1];
  a[0][2] += other.a[0][2];
  a[0][3] += other.a[0][3];
  a[1][0] += other.a[1][0];
  a[1][1] += other.a[1][1];
  a[1][2] += other.a[1][2];
  a[1][3] += other.a[1][3];
  a[2][0] += other.a[2][0];
  a[2][1] += other.a[2][1];
  a[2][2] += other.a[2][2];
  a[2][3] += other.a[2][3];
}

void StdMeshMatrix::Transform(const StdMeshMatrix& other)
{
  // StdMeshMatrix blah(other);
  // bla.Mul(*this);
  // *this = bla;

  StdMeshMatrix old(*this);

  a[0][0] = other.a[0][0]*old.a[0][0] + other.a[0][1]*old.a[1][0] + other.a[0][2]*old.a[2][0];
  a[1][0] = other.a[1][0]*old.a[0][0] + other.a[1][1]*old.a[1][0] + other.a[1][2]*old.a[2][0];
  a[2][0] = other.a[2][0]*old.a[0][0] + other.a[2][1]*old.a[1][0] + other.a[2][2]*old.a[2][0];

  a[0][1] = other.a[0][0]*old.a[0][1] + other.a[0][1]*old.a[1][1] + other.a[0][2]*old.a[2][1];
  a[1][1] = other.a[1][0]*old.a[0][1] + other.a[1][1]*old.a[1][1] + other.a[1][2]*old.a[2][1];
  a[2][1] = other.a[2][0]*old.a[0][1] + other.a[2][1]*old.a[1][1] + other.a[2][2]*old.a[2][1];

  a[0][2] = other.a[0][0]*old.a[0][2] + other.a[0][1]*old.a[1][2] + other.a[0][2]*old.a[2][2];
  a[1][2] = other.a[1][0]*old.a[0][2] + other.a[1][1]*old.a[1][2] + other.a[1][2]*old.a[2][2];
  a[2][2] = other.a[2][0]*old.a[0][2] + other.a[2][1]*old.a[1][2] + other.a[2][2]*old.a[2][2];

  a[0][3] = other.a[0][0]*old.a[0][3] + other.a[0][1]*old.a[1][3] + other.a[0][2]*old.a[2][3] + other.a[0][3];
  a[1][3] = other.a[1][0]*old.a[0][3] + other.a[1][1]*old.a[1][3] + other.a[1][2]*old.a[2][3] + other.a[1][3];
  a[2][3] = other.a[2][0]*old.a[0][3] + other.a[2][1]*old.a[1][3] + other.a[2][2]*old.a[2][3] + other.a[2][3];
}

void StdMeshVertex::Transform(const StdMeshMatrix& trans)
{
  StdMeshVertex old(*this);

  x = trans(0,0)*old.x + trans(0,1)*old.y + trans(0,2)*old.z + trans(0,3);
  y = trans(1,0)*old.x + trans(1,1)*old.y + trans(1,2)*old.z + trans(0,3);
  z = trans(2,0)*old.x + trans(2,1)*old.y + trans(2,2)*old.z + trans(0,3);
  nx = trans(0,0)*old.nx + trans(0,1)*old.ny + trans(0,2)*old.nz;
  ny = trans(1,0)*old.nx + trans(1,1)*old.ny + trans(0,2)*old.nz;
  nz = trans(2,0)*old.nx + trans(2,1)*old.ny + trans(2,2)*old.nz;
}

#if 0
void StdMeshVertex::Mul(float f)
{
  x *= f;
  y *= f;
  z *= f;

  // We also multiplicate normals because we expect this to happen in
  // an expression such as a*v1 + (1-a)*v2 which would ensure normalization
  // of the normals again.
  nx *= f;
  ny *= f;
  nz *= f;
}

void StdMeshVertex::Add(const StdMeshVertex& other)
{
  x += other.x;
  y += other.y;
  z += other.z;

  nx += other.nx;
  ny += other.ny;
  nz += other.nz;
}
#endif

StdMeshMatrix StdMeshTrack::GetTransformAt(float time) const
{
  std::map<float, StdMeshKeyFrame>::const_iterator iter = Frames.lower_bound(time);

  // If this points to end(), then either
  // a) time > animation length
  // b) The track does not include a frame for the very end of the animation
  // Both is considered an error
  assert(iter != Frames.end());

  if(iter == Frames.begin())
    return iter->second.Trans;

  std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
  -- prev_iter;

  float dt = iter->first - prev_iter->first;
  float weight1 = (time - prev_iter->first) / dt;
  float weight2 = (iter->first - time) / dt;

  assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
  assert(fabs(weight1 + weight2 - 1) < 1e-6);

  StdMeshMatrix trans1 = iter->second.Trans;
  StdMeshMatrix trans2 = prev_iter->second.Trans;
  trans1.Mul(weight1);
  trans2.Mul(weight2);

  trans1.Add(trans2);
  return trans1;
}

StdMeshAnimation::~StdMeshAnimation()
{
  for(unsigned int i = 0; i < Tracks.size(); ++i)
    delete Tracks[i];
}

StdMesh::StdMesh():
  Material(NULL)
{
}

StdMesh::~StdMesh()
{
  for(unsigned int i = 0; i < Bones.size(); ++i)
    delete Bones[i];
}

void StdMesh::InitXML(const char* xml_data, StdMeshSkeletonLoader& skel_loader, const StdMeshMatManager& manager)
{
  
}
