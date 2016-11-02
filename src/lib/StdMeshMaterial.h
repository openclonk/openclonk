/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#ifndef INC_StdMeshMaterial
#define INC_StdMeshMaterial

#include "lib/StdBuf.h"
#include "graphics/C4Surface.h"
#include "graphics/C4Shader.h"

#include <vector>
#include <map>
#include <tuple>

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
	StdCopyStrBuf Buf;
};

class StdMeshMaterialShaderParameter
{
public:
	enum Type {
		AUTO,
		AUTO_TEXTURE_MATRIX, // Texture matrix for the i-th texture
		INT,
		FLOAT,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		MATRIX_4X4
	};

	enum Auto {
		// TODO: OGRE auto values
		AUTO_DUMMY
	};

	StdMeshMaterialShaderParameter(); // type=FLOAT, value uninitialized
	StdMeshMaterialShaderParameter(Type type); // value uninitialized
	StdMeshMaterialShaderParameter(const StdMeshMaterialShaderParameter& other);
	StdMeshMaterialShaderParameter(StdMeshMaterialShaderParameter &&other);
	~StdMeshMaterialShaderParameter();

	StdMeshMaterialShaderParameter& operator=(const StdMeshMaterialShaderParameter& other);
	StdMeshMaterialShaderParameter& operator=(StdMeshMaterialShaderParameter &&other);

	Type GetType() const { return type; }
	void SetType(Type type); // changes type, new value is uninitialized

	// Getters
	Auto GetAuto() const { assert(type == AUTO); return a; }
	int GetInt() const { assert(type == INT || type == AUTO_TEXTURE_MATRIX); return i; }
	float GetFloat() const { assert(type == FLOAT); return f[0]; }
	const float* GetFloatv() const { assert(type == FLOAT2 || type == FLOAT3 || type == FLOAT4); return f; }
	const float* GetMatrix() const { assert(type == MATRIX_4X4); return matrix; }

	// Setters
	Auto& GetAuto() { assert(type == AUTO); return a; }
	int& GetInt() { assert(type == INT || type == AUTO_TEXTURE_MATRIX); return i; }
	float& GetFloat() { assert(type == FLOAT); return f[0]; }
	float* GetFloatv() { assert(type == FLOAT2 || type == FLOAT3 || type == FLOAT4); return f; }
	float* GetMatrix() { assert(type == MATRIX_4X4); return matrix; }
private:
	void CopyShallow(const StdMeshMaterialShaderParameter& other);
	void CopyDeep(const StdMeshMaterialShaderParameter& other);
	void Move(StdMeshMaterialShaderParameter &&other);

	Type type;

	union {
		Auto a;
		int i;
		float f[4];
		float* matrix; // 16 floats, row-major order
	};
};

class StdMeshMaterialShaderParameters
{
public:
	StdMeshMaterialShaderParameters();

	void Load(StdMeshMaterialParserCtx& ctx);

	StdMeshMaterialShaderParameter& AddParameter(const char* name, StdMeshMaterialShaderParameter::Type type);

	std::vector<std::pair<StdCopyStrBuf, StdMeshMaterialShaderParameter> > NamedParameters;
private:
	StdMeshMaterialShaderParameter LoadConstParameter(StdMeshMaterialParserCtx& ctx);
	StdMeshMaterialShaderParameter LoadAutoParameter(StdMeshMaterialParserCtx& ctx);
};

enum StdMeshMaterialShaderType {
	SMMS_FRAGMENT,
	SMMS_VERTEX,
	SMMS_GEOMETRY
};

// Interface to load additional resources.
// Given a texture filename occuring in the
// material script, this should load the texture from wherever the material
// script is actually loaded, for example from a C4Group.
// Given a shader filename, this should load the shader text.
class StdMeshMaterialLoader
{
public:
	virtual C4Surface* LoadTexture(const char* filename) = 0;
	virtual StdStrBuf LoadShaderCode(const char* filename) = 0;
	virtual void AddShaderSlices(C4Shader& shader, int ssc) = 0; // add default shader slices
	virtual ~StdMeshMaterialLoader() {}
};

// This is just a container class to hold the shader code; the C4Shader
// objects are later created from that code by mixing them with the default
// slices.
class StdMeshMaterialShader
{
public:
	StdMeshMaterialShader(const char* filename, const char* name, const char* language, StdMeshMaterialShaderType /* type */, const char* code):
		Filename(filename), Name(name), Language(language), Code(code)
	{}

	const char* GetFilename() const { return Filename.getData(); }
	const char* GetCode() const { return Code.getData(); }

private:
	StdCopyStrBuf Filename;
	StdCopyStrBuf Name;
	StdCopyStrBuf Language;
	StdCopyStrBuf Code;
};

