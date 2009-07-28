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

#ifndef INC_StdMeshMaterial
#define INC_StdMeshMaterial

#include <Standard.h>
#include <StdBuf.h>
#include <StdPNG.h>
#include <StdSurface2.h>

#include <vector>
#include <map>

// TODO: Support more features of OGRE material scripts
// Refer to http://www.ogre3d.org/docs/manual/manual_14.html

class StdMeshMaterialParserCtx;

class StdMeshMaterialError: public std::exception
{
public:
  StdMeshMaterialError(const StdStrBuf& message, const char* file, unsigned int line);
  virtual ~StdMeshMaterialError() throw() {}

  virtual const char* what() const throw() { return Buf.getData(); }

protected:
  StdStrBuf Buf;
};

// Interface to load textures. Given a texture filename occuring in the
// material script, this should load the texture from wherever the material
// script is actually loaded, for example from a C4Group.
class StdMeshMaterialTextureLoader
{
public:
  virtual bool LoadTexture(const char* filename, CPNGFile& dest) = 0;
};

class StdMeshMaterialTextureUnit
{
public:
  // Ref-counted texture. When a meterial inherits from one which contains
  // a TextureUnit, then they will share the same CTexRef.
  class TexRef
  {
  public:
    TexRef(unsigned int size);
    ~TexRef();

    unsigned int RefCount;
    CTexRef Tex;
  };

  StdMeshMaterialTextureUnit();
  StdMeshMaterialTextureUnit(const StdMeshMaterialTextureUnit& other);
  ~StdMeshMaterialTextureUnit();

  StdMeshMaterialTextureUnit& operator=(const StdMeshMaterialTextureUnit&);

  void Load(StdMeshMaterialParserCtx& ctx);

  const CTexRef& GetTexture() const { return Texture->Tex; }

private:
  TexRef* Texture;
};

class StdMeshMaterialPass
{
public:
  StdMeshMaterialPass();
  void Load(StdMeshMaterialParserCtx& ctx);

  std::vector<StdMeshMaterialTextureUnit> TextureUnits;

  float Ambient[4];
  float Diffuse[4];
  float Specular[4];
  float Emissive[4];
  float Shininess;
};

class StdMeshMaterialTechnique
{
public:
  void Load(StdMeshMaterialParserCtx& ctx);

  std::vector<StdMeshMaterialPass> Passes;
};

class StdMeshMaterial
{
public:
  StdMeshMaterial();
  void Load(StdMeshMaterialParserCtx& ctx);

  // Location the Material was loaded from
  StdStrBuf FileName;
  unsigned int Line;

  // Material name
  StdStrBuf Name;

  // Not currently used in Clonk, but don't fail when we see this in a
  // Material script:
  bool ReceiveShadows;

  // Available techniques
  std::vector<StdMeshMaterialTechnique> Techniques;
};

class StdMeshMatManager
{
public:
  // Remove all materials from manager. Make sure there is no StdMesh
  // referencing any out there before calling this.
  void Clear();

  // Parse a material script file, and add the materials to the manager.
  // filename may be NULL if the source is not a file. It will only be used
  // for error messages.
  // Throws StdMeshMaterialError.
  void Parse(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader);

  // Get material by name. NULL if there is no such material with this name.
  const StdMeshMaterial* GetMaterial(const char* material_name) const;

private:
  std::map<StdStrBuf, StdMeshMaterial> Materials;
};

#endif

// vim: et ts=2 sw=2
