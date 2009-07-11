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

#include <StdMeshMaterial.h>

#include <cctype>

StdMeshMaterialError::StdMeshMaterialError(const StdStrBuf& message, const char* file, unsigned int line)
{
  Buf.Format("%s:%u: %s", file, line, message.getData());
}

enum Token
{
  TOKEN_IDTF,
  TOKEN_BRACE_OPEN,
  TOKEN_BRACE_CLOSE,
  TOKEN_COLON,
  TOKEN_EOF
};

class StdMeshMaterialParserCtx
{
public:
  StdMeshMaterialParserCtx(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader);

  void SkipWhitespace();
  Token Peek(StdStrBuf& name);
  Token Advance(StdStrBuf& name);
  Token AdvanceNonEOF(StdStrBuf& name);
  Token AdvanceRequired(StdStrBuf& name, Token expect);
  Token AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2);
  float AdvanceFloat();
  bool AdvanceFloatOptional(float& value);
  bool AdvanceBoolean();
  void Error(const StdStrBuf& message);
  void ErrorUnexpectedIdentifier(const StdStrBuf& identifier);

  // Current parsing data
  unsigned int Line;
  const char* Script;

  StdStrBuf FileName;
  StdMeshMaterialTextureLoader& TextureLoader;
};

StdMeshMaterialParserCtx::StdMeshMaterialParserCtx(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader):
  Line(0), Script(mat_script), FileName(filename), TextureLoader(tex_loader)
{
}

void StdMeshMaterialParserCtx::SkipWhitespace()
{
  while(isspace(*Script))
  {
    if(*Script == '\n') ++Line;
    ++Script;
  }
}

Token StdMeshMaterialParserCtx::Peek(StdStrBuf& name)
{
  SkipWhitespace();

  const char* before = Script;
  Token tok = Advance(name);
  Script = before;
  return tok;
}

Token StdMeshMaterialParserCtx::Advance(StdStrBuf& name)
{
  SkipWhitespace();

  switch(*Script)
  {
  case '\0':
    name.Clear();
    return TOKEN_EOF;
  case '{':
    ++Script;
    name = "{";
    return TOKEN_BRACE_OPEN;
  case '}':
    ++Script;
    name = "}";
    return TOKEN_BRACE_CLOSE;
  case ':':
    ++Script;
    name = ":";
    return TOKEN_COLON;
  default:
    const char* begin = Script;
    // Advance to next whitespace
    do { ++Script; } while(!isspace(*Script) && *Script != '{' && *Script != '}' && *Script != ':');
    name.Copy(begin, Script - begin);
    return TOKEN_IDTF;
  }
}

Token StdMeshMaterialParserCtx::AdvanceNonEOF(StdStrBuf& name)
{
  Token token = Advance(name);
  if(token == TOKEN_EOF) Error(StdStrBuf("Unexpected end of file"));
  return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect)
{
  Token token = AdvanceNonEOF(name);
  // TODO: Explain what was actually expected
  if(token != expect) Error(StdStrBuf("'") + name + "' unexpected");
  return token;
}

Token StdMeshMaterialParserCtx::AdvanceRequired(StdStrBuf& name, Token expect1, Token expect2)
{
  Token token = AdvanceNonEOF(name);
  // TODO: Explain what was actually expected
  if(token != expect1 && token != expect2)
    Error(StdStrBuf("'") + name + "' unexpected");
  return token;
}

float StdMeshMaterialParserCtx::AdvanceFloat()
{
  StdStrBuf buf;
  AdvanceRequired(buf, TOKEN_IDTF);
  char* end;
  float f = strtof(buf.getData(), &end);
  if(*end != '\0') Error(StdStrBuf("Floating point value expected"));
  return f;
}

bool StdMeshMaterialParserCtx::AdvanceFloatOptional(float& value)
{
  StdStrBuf buf;
  Token tok = Peek(buf);

  if(tok == TOKEN_IDTF && isdigit(buf[0]))
  {
    value = AdvanceFloat();
    return true;
  }

  return false;
}

bool StdMeshMaterialParserCtx::AdvanceBoolean()
{
  StdStrBuf buf;
  AdvanceRequired(buf, TOKEN_IDTF);
  if(buf == "on") return true;
  if(buf == "off") return false;
  Error(StdStrBuf("Expected either 'on' or 'off', but not '") + buf + "'");
  return false; // Never reached
}

void StdMeshMaterialParserCtx::Error(const StdStrBuf& message)
{
  throw StdMeshMaterialError(message, FileName.getData(), Line);
}

void StdMeshMaterialParserCtx::ErrorUnexpectedIdentifier(const StdStrBuf& identifier)
{
  Error(StdStrBuf("Unexpected identifier: '") + identifier + "'");
}