class StdMeshMaterialProgram
{
public:
	StdMeshMaterialProgram(const char* name, const StdMeshMaterialShader* fragment_shader, const StdMeshMaterialShader* vertex_shader, const StdMeshMaterialShader* geometry_shader);
	bool AddParameterNames(const StdMeshMaterialShaderParameters& parameters); // returns true if some parameter names were not yet registered.

	bool IsCompiled() const { return Shader.Initialised(); }
	bool Compile(StdMeshMaterialLoader& loader);

	const C4Shader* GetShader(int ssc) const;
	int GetParameterIndex(const char* name) const;

	const StdMeshMaterialShader* GetFragmentShader() const { return FragmentShader; }
	const StdMeshMaterialShader* GetVertexShader() const { return VertexShader; }
	const StdMeshMaterialShader* GetGeometryShader() const { return GeometryShader; }
private:
	bool CompileShader(StdMeshMaterialLoader& loader, C4Shader& shader, int ssc);

	// Human-readable program name
	const StdCopyStrBuf Name;

	// Program components
	const StdMeshMaterialShader* FragmentShader;
	const StdMeshMaterialShader* VertexShader;
	const StdMeshMaterialShader* GeometryShader;

	// Compiled shaders
	C4Shader Shader;
	C4Shader ShaderMod2;
	C4Shader ShaderLight;
	C4Shader ShaderLightMod2;

	// Filled as program references are encountered;
	std::vector<StdCopyStrBuf> ParameterNames;
};

class StdMeshMaterialTextureUnit
{
public:
	enum TexAddressModeType
	{
		AM_Wrap,
		AM_Clamp,
		AM_Mirror,
		AM_Border
	};

	enum FilteringType
	{
		F_None,
		F_Point,
		F_Linear,
		F_Anisotropic
	};

	enum BlendOpType
	{
		BO_Replace,
		BO_Add,
		BO_Modulate,
		BO_AlphaBlend
	};

	enum BlendOpExType
	{
		BOX_Source1,
		BOX_Source2,
		BOX_Modulate,
		BOX_ModulateX2,
		BOX_ModulateX4,
		BOX_Add,
		BOX_AddSigned,
		BOX_AddSmooth,
		BOX_Subtract,
		BOX_BlendDiffuseAlpha,
		BOX_BlendTextureAlpha,
		BOX_BlendCurrentAlpha,
		BOX_BlendManual,
		BOX_Dotproduct,
		BOX_BlendDiffuseColor
	};

	enum BlendOpSourceType
	{
		BOS_Current,
		BOS_Texture,
		BOS_Diffuse,
		BOS_Specular,
		BOS_PlayerColor, // not specified in ogre, added in OpenClonk
		BOS_Manual
	};

	struct Transformation
	{
		enum Type
		{
			T_SCROLL,
			T_SCROLL_ANIM,
			T_ROTATE,
			T_ROTATE_ANIM,
			T_SCALE,
			T_TRANSFORM,
			T_WAVE_XFORM
		};

		enum XFormType
		{
			XF_SCROLL_X,
			XF_SCROLL_Y,
			XF_ROTATE,
			XF_SCALE_X,
			XF_SCALE_Y
		};

		enum WaveType
		{
			W_SINE,
			W_TRIANGLE,
			W_SQUARE,
			W_SAWTOOTH,
			W_INVERSE_SAWTOOTH
		};

		Type TransformType;

		union
		{
			struct { float X; float Y; } Scroll;
			struct { float XSpeed; float YSpeed; } ScrollAnim;
			struct { float Angle; } Rotate;
			struct { float RevsPerSec; } RotateAnim;
			struct { float X; float Y; } Scale;
			struct { float M[16]; } Transform;
			struct { XFormType XForm; WaveType Wave; float Base; float Frequency; float Phase; float Amplitude; } WaveXForm;
		};

		double GetScrollX(double t) const { assert(TransformType == T_SCROLL_ANIM); return ScrollAnim.XSpeed * t; }
		double GetScrollY(double t) const { assert(TransformType == T_SCROLL_ANIM); return ScrollAnim.YSpeed * t; }
		double GetRotate(double t) const { assert(TransformType == T_ROTATE_ANIM); return fmod(RotateAnim.RevsPerSec * t, 1.0) * 360.0; }
		double GetWaveXForm(double t) const;
	};

	// Ref-counted texture. When a meterial inherits from one which contains
	// a TextureUnit, then they will share the same C4TexRef.
	class Tex
	{
	public:
		Tex(C4Surface* Surface); // Takes ownership
		~Tex();

		unsigned int RefCount;