StdMeshMaterialTextureUnit::TexRef::TexRef(unsigned int size):
  RefCount(1), Tex(size, false)
{
}

StdMeshMaterialTextureUnit::TexRef::~TexRef()
{
  assert(RefCount == 0);
}

StdMeshMaterialTextureUnit::StdMeshMaterialTextureUnit():
  Texture(NULL)
{
}

StdMeshMaterialTextureUnit::StdMeshMaterialTextureUnit(const StdMeshMaterialTextureUnit& other):
  Texture(other.Texture)
{
  if(Texture)
    ++Texture->RefCount;
}

StdMeshMaterialTextureUnit::~StdMeshMaterialTextureUnit()
{
  if(Texture && !--Texture->RefCount)
    delete Texture;
}

StdMeshMaterialTextureUnit& StdMeshMaterialTextureUnit::operator=(const StdMeshMaterialTextureUnit& other)
{
  if(this == &other) return *this;
  if(Texture) if(!--Texture->RefCount) delete Texture;
  Texture = other.Texture;
  if(Texture) ++Texture->RefCount;
  return *this;
}

void StdMeshMaterialTextureUnit::Load(StdMeshMaterialParserCtx& ctx)
{
  Token token;
  StdStrBuf token_name;
  while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
  {
    if(token_name == "texture")
    {
      ctx.AdvanceRequired(token_name, TOKEN_IDTF);

      CPNGFile png;
      if(!ctx.TextureLoader.LoadTexture(token_name.getData(), png))
        ctx.Error(StdStrBuf("Could not load texture '") + token_name + "'");

      if(png.iWdt != png.iHgt)
        ctx.Error(StdStrBuf("Texture '") + token_name + "' is not quadratic");

      Texture = new TexRef(png.iWdt);

      Texture->Tex.Lock();
      for(unsigned int y = 0; y < png.iHgt; ++y)
        for(unsigned int x = 0; x < png.iWdt; ++x)
          Texture->Tex.SetPix4(x, y, png.GetPix(x, y));
      Texture->Tex.Unlock();
    }
    else
      ctx.ErrorUnexpectedIdentifier(token_name);
  }

  if(token != TOKEN_BRACE_CLOSE)
    ctx.Error(StdStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterialPass::StdMeshMaterialPass()
{
  Ambient[0]  = Ambient[1]  = Ambient[2]  = 1.0f; Ambient[3]  = 0.0f;
  Diffuse[0]  = Diffuse[1]  = Diffuse[2]  = 1.0f; Diffuse[3]  = 0.0f;
  Specular[0] = Specular[1] = Specular[2] = 0.0f; Specular[3] = 1.0f;
  Emissive[0] = Emissive[1] = Emissive[2] = 0.0f; Emissive[3] = 1.0f;
  Shininess = 0.0f;
}

void StdMeshMaterialPass::Load(StdMeshMaterialParserCtx& ctx)
{
  Token token;
  StdStrBuf token_name;
  while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
  {
    if(token_name == "texture_unit")
    {
      // TODO: Can there be an optional name?
      ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
      TextureUnits.push_back(StdMeshMaterialTextureUnit());
      TextureUnits.back().Load(ctx);
    }
    else if(token_name == "ambient")
    {
      Ambient[0] = ctx.AdvanceFloat();
      Ambient[1] = ctx.AdvanceFloat();
      Ambient[2] = ctx.AdvanceFloat();
      if(ctx.AdvanceFloatOptional(Ambient[3]))
        Ambient[3] = 1 - Ambient[3];
    }
    else if(token_name == "diffuse")
    {
      Diffuse[0] = ctx.AdvanceFloat();
      Diffuse[1] = ctx.AdvanceFloat();
      Diffuse[2] = ctx.AdvanceFloat();
      if(ctx.AdvanceFloatOptional(Diffuse[3]))
        Diffuse[3] = 1 - Diffuse[3];
    }
    else if(token_name == "specular")
    {
      Specular[0] = ctx.AdvanceFloat();
      Specular[1] = ctx.AdvanceFloat();
      Specular[2] = ctx.AdvanceFloat();

      // The fourth argument is optional, not the fifth:
      float specular3 = ctx.AdvanceFloat();

      float shininess;
      if(ctx.AdvanceFloatOptional(shininess))
      {
        Specular[3] = 1 - specular3;
        Shininess = shininess;
      }
      else
      {
        Shininess = specular3;
      }
    }
    else if(token_name == "emissive")
    {
      Emissive[0] = ctx.AdvanceFloat();
      Emissive[1] = ctx.AdvanceFloat();
      Emissive[2] = ctx.AdvanceFloat();
      if(ctx.AdvanceFloatOptional(Emissive[3]))
        Emissive[3] = 1 - Emissive[3];
    }
    else
      ctx.ErrorUnexpectedIdentifier(token_name);
  }

  if(token != TOKEN_BRACE_CLOSE)
    ctx.Error(StdStrBuf("'") + token_name.getData() + "' unexpected");
}

void StdMeshMaterialTechnique::Load(StdMeshMaterialParserCtx& ctx)
{
  Token token;
  StdStrBuf token_name;
  while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
  {
    if(token_name == "pass")
    {
      // TODO: Can there be an optional name?
      ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
      Passes.push_back(StdMeshMaterialPass());
      Passes.back().Load(ctx);
    }
    else
      ctx.ErrorUnexpectedIdentifier(token_name);
  }

  if(token != TOKEN_BRACE_CLOSE)
    ctx.Error(StdStrBuf("'") + token_name.getData() + "' unexpected");
}

StdMeshMaterial::StdMeshMaterial():
  Line(0), ReceiveShadows(true)
{
}

void StdMeshMaterial::Load(StdMeshMaterialParserCtx& ctx)
{
  Token token;
  StdStrBuf token_name;
  while((token = ctx.AdvanceNonEOF(token_name)) == TOKEN_IDTF)
  {
    if(token_name == "technique")
    {
      // TODO: Can there be an optional name?
      ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);
      Techniques.push_back(StdMeshMaterialTechnique());
      Techniques.back().Load(ctx);
    }
    else if(token_name == "receive_shadows")
    {
      ReceiveShadows = ctx.AdvanceBoolean();
    }
    else
      ctx.ErrorUnexpectedIdentifier(token_name);
  }
  
  if(token != TOKEN_BRACE_CLOSE)
    ctx.Error(StdStrBuf("'") + token_name.getData() + "' unexpected");
}

void StdMeshMatManager::Clear()
{
  Materials.clear();
}

void StdMeshMatManager::Parse(const char* mat_script, const char* filename, StdMeshMaterialTextureLoader& tex_loader)
{
  StdMeshMaterialParserCtx ctx(mat_script, filename, tex_loader);

  Token token;
  StdStrBuf token_name;
  while((token = ctx.Advance(token_name)) == TOKEN_IDTF)
  {
    if(token_name == "material")
    {
      // Read name
      StdStrBuf material_name;
      ctx.AdvanceRequired(material_name, TOKEN_IDTF);

      // Check for uniqueness
      std::map<StdStrBuf, StdMeshMaterial>::iterator iter = Materials.find(material_name);
      if(iter != Materials.end())
        ctx.Error(FormatString("Material with name '%s' is already defined in %s:%u", material_name.getData(), iter->second.FileName.getData(), iter->second.Line));

      // Check if there is a parent given
      Token next = ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN, TOKEN_COLON);
      // Read parent name, if any
      StdMeshMaterial* parent = NULL;
      if(next == TOKEN_COLON)
      {
        // Note that if there is a parent, then it needs to be loaded
        // already. This currently makes only sense when its defined above
        // in the same material script file or in a parent definition.
        // We could later support material scripts in the System.c4g.
        StdStrBuf parent_name;
        ctx.AdvanceRequired(parent_name, TOKEN_IDTF);
        ctx.AdvanceRequired(token_name, TOKEN_BRACE_OPEN);

        iter = Materials.find(parent_name);
        if(iter == Materials.end())
          ctx.Error(StdStrBuf("Parent material '") + parent_name + "' does not exist (or is not yet loaded)");
        parent = &iter->second;
      }

      // Copy properties from parent if one is given, otherwise
      // default-construct the material.
      StdMeshMaterial& mat = Materials.insert(std::make_pair(material_name, parent ? StdMeshMaterial(*parent) : StdMeshMaterial())).first->second;
      // Set/Overwrite source and name
      mat.Name = material_name;
      mat.FileName = ctx.FileName;
      mat.Line = ctx.Line;

      mat.Load(ctx);
    }
    else
      ctx.ErrorUnexpectedIdentifier(token_name);
  }

  if(token != TOKEN_EOF)
    ctx.Error(StdStrBuf("'") + token_name.getData() + "' unexpected");
}

const StdMeshMaterial* StdMeshMatManager::GetMaterial(const char* material_name) const
{
  std::map<StdStrBuf, StdMeshMaterial>::const_iterator iter = Materials.find(StdStrBuf(material_name));
  if(iter == Materials.end()) return NULL;
  return &iter->second;
}

// vim: et ts=2 sw=2