		// TODO: Note this cannot be C4Surface here, because C4Surface
		// does not have a virtual destructor, so we couldn't delete it
		// properly in that case. I am a bit annoyed that this
		// currently requires a cross-ref to lib/texture. I think
		// C4Surface should go away and the file loading/saving
		// should be free functions instead. I also think the file
		// loading/saving should be decoupled from the surfaces, so we
		// can skip the surface here and simply use a C4TexRef. armin.
		C4Surface* Surf;
		C4TexRef& Texture;
	};

	// Simple wrapper which handles refcounting of Tex
	class TexPtr
	{
	public:
		TexPtr(C4Surface* Surface);
		TexPtr(const TexPtr& other);
		~TexPtr();
		
		TexPtr& operator=(const TexPtr& other);
		
		Tex* pTex;
	};

	StdMeshMaterialTextureUnit();

	void LoadTexture(StdMeshMaterialParserCtx& ctx, const char* texname);
	void Load(StdMeshMaterialParserCtx& ctx);

	bool HasTexture() const { return !Textures.empty(); }
	size_t GetNumTextures() const { return Textures.size(); }
	const C4TexRef& GetTexture(unsigned int i) const { return Textures[i].pTex->Texture; }
	bool HasFrameAnimation() const { return Duration > 0; }
	bool HasTexCoordAnimation() const { return !Transformations.empty(); }

	StdCopyStrBuf Name;
	float Duration; // Duration of texture animation, if any.

	TexAddressModeType TexAddressMode;
	float TexBorderColor[4];
	FilteringType Filtering[3]; // min, max, mipmap

	BlendOpExType ColorOpEx;
	BlendOpSourceType ColorOpSources[2];
	float ColorOpManualFactor;
	float ColorOpManualColor1[3];
	float ColorOpManualColor2[3];

	BlendOpExType AlphaOpEx;
	BlendOpSourceType AlphaOpSources[2];
	float AlphaOpManualFactor;
	float AlphaOpManualAlpha1;
	float AlphaOpManualAlpha2;

	// Transformations to be applied to texture coordinates in order
	std::vector<Transformation> Transformations;

private:
	std::vector<TexPtr> Textures;
};

class StdMeshMaterialPass
{
public:
	enum CullHardwareType
	{
		CH_Clockwise,
		CH_CounterClockwise,
		CH_None
	};

	enum SceneBlendType
	{
		SB_One,
		SB_Zero,
		SB_DestColor,
		SB_SrcColor,
		SB_OneMinusDestColor,
		SB_OneMinusSrcColor,
		SB_DestAlpha,
		SB_SrcAlpha,
		SB_OneMinusDestAlpha,
		SB_OneMinusSrcAlpha
	};

	enum DepthFunctionType
	{
		DF_AlwaysFail,
		DF_AlwaysPass,
		DF_Less,
		DF_LessEqual,
		DF_Equal,
		DF_NotEqual,
		DF_GreaterEqual,
		DF_Greater
	};

	StdMeshMaterialPass();
	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const { return SceneBlendFactors[1] == SB_Zero; }

	StdCopyStrBuf Name;
	std::vector<StdMeshMaterialTextureUnit> TextureUnits;

	float Ambient[4];
	float Diffuse[4];
	float Specular[4];
	float Emissive[4];
	float Shininess;

	bool DepthCheck;
	bool DepthWrite;

	CullHardwareType CullHardware;
	SceneBlendType SceneBlendFactors[2];
	DepthFunctionType AlphaRejectionFunction;
	float AlphaRejectionValue;
	bool AlphaToCoverage;

	struct ShaderInstance
	{
		// This points into the StdMeshMatManager maps
		const StdMeshMaterialShader* Shader;
		// Parameters for this instance
		StdMeshMaterialShaderParameters Parameters;
	};

	class ProgramInstance
	{
	public:
		ProgramInstance(const StdMeshMaterialProgram* program, const ShaderInstance* fragment_instance, const ShaderInstance* vertex_instance, const ShaderInstance* geometry_instance);

		// This points into the StdMeshMatManager map
		const StdMeshMaterialProgram* const Program;

		// Parameters for this instance
		struct ParameterRef {
			const StdMeshMaterialShaderParameter* Parameter;
			int UniformIndex; // Index into parameter table for this program
		};

		std::vector<ParameterRef> Parameters;

	private:
		void LoadParameterRefs(const ShaderInstance* instance);
	};

	ShaderInstance FragmentShader;
	ShaderInstance VertexShader;
	ShaderInstance GeometryShader;

	// This is a shared_ptr and not a unique_ptr so that this class is
	// copyable, so it can be inherited. However, when the inherited
	// material is prepared, the ProgramInstance will be overwritten
	// anyway, so in that sense a unique_ptr would be enough. We could
	// change it and make this class only movable (not copyable), and
	// provide inheritance by copying all other fields, and letting
	// PrepareMaterial fill the program instance.
	std::shared_ptr<ProgramInstance> Program;

private:
	void LoadShaderRef(StdMeshMaterialParserCtx& ctx, StdMeshMaterialShaderType type);
};

class StdMeshMaterialTechnique
{
public:
	StdMeshMaterialTechnique();

	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const;

	StdCopyStrBuf Name;
	std::vector<StdMeshMaterialPass> Passes;

	// Filled in by gfx implementation: Whether this technique is available on
	// the hardware and gfx engine (DX/GL) we are running on
	bool Available;
};

class StdMeshMaterial
{
public:
	StdMeshMaterial();
	void Load(StdMeshMaterialParserCtx& ctx);

	bool IsOpaque() const { assert(BestTechniqueIndex >= 0); return Techniques[BestTechniqueIndex].IsOpaque(); }

	// Location the Material was loaded from
	StdCopyStrBuf FileName;
	unsigned int Line;

	// Material name
	StdCopyStrBuf Name;

	// Not currently used in Clonk, but don't fail when we see this in a
	// Material script:
	bool ReceiveShadows;

	// Available techniques
	std::vector<StdMeshMaterialTechnique> Techniques;

	// Filled in by gfx implementation: Best technique to use
	int BestTechniqueIndex; // Don't use a pointer into the Technique vector to save us from implementing a copyctor
};

class StdMeshMatManager
{
	friend class StdMeshMaterialUpdate;
private:
	typedef std::map<StdCopyStrBuf, StdMeshMaterial> MaterialMap;

public:
	enum ShaderLoadFlag {
		SMM_AcceptExisting = 1,
		SMM_ForceReload = 2
	};

	class Iterator
	{
		friend class StdMeshMatManager;
	public:
		Iterator(const MaterialMap::iterator& iter): iter_(iter) {}
		Iterator(const Iterator& iter): iter_(iter.iter_) {}

		Iterator operator=(const Iterator& iter) { iter_ = iter.iter_; return *this; }
		Iterator& operator++() { ++iter_; return *this; }
		bool operator==(const Iterator& other) const { return iter_ == other.iter_; }
		bool operator!=(const Iterator& other) const { return iter_ != other.iter_; }

		const StdMeshMaterial& operator*() const { return iter_->second; }
		const StdMeshMaterial* operator->() const { return &iter_->second; }
	private:
		MaterialMap::iterator iter_;
	};

	// Remove all materials from manager. Make sure there is no StdMesh
	// referencing any out there before calling this.
	void Clear();

	// Parse a material script file, and add the materials to the manager.
	// filename may be nullptr if the source is not a file. It will only be used
	// for error messages.
	// Throws StdMeshMaterialError.
	// Returns a set of all loaded materials.
	std::set<StdCopyStrBuf> Parse(const char* mat_script, const char* filename, StdMeshMaterialLoader& loader);

	// Get material by name. nullptr if there is no such material with this name.
	const StdMeshMaterial* GetMaterial(const char* material_name) const;

	Iterator Begin() { return Iterator(Materials.begin()); }
	Iterator End() { return Iterator(Materials.end()); }
	void Remove(const StdStrBuf& name, class StdMeshMaterialUpdate* update);
	Iterator Remove(const Iterator& iter, class StdMeshMaterialUpdate* update);

	const StdMeshMaterialShader* AddShader(const char* filename, const char* name, const char* language, StdMeshMaterialShaderType type, const char* text, uint32_t load_flags); // if load_flags & SMM_AcceptExisting, the function returns the existing shader, otherwise returns nullptr.
	const StdMeshMaterialProgram* AddProgram(const char* name, StdMeshMaterialLoader& loader, const StdMeshMaterialPass::ShaderInstance& fragment_shader, const StdMeshMaterialPass::ShaderInstance& vertex_shader, const StdMeshMaterialPass::ShaderInstance& geometry_shader); // returns nullptr if shader code cannot be compiled

	const StdMeshMaterialShader* GetFragmentShader(const char* name) const;
	const StdMeshMaterialShader* GetVertexShader(const char* name) const;
	const StdMeshMaterialShader* GetGeometryShader(const char* name) const;
private:
	MaterialMap Materials;

	// Shader code for custom shaders.
	typedef std::map<StdCopyStrBuf, std::unique_ptr<StdMeshMaterialShader>> ShaderMap;
	ShaderMap FragmentShaders;
	ShaderMap VertexShaders;
	ShaderMap GeometryShaders;

	// Linked programs
	typedef std::map<std::tuple<const StdMeshMaterialShader*, const StdMeshMaterialShader*, const StdMeshMaterialShader*>, std::unique_ptr<StdMeshMaterialProgram> > ProgramMap;
	ProgramMap Programs;
};

extern StdMeshMatManager MeshMaterialManager;

#endif
