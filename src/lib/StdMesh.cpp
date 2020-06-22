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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "lib/StdMesh.h"

#include "graphics/C4DrawGL.h"

namespace
{
	struct StdMeshFaceOrderHelper
	{
		float z;
		unsigned int i;
	};
}

static int StdMeshFaceCmp(const StdMeshFaceOrderHelper& h1, const StdMeshFaceOrderHelper& h2)
{
	if(h1.z < h2.z) return -1;
	else if(h1.z > h2.z) return +1;
	return 0;
}

#define SORT_NAME StdMesh
#define SORT_TYPE StdMeshFaceOrderHelper
#define SORT_CMP StdMeshFaceCmp
#include "timsort/sort.h"

std::vector<StdMeshInstance::SerializableValueProvider::IDBase*>* StdMeshInstance::SerializableValueProvider::IDs = nullptr;

namespace
{
#ifndef USE_CONSOLE
	// Helper to sort submeshes so that opaque ones appear before non-opaque ones
	struct StdMeshSubMeshVisibilityCmpPred
	{
		bool operator()(const StdSubMesh& first, const StdSubMesh& second)
		{
			return first.GetMaterial().IsOpaque() > second.GetMaterial().IsOpaque();
		}
	};

	// Helper to sort submesh instances so that opaque ones appear before non-opaque ones,
	// this is required if materials are changed with SetMeshMaterial.
	struct StdMeshSubMeshInstanceVisibilityCmpPred
	{
		bool operator()(const StdSubMeshInstance* first, const StdSubMeshInstance* second)
		{
			return first->GetMaterial().IsOpaque() > second->GetMaterial().IsOpaque();
		}
	};

	float StdMeshFaceOrderGetVertexZ(const StdMeshVertex& vtx, const StdMeshMatrix& trans)
	{
		// TODO: Need to apply attach matrix in case of attached meshes

		// We need to evaluate the Z coordinate of the transformed vertex
		// (for all three vertices of the two faces), something like
		// float z11 = (trans*m_vertices[face1.Vertices[0]]).z;
		// However we don't do the full matrix multiplication as we are
		// only interested in the Z coordinate of the result, also we are
		// not interested in the resulting normals.
		return trans(2,0)*vtx.x + trans(2,1)*vtx.y + trans(2,2)*vtx.z + trans(2,3);
	}

	float StdMeshFaceOrderGetFaceZ(const StdMeshVertex* vertices, const StdMeshFace& face, const StdMeshMatrix& trans)
	{
		const float z1 = StdMeshFaceOrderGetVertexZ(vertices[face.Vertices[0]], trans);
		const float z2 = StdMeshFaceOrderGetVertexZ(vertices[face.Vertices[1]], trans);
		const float z3 = StdMeshFaceOrderGetVertexZ(vertices[face.Vertices[2]], trans);
		return std::max(std::max(z1, z2), z3);
	}

	void SortFacesArray(const StdMeshVertex* vertices, std::vector<StdMeshFace>& faces, StdSubMeshInstance::FaceOrdering face_ordering, const StdMeshMatrix& trans)
	{
		if(faces.empty()) return;

		std::vector<StdMeshFaceOrderHelper> helpers(faces.size());
		for(unsigned int i = 0; i < faces.size(); ++i)
		{
			helpers[i].i = i;
			helpers[i].z = StdMeshFaceOrderGetFaceZ(vertices, faces[i], trans);
		}

		// The reason to use timsort here instead of std::sort is for performance
		// reasons. This is performance critical code, with this function being
		// called at least once per frame for each semi-transparent object. I have
		// measured a factor 7 difference between the two sorting algorithms on my
		// system.

		// We also pre-compute the Z values that we use for sorting, and sort the
		// array of Z values, then use the resorted indices to sort the original
		// faces array. The reason for this is twofold:
		// 1. We don't need to compute the Z value every time the comparison function
		//    is called. Even though the computation is not very expensive, we have
		//    to do many comparisons, and small things add up. I have measured a
		//    5-10% performance benefit.
		// 2. More importantly, due to floating point rounding errors we cannot guarantee
		//    that Z values computed in the sorting function always yield the exact same
		//    number, and the same sorting result for the same faces. This can lead to
		//    a crash, because the f(a1, a2) = -f(a2, a1) property for the sorting function
		//    would no longer be met, resulting in undefined behaviour in the sort call.
		//    See http://bugs.openclonk.org/view.php?id=984.
		StdMesh_tim_sort(&helpers[0], helpers.size());

		std::vector<StdMeshFace> new_faces(faces.size());
		switch(face_ordering)
		{
		case StdSubMeshInstance::FO_Fixed:
			assert(false);
			break;
		case StdSubMeshInstance::FO_FarthestToNearest:
			for(unsigned int i = 0; i < faces.size(); ++i)
				new_faces[i] = faces[helpers[i].i];
			break;
		case StdSubMeshInstance::FO_NearestToFarthest:
			for(unsigned int i = 0; i < faces.size(); ++i)
				new_faces[i] = faces[helpers[faces.size() - i - 1].i];
			break;
		default:
			assert(false);
			break;
		}

		faces.swap(new_faces);
	}
#endif

	// Serialize a ValueProvider with StdCompiler
	struct ValueProviderAdapt
	{
		ValueProviderAdapt(StdMeshInstance::ValueProvider** Provider):
				ValueProvider(Provider) {}

		StdMeshInstance::ValueProvider** ValueProvider;

		void CompileFunc(StdCompiler* pComp)
		{
			const StdMeshInstance::SerializableValueProvider::IDBase* id;
			StdMeshInstance::SerializableValueProvider* svp = nullptr;

			if(pComp->isDeserializer())
			{
				StdCopyStrBuf id_str;
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));

				id = StdMeshInstance::SerializableValueProvider::Lookup(id_str.getData());
				if(!id) pComp->excCorrupt(R"(No value provider for ID "%s")", id_str.getData());
			}
			else
			{
				svp = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(*ValueProvider);
				if(!svp) pComp->excCorrupt("Value provider cannot be compiled");
				id = StdMeshInstance::SerializableValueProvider::Lookup(typeid(*svp));
				if(!id) pComp->excCorrupt("No ID for value provider registered");

				StdCopyStrBuf id_str(id->name);
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));
			}

			pComp->Separator(StdCompiler::SEP_START);
			pComp->Value(mkContextPtrAdapt(svp, *id, false));
			pComp->Separator(StdCompiler::SEP_END);

			if(pComp->isDeserializer())
				*ValueProvider = svp;
		}
	};

	ValueProviderAdapt mkValueProviderAdapt(StdMeshInstance::ValueProvider** ValueProvider) { return ValueProviderAdapt(ValueProvider); }

	void CompileFloat(StdCompiler* pComp, float& f)
	{
		// TODO: Teach StdCompiler how to handle float
		if(pComp->isDeserializer())
		{
			C4Real r;
			pComp->Value(r);
			f = fixtof(r);
		}
		else
		{
			C4Real r = ftofix(f);
			pComp->Value(r);
		}
	}

	// Serialize a transformation matrix by name with StdCompiler
	struct MatrixAdapt
	{
		StdMeshMatrix& Matrix;
		MatrixAdapt(StdMeshMatrix& matrix): Matrix(matrix) {}

		void CompileFunc(StdCompiler* pComp)
		{
			pComp->Separator(StdCompiler::SEP_START);
			for(unsigned int i = 0; i < 3; ++i)
			{
				for(unsigned int j = 0; j < 4; ++j)
				{
					if(i != 0 || j != 0) pComp->Separator();
					CompileFloat(pComp, Matrix(i, j));
				}
			}

			pComp->Separator(StdCompiler::SEP_END);
		}
	};

	struct TransformAdapt
	{
		StdMeshTransformation& Trans;
		TransformAdapt(StdMeshTransformation& trans): Trans(trans) {}

		void CompileFunc(StdCompiler* pComp)
		{
			pComp->Separator(StdCompiler::SEP_START);
			CompileFloat(pComp, Trans.translate.x);
			CompileFloat(pComp, Trans.translate.y);
			CompileFloat(pComp, Trans.translate.z);
			CompileFloat(pComp, Trans.rotate.w);
			CompileFloat(pComp, Trans.rotate.x);
			CompileFloat(pComp, Trans.rotate.y);
			CompileFloat(pComp, Trans.rotate.z);
			CompileFloat(pComp, Trans.scale.x);
			CompileFloat(pComp, Trans.scale.y);
			CompileFloat(pComp, Trans.scale.z);
			pComp->Separator(StdCompiler::SEP_END);
		}
	};

	MatrixAdapt mkMatrixAdapt(StdMeshMatrix& Matrix) { return MatrixAdapt(Matrix); }
	TransformAdapt mkTransformAdapt(StdMeshTransformation& Trans) { return TransformAdapt(Trans); }

	// Reset all animation list entries corresponding to node or its children
	void ClearAnimationListRecursively(std::vector<StdMeshInstance::AnimationNode*>& list, StdMeshInstance::AnimationNode* node)
	{
		list[node->GetNumber()] = nullptr;

		if (node->GetType() == StdMeshInstance::AnimationNode::LinearInterpolationNode)
		{
			ClearAnimationListRecursively(list, node->GetLeftChild());
			ClearAnimationListRecursively(list, node->GetRightChild());
		}
	}

	// Mirror is wrt Z axis
	void MirrorKeyFrame(StdMeshKeyFrame& frame, const StdMeshTransformation& old_bone_transformation, const StdMeshTransformation& new_inverse_bone_transformation)
	{
		// frame was a keyframe of a track for old_bone and was now transplanted to new_bone.
		frame.Transformation.rotate.x = -frame.Transformation.rotate.x;
		frame.Transformation.rotate.y = -frame.Transformation.rotate.y;

		StdMeshVector d = old_bone_transformation.scale * (old_bone_transformation.rotate * frame.Transformation.translate);
		d.z = -d.z;
		frame.Transformation.translate = new_inverse_bone_transformation.rotate * (new_inverse_bone_transformation.scale * d);

		// TODO: scale
	}

	bool MirrorName(StdStrBuf& buf)
	{
		unsigned int len = buf.getLength();

		if(buf.Compare_(".R", len-2) == 0)
			buf.getMData()[len-1] = 'L';
		else if(buf.Compare_(".L", len-2) == 0)
			buf.getMData()[len-1] = 'R';
		else
			return false;

		return true;
	}
}

StdMeshTransformation StdMeshTrack::GetTransformAt(float time, float length) const
{
	std::map<float, StdMeshKeyFrame>::const_iterator iter = Frames.lower_bound(time);

	// We are at or before the first keyframe. This short typically not
	// happen, since all animations have a keyframe 0. Simply return the
	// first keyframe.
	if (iter == Frames.begin())
		return iter->second.Transformation;

	std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
	--prev_iter;

	float iter_pos;
	if (iter == Frames.end())
	{
		// We are beyond the last keyframe.
		// Interpolate between the last and the first keyframe.
		// See also bug #1406.
		iter = Frames.begin();
		iter_pos = length;
	}
	else
	{
		iter_pos = iter->first;
	}

	// No two keyframes with the same position:
	assert(iter_pos > prev_iter->first);

	// Requested position is between the two selected keyframes:
	assert(time >= prev_iter->first);
	assert(iter_pos >= time);

	float dt = iter_pos - prev_iter->first;
	float weight1 = (time - prev_iter->first) / dt;
	float weight2 = (iter_pos - time) / dt;
	(void)weight2; // used in assertion only

	assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
	assert(fabs(weight1 + weight2 - 1) < 1e-6);

	return StdMeshTransformation::Nlerp(prev_iter->second.Transformation, iter->second.Transformation, weight1);
}

StdMeshAnimation::StdMeshAnimation(const StdMeshAnimation& other):
		Name(other.Name), Length(other.Length), Tracks(other.Tracks.size())
{
	// Note that all Tracks are already default-initialized to zero
	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);

	OriginSkeleton = other.OriginSkeleton;
}

StdMeshAnimation::~StdMeshAnimation()
{
	for (auto & Track : Tracks)
		delete Track;
}

StdMeshAnimation& StdMeshAnimation::operator=(const StdMeshAnimation& other)
{
	if (this == &other) return *this;

	Name = other.Name;
	Length = other.Length;

	for (auto & Track : Tracks)
		delete Track;

	Tracks.resize(other.Tracks.size());

	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);

	return *this;
}

StdMeshSkeleton::StdMeshSkeleton() = default;

StdMeshSkeleton::~StdMeshSkeleton()
{
	for (auto & Bone : Bones)
		delete Bone;
}

void StdMeshSkeleton::AddMasterBone(StdMeshBone *bone)
{
	bone->Index = Bones.size(); // Remember index in master bone table
	Bones.push_back(bone);
	for (auto & i : bone->Children)
		AddMasterBone(i);
}

const StdMeshBone* StdMeshSkeleton::GetBoneByName(const StdStrBuf& name) const
{
	// Lookup parent bone
	for (const auto & Bone : Bones)
		if (Bone->Name == name)
			return Bone;

	return nullptr;
}

const StdMeshAnimation* StdMeshSkeleton::GetAnimationByName(const StdStrBuf& name) const
{
	StdCopyStrBuf name2(name);
	std::map<StdCopyStrBuf, StdMeshAnimation>::const_iterator iter = Animations.find(name2);
	if (iter == Animations.end()) return nullptr;
	return &iter->second;
}

std::vector<const StdMeshAnimation*> StdMeshSkeleton::GetAnimations() const
{
	std::vector<const StdMeshAnimation*> result;
	result.reserve(Animations.size());
	for (const auto & Animation : Animations)
		result.push_back(&Animation.second);
	return result;
}

void StdMeshSkeleton::MirrorAnimation(const StdMeshAnimation& animation)
{
	StdCopyStrBuf name(animation.Name);

	// do nothing if the name cannot be switched from *.L to *.R or vice versa
	// or if the animation already exists
	if (!MirrorName(name) || Animations.find(name) != Animations.end())
	{
		return;
	}

	StdMeshAnimation& new_anim = Animations.insert(std::make_pair(name, animation)).first->second;
	new_anim.Name = name;

	// Go through all bones
	for (unsigned int i = 0; i < GetNumBones(); ++i)
	{
		const StdMeshBone& bone = GetBone(i);
		StdCopyStrBuf other_bone_name(bone.Name);
		if (MirrorName(other_bone_name))
		{
			const StdMeshBone* other_bone = GetBoneByName(other_bone_name);
			if (!other_bone)
				throw std::runtime_error(std::string("No counterpart for bone ") + bone.Name.getData() + " found");

			// Make sure to not swap tracks twice
			if ((animation.Tracks[i] != nullptr || animation.Tracks[other_bone->Index] != nullptr) &&
				other_bone->Index > bone.Index)
			{
				std::swap(new_anim.Tracks[i], new_anim.Tracks[other_bone->Index]);

				StdMeshTransformation own_trans = bone.GetParent()->InverseTransformation * bone.Transformation;
				StdMeshTransformation other_own_trans = other_bone->GetParent()->InverseTransformation * other_bone->Transformation;

				// Mirror all the keyframes of both tracks
				if (new_anim.Tracks[i] != nullptr)
					for (auto & Frame : new_anim.Tracks[i]->Frames)
						MirrorKeyFrame(Frame.second, own_trans, StdMeshTransformation::Inverse(other_own_trans));

				if (new_anim.Tracks[other_bone->Index] != nullptr)
					for (auto & Frame : new_anim.Tracks[other_bone->Index]->Frames)
						MirrorKeyFrame(Frame.second, other_own_trans, StdMeshTransformation::Inverse(own_trans));
			}
		}
		else if (bone.Name.Compare_(".N", bone.Name.getLength() - 2) != 0)
		{
			if (new_anim.Tracks[i] != nullptr)
			{
				StdMeshTransformation own_trans = bone.Transformation;
				if (bone.GetParent()) own_trans = bone.GetParent()->InverseTransformation * bone.Transformation;

				for (auto & Frame : new_anim.Tracks[i]->Frames)
					MirrorKeyFrame(Frame.second, own_trans, StdMeshTransformation::Inverse(own_trans));
			}
		}
	}
}

void StdMeshSkeleton::InsertAnimation(const StdMeshAnimation& animation)
{
	assert(Animations.find(animation.Name) == Animations.end());

	Animations.insert(std::make_pair(animation.Name, animation));
}

void StdMeshSkeleton::InsertAnimation(const StdMeshSkeleton& source, const StdMeshAnimation& animation)
{
	assert(Animations.find(animation.Name) == Animations.end());

	// get matching bones from the source
	std::vector<int> bone_index_source = source.GetMatchingBones(*this);

	// create a new animation and copy the basic data from the other animation
	StdMeshAnimation anim;
	anim.Name = animation.Name;
	anim.Length = animation.Length;
	anim.Tracks.resize(GetNumBones());
	anim.OriginSkeleton = &source;

	// sort the tracks according to the matched bones
	for (unsigned int i = 0; i < anim.Tracks.size(); ++i)
	{
		if (bone_index_source[i] > -1 && animation.Tracks[bone_index_source[i]] != nullptr)
		{
			anim.Tracks[i] = new StdMeshTrack(*animation.Tracks[bone_index_source[i]]);
		}
	}

	// and add it to the map
	Animations.insert(std::make_pair(animation.Name, anim));
}

void StdMeshSkeleton::PostInit()
{
	// Mirror .R and .L animations without counterpart
	for (auto & Animation : Animations)
	{
		// For debugging purposes:
		//		if(iter->second.Name == "Jump")
		//			MirrorAnimation(StdCopyStrBuf("Jump.Mirror"), iter->second);

		// mirrors only if necessary
		MirrorAnimation(Animation.second);
	}
}

std::vector<int> StdMeshSkeleton::GetMatchingBones(const StdMeshSkeleton& child_skeleton) const
{
	std::vector<int> MatchedBoneInParentSkeleton;

	// find matching bones names in both skeletons
	for (unsigned int i = 0; i < child_skeleton.GetNumBones(); ++i)
	{
		int parent_bone_index = -1; // instantiate with invalid index == no match

		for (unsigned int j = 0; j < GetNumBones(); ++j)
		{
			// start searching at the same index
			int sample_index = (i + j) % GetNumBones();

			if (GetBone(sample_index).Name == child_skeleton.GetBone(i).Name)
			{
				parent_bone_index = sample_index;
				break;
			}
		}

		// add valid or invalid mapped index to list of mapped bones
		MatchedBoneInParentSkeleton.push_back(parent_bone_index);
	}

	return MatchedBoneInParentSkeleton;
}

StdMesh::StdMesh() :
	Skeleton(new StdMeshSkeleton)
{
	BoundingBox.x1 = BoundingBox.y1 = BoundingBox.z1 = 0.0f;
	BoundingBox.x2 = BoundingBox.y2 = BoundingBox.z2 = 0.0f;
	BoundingRadius = 0.0f;
}

StdMesh::~StdMesh()
{
#ifndef USE_CONSOLE
	if (ibo)
		glDeleteBuffers(1, &ibo);
	if (vbo)
		glDeleteBuffers(1, &vbo);
	if (vaoid)
		pGL->FreeVAOID(vaoid);
#endif
}

void StdMesh::PostInit()
{
#ifndef USE_CONSOLE
	// Order submeshes so that opaque submeshes come before non-opaque ones
	std::sort(SubMeshes.begin(), SubMeshes.end(), StdMeshSubMeshVisibilityCmpPred());
	UpdateVBO();
	UpdateIBO();

	// Allocate a VAO ID as well
	assert(vaoid == 0);
	vaoid = pGL->GenVAOID();
#endif
}

#ifndef USE_CONSOLE
void StdMesh::UpdateVBO()
{
	// We're only uploading vertices once, so there shouldn't be a VBO so far
	assert(vbo == 0);
	if (vbo != 0)
		glDeleteBuffers(1, &vbo);
	glGenBuffers(1, &vbo);

	// Calculate total number of vertices
	size_t total_vertices = SharedVertices.size();
	for (auto &submesh : SubMeshes)
	{
		total_vertices += submesh.GetNumVertices();
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	pGL->ObjectLabel(GL_BUFFER, vbo, -1, (Label + "/VBO").c_str());

	// Unmapping the buffer may fail for certain reasons, in which case we need to try again.
	do
	{
		// Allocate VBO backing memory. If this mesh's skeleton has no animations
		// defined, we assume that the VBO will not change frequently.
		glBufferData(GL_ARRAY_BUFFER, total_vertices * sizeof(StdMeshVertex), nullptr, GL_STATIC_DRAW);
		void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		uint8_t *buffer = static_cast<uint8_t*>(map);
		uint8_t *cursor = buffer;

		// Add shared vertices to buffer
		if (!SharedVertices.empty())
		{
			size_t shared_vertices_size = SharedVertices.size() * sizeof(SharedVertices[0]);
			std::memcpy(cursor, &SharedVertices[0], shared_vertices_size);
			cursor += shared_vertices_size;
		}

		// Add all submeshes to buffer
		for (auto &submesh : SubMeshes)
		{
			// Store the offset, so the render code can use it later
			submesh.vertex_buffer_offset = cursor - buffer;

			if (submesh.Vertices.empty()) continue;
			size_t vertices_size = sizeof(submesh.Vertices[0]) * submesh.Vertices.size();
			std::memcpy(cursor, &submesh.Vertices[0], vertices_size);
			cursor += vertices_size;
		}
	} while (glUnmapBuffer(GL_ARRAY_BUFFER) == GL_FALSE);
	// Unbind the buffer so following rendering calls do not use it
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void StdMesh::UpdateIBO()
{
	assert(ibo == 0);
	if (ibo != 0)
		glDeleteBuffers(1, &ibo);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	pGL->ObjectLabel(GL_BUFFER, ibo, -1, (Label + "/IBO").c_str());

	size_t total_faces = 0;
	for (auto &submesh : SubMeshes)
		total_faces += submesh.GetNumFaces();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_faces * 3 * sizeof(GLuint), nullptr, GL_STATIC_DRAW);
	size_t offset = 0;
	for (auto &submesh : SubMeshes)
	{
		submesh.index_buffer_offset = offset * 3 * sizeof(GLuint);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, submesh.index_buffer_offset, submesh.GetNumFaces() * 3 * sizeof(GLuint), &submesh.Faces[0]);
		offset += submesh.GetNumFaces();
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
#endif

StdSubMeshInstance::StdSubMeshInstance(StdMeshInstance& instance, const StdSubMesh& submesh, float completion):
	base(&submesh), Material(nullptr), CurrentFaceOrdering(FO_Fixed)
{
#ifndef USE_CONSOLE
	LoadFacesForCompletion(instance, submesh, completion);
#endif

	SetMaterial(submesh.GetMaterial());
}

void StdSubMeshInstance::LoadFacesForCompletion(StdMeshInstance& instance, const StdSubMesh& submesh, float completion)
{
#ifndef USE_CONSOLE
	// First: Copy all faces
	Faces.resize(submesh.GetNumFaces());
	for (unsigned int i = 0; i < submesh.GetNumFaces(); ++i)
		Faces[i] = submesh.GetFace(i);

	if(completion < 1.0f)
	{
		// Second: Order by Y position. StdMeshInstanceFaceOrderingCmpPred orders by Z position,
		// however we can simply give an appropriate transformation matrix to the face ordering.
		// At this point, all vertices are in the OGRE coordinate frame, and Z in OGRE equals
		// Y in Clonk, so we are fine without additional transformation.
		const StdMeshVertex* vertices;
		if(submesh.GetNumVertices() > 0)
			vertices = &submesh.GetVertices()[0];
		else
			vertices = &instance.GetSharedVertices()[0];
		SortFacesArray(vertices, Faces, FO_FarthestToNearest, StdMeshMatrix::Identity());

		// Third: Only use the first few ones
		assert(submesh.GetNumFaces() >= 1);
		Faces.resize(Clamp<unsigned int>(static_cast<unsigned int>(completion * submesh.GetNumFaces() + 0.5), 1, submesh.GetNumFaces()));
	}
#endif
}

void StdSubMeshInstance::SetMaterial(const StdMeshMaterial& material)
{
	Material = &material;

#ifndef USE_CONSOLE
	// Setup initial texture animation data
	assert(Material->BestTechniqueIndex >= 0);
	const StdMeshMaterialTechnique& technique = Material->Techniques[Material->BestTechniqueIndex];
	PassData.resize(technique.Passes.size());
	for (unsigned int i = 0; i < PassData.size(); ++i)
	{
		const StdMeshMaterialPass& pass = technique.Passes[i];
		// Clear from previous material
		PassData[i].TexUnits.clear();

		for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
		{
			TexUnit unit;
			unit.Phase = 0;
			unit.PhaseDelay = 0.0f;
			unit.Position = 0.0;
			PassData[i].TexUnits.push_back(unit);
		}
	}

	// TODO: Reset face ordering
#endif
}

void StdSubMeshInstance::SetFaceOrdering(StdMeshInstance& instance, const StdSubMesh& submesh, FaceOrdering ordering)
{
#ifndef USE_CONSOLE
	if (CurrentFaceOrdering != ordering)
	{
		CurrentFaceOrdering = ordering;
		if (ordering == FO_Fixed)
		{
			LoadFacesForCompletion(instance, submesh, instance.GetCompletion());
		}
	}
#endif
}

void StdSubMeshInstance::SetFaceOrderingForClrModulation(StdMeshInstance& instance, const StdSubMesh& submesh, uint32_t clrmod)
{
#ifndef USE_CONSOLE
	bool opaque = Material->IsOpaque();

	if(!opaque)
		SetFaceOrdering(instance, submesh, FO_FarthestToNearest);
	else if( ((clrmod >> 24) & 0xff) != 0xff)
		SetFaceOrdering(instance, submesh, FO_NearestToFarthest);
	else
		SetFaceOrdering(instance, submesh, FO_Fixed);
#endif
}

void StdSubMeshInstance::CompileFunc(StdCompiler* pComp)
{
	// TODO: We should also serialize the texture animation positions
	if(pComp->isDeserializer())
	{
		StdCopyStrBuf material_name;
		pComp->Value(mkNamingAdapt(material_name, "Material"));
		if(material_name != Material->Name)
		{
			const StdMeshMaterial* material = ::MeshMaterialManager.GetMaterial(material_name.getData());
			if(!material)
			{
				StdStrBuf buf;
				buf.Format(R"(There is no such material with name "%s")", material_name.getData());
				pComp->excCorrupt(buf.getData());
			}

			SetMaterial(*material);
		}
	}
	else
	{
		StdCopyStrBuf material_name = Material->Name;
		pComp->Value(mkNamingAdapt(material_name, "Material"));
	}
}

void StdMeshInstance::SerializableValueProvider::CompileFunc(StdCompiler* pComp)
{
	pComp->Value(Value);
}

StdMeshInstanceAnimationNode::StdMeshInstanceAnimationNode() 
{
	Leaf.Animation = nullptr;
	Leaf.Position = nullptr;
}

StdMeshInstanceAnimationNode::StdMeshInstanceAnimationNode(const StdMeshAnimation* animation, ValueProvider* position)
{
	Leaf.Animation = animation;
	Leaf.Position = position;
}

StdMeshInstanceAnimationNode::StdMeshInstanceAnimationNode(const StdMeshBone* bone, const StdMeshTransformation& trans):
		Type(CustomNode), Parent(nullptr)
{
	Custom.BoneIndex = bone->Index;
	Custom.Transformation = new StdMeshTransformation(trans);
}

StdMeshInstanceAnimationNode::StdMeshInstanceAnimationNode(AnimationNode* child_left, AnimationNode* child_right, ValueProvider* weight):
		Type(LinearInterpolationNode), Parent(nullptr)
{
	LinearInterpolation.ChildLeft = child_left;
	LinearInterpolation.ChildRight = child_right;
	LinearInterpolation.Weight = weight;
}

StdMeshInstanceAnimationNode::~StdMeshInstanceAnimationNode()
{
	switch (Type)
	{
	case LeafNode:
		delete Leaf.Position;
		break;
	case CustomNode:
		delete Custom.Transformation;
		break;
	case LinearInterpolationNode:
		delete LinearInterpolation.ChildLeft;
		delete LinearInterpolation.ChildRight;
		delete LinearInterpolation.Weight;
		break;
	}
}

bool StdMeshInstanceAnimationNode::GetBoneTransform(unsigned int bone, StdMeshTransformation& transformation)
{
	StdMeshTransformation combine_with;
	StdMeshTrack* track;

	switch (Type)
	{
	case LeafNode:
		track = Leaf.Animation->Tracks[bone];
		if (!track) return false;
		transformation = track->GetTransformAt(fixtof(Leaf.Position->Value), Leaf.Animation->Length);
		return true;
	case CustomNode:
		if(bone == Custom.BoneIndex)
			transformation = *Custom.Transformation;
		else
			return false;
		return true;
	case LinearInterpolationNode:
		if (!LinearInterpolation.ChildLeft->GetBoneTransform(bone, transformation))
			return LinearInterpolation.ChildRight->GetBoneTransform(bone, transformation);
		if (!LinearInterpolation.ChildRight->GetBoneTransform(bone, combine_with))
			return true; // First Child affects bone

		transformation = StdMeshTransformation::Nlerp(transformation, combine_with, fixtof(LinearInterpolation.Weight->Value));
		return true;
	default:
		assert(false);
		return false;
	}
}

void StdMeshInstanceAnimationNode::CompileFunc(StdCompiler* pComp, const StdMesh *Mesh)
{
	static const StdEnumEntry<NodeType> NodeTypes[] =
	{
		{ "Leaf",                  LeafNode                      },
		{ "Custom",                CustomNode                    },
		{ "LinearInterpolation",   LinearInterpolationNode       },

		{ nullptr,     static_cast<NodeType>(0)  }
	};

	pComp->Value(mkNamingAdapt(Slot, "Slot"));
	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(Type, NodeTypes), "Type"));

	switch(Type)
	{
	case LeafNode:
		if(pComp->isDeserializer())
		{
			StdCopyStrBuf anim_name;
			pComp->Value(mkNamingAdapt(toC4CStrBuf(anim_name), "Animation"));
			Leaf.Animation = Mesh->GetSkeleton().GetAnimationByName(anim_name);
			if(!Leaf.Animation) pComp->excCorrupt(R"(No such animation: "%s")", anim_name.getData());
		}
		else
		{
			pComp->Value(mkNamingAdapt(mkParAdapt(mkDecompileAdapt(Leaf.Animation->Name), StdCompiler::RCT_All), "Animation"));
		}

		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&Leaf.Position), "Position"));
		break;
	case CustomNode:
		if(pComp->isDeserializer())
		{
			StdCopyStrBuf bone_name;
			pComp->Value(mkNamingAdapt(toC4CStrBuf(bone_name), "Bone"));
			const StdMeshBone* bone = Mesh->GetSkeleton().GetBoneByName(bone_name);
			if(!bone) pComp->excCorrupt(R"(No such bone: "%s")", bone_name.getData());
			Custom.BoneIndex = bone->Index;
			Custom.Transformation = new StdMeshTransformation;
		}
		else
		{
			pComp->Value(mkNamingAdapt(mkParAdapt(mkDecompileAdapt(Mesh->GetSkeleton().GetBone(Custom.BoneIndex).Name), StdCompiler::RCT_All), "Bone"));
		}

		pComp->Value(mkNamingAdapt(mkTransformAdapt(*Custom.Transformation), "Transformation"));
		break;
	case LinearInterpolationNode:
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildLeft, "ChildLeft"), Mesh));
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildRight, "ChildRight"), Mesh));
		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&LinearInterpolation.Weight), "Weight"));
		if(pComp->isDeserializer())
		{
			if(LinearInterpolation.ChildLeft->Slot != Slot)
				pComp->excCorrupt("Slot of left child does not match parent slot");
			if(LinearInterpolation.ChildRight->Slot != Slot)
				pComp->excCorrupt("Slof of right child does not match parent slot");
			LinearInterpolation.ChildLeft->Parent = this;
			LinearInterpolation.ChildRight->Parent = this;
		}
		break;
	default:
		pComp->excCorrupt("Invalid animation node type");
		break;
	}
}

void StdMeshInstanceAnimationNode::DenumeratePointers()
{
	StdMeshInstance::SerializableValueProvider* value_provider = nullptr;
	switch(Type)
	{
	case LeafNode:
		value_provider = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(Leaf.Position);
		break;
	case CustomNode:
		value_provider = nullptr;
		break;
	case LinearInterpolationNode:
		value_provider = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(LinearInterpolation.Weight);
		// non-recursive, StdMeshInstance::DenumeratePointers walks over all nodes
		break;
	}

	if(value_provider) value_provider->DenumeratePointers();
}

void StdMeshInstanceAnimationNode::ClearPointers(class C4Object* pObj)
{
	StdMeshInstance::SerializableValueProvider* value_provider = nullptr;
	switch(Type)
	{
	case LeafNode:
		value_provider = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(Leaf.Position);
		break;
	case CustomNode:
		value_provider = nullptr;
		break;
	case LinearInterpolationNode:
		value_provider = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(LinearInterpolation.Weight);
		// non-recursive, StdMeshInstance::ClearPointers walks over all nodes
		break;
	}

	if(value_provider) value_provider->ClearPointers(pObj);
}

StdMeshInstance::AttachedMesh::AttachedMesh() = default;

StdMeshInstance::AttachedMesh::AttachedMesh(unsigned int number, StdMeshInstance* parent, StdMeshInstance* child, bool own_child, Denumerator* denumerator,
		unsigned int parent_bone, unsigned int child_bone, const StdMeshMatrix& transform, uint32_t flags):
		Number(number), Parent(parent), Child(child), OwnChild(own_child), ChildDenumerator(denumerator),
		ParentBone(parent_bone), ChildBone(child_bone), AttachTrans(transform), Flags(flags),
		FinalTransformDirty(true)
{
		MapBonesOfChildToParent(parent->GetMesh().GetSkeleton(), child->GetMesh().GetSkeleton());
}

StdMeshInstance::AttachedMesh::~AttachedMesh()
{
	if (OwnChild)
		delete Child;
	delete ChildDenumerator;
}

bool StdMeshInstance::AttachedMesh::SetParentBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Parent->GetMesh().GetSkeleton().GetBoneByName(bone);
	if (!bone_obj) return false;
	ParentBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

bool StdMeshInstance::AttachedMesh::SetChildBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Child->GetMesh().GetSkeleton().GetBoneByName(bone);
	if (!bone_obj) return false;
	ChildBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

void StdMeshInstance::AttachedMesh::SetAttachTransformation(const StdMeshMatrix& transformation)
{
	AttachTrans = transformation;
	FinalTransformDirty = true;
}

void StdMeshInstance::AttachedMesh::CompileFunc(StdCompiler* pComp, DenumeratorFactoryFunc Factory)
{
	if(pComp->isDeserializer())
	{
		FinalTransformDirty = true;
		ChildDenumerator = Factory();
	}

	const StdBitfieldEntry<uint8_t> AM_Entries[] =
	{
		{ "MatchSkeleton", AM_MatchSkeleton },
		{ "DrawBefore",    AM_DrawBefore },
		{ nullptr,            0 }
	};

	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(ParentBone, "ParentBone")); // TODO: Save as string
	pComp->Value(mkNamingAdapt(ChildBone, "ChildBone")); // TODO: Save as string (note we can only resolve this in DenumeratePointers then!)
	pComp->Value(mkNamingAdapt(mkMatrixAdapt(AttachTrans), "AttachTransformation"));

	uint8_t dwSyncFlags = static_cast<uint8_t>(Flags);
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(dwSyncFlags, AM_Entries), "Flags", 0u));
	if(pComp->isDeserializer()) Flags = dwSyncFlags;

	pComp->Value(mkParAdapt(*ChildDenumerator, this));
}

void StdMeshInstance::AttachedMesh::DenumeratePointers()
{
	ChildDenumerator->DenumeratePointers(this);

	assert(Child != nullptr);
	Child->AttachParent = this;

	MapBonesOfChildToParent(Parent->GetMesh().GetSkeleton(), Child->GetMesh().GetSkeleton());

	if(OwnChild)
		Child->DenumeratePointers();
}

bool StdMeshInstance::AttachedMesh::ClearPointers(class C4Object* pObj)
{
	return ChildDenumerator->ClearPointers(pObj);
}

void StdMeshInstance::AttachedMesh::MapBonesOfChildToParent(const StdMeshSkeleton& parent_skeleton, const StdMeshSkeleton& child_skeleton)
{
	// not necessary if we do not match anyway.
	if (!(Flags & AM_MatchSkeleton)) return;

	// clear array to avoid filling it twice
	MatchedBoneInParentSkeleton.clear();
	MatchedBoneInParentSkeleton = parent_skeleton.GetMatchingBones(child_skeleton);
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh, float completion):
		Mesh(&mesh), Completion(completion),
		BoneTransforms(Mesh->GetSkeleton().GetNumBones(), StdMeshMatrix::Identity()),
		SubMeshInstances(Mesh->GetNumSubMeshes()), AttachParent(nullptr),
		BoneTransformsDirty(false)
#ifndef USE_CONSOLE
		, ibo(0), vaoid(0)
#endif
{
	// Create submesh instances
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
	{
		const StdSubMesh& submesh = Mesh->GetSubMesh(i);
		SubMeshInstances[i] = new StdSubMeshInstance(*this, submesh, completion);
	}

	// copy, order is fine at the moment since only default materials are used.
	SubMeshInstancesOrdered = SubMeshInstances;
}

StdMeshInstance::~StdMeshInstance()
{
#ifndef USE_CONSOLE
	if (ibo) glDeleteBuffers(1, &ibo);
	if (vaoid) pGL->FreeVAOID(vaoid);
#endif

	// If we are attached then detach from parent
	if (AttachParent)
		AttachParent->Parent->DetachMesh(AttachParent->Number);

	// Remove all attach children
	while (!AttachChildren.empty())
		DetachMesh(AttachChildren.back()->Number);

	while (!AnimationStack.empty())
		StopAnimation(AnimationStack.front());
	assert(AnimationNodes.empty());

	// Delete submeshes
	for (auto & SubMeshInstance : SubMeshInstances)
	{
		delete SubMeshInstance;
	}
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
#ifndef USE_CONSOLE
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->SetFaceOrdering(*this, Mesh->GetSubMesh(i), ordering);

	// Faces have been reordered: upload new order to GPU
	UpdateIBO();

	// Update attachments (only own meshes for now... others might be displayed both attached and non-attached...)
	// still not optimal.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->OwnChild)
			(*iter)->Child->SetFaceOrdering(ordering);
#endif
}

void StdMeshInstance::SetFaceOrderingForClrModulation(uint32_t clrmod)
{
#ifndef USE_CONSOLE
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->SetFaceOrderingForClrModulation(*this, Mesh->GetSubMesh(i), clrmod);

	// Faces have been reordered: upload new order to GPU
	UpdateIBO();

	// Update attachments (only own meshes for now... others might be displayed both attached and non-attached...)
	// still not optimal.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->OwnChild)
			(*iter)->Child->SetFaceOrderingForClrModulation(clrmod);
#endif
}

void StdMeshInstance::SetCompletion(float completion)
{
	Completion = completion;

#ifndef USE_CONSOLE
	// TODO: Load all submesh faces and then determine the ones to use from the
	// full pool.
	for(unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->LoadFacesForCompletion(*this, Mesh->GetSubMesh(i), completion);

	// Faces have been reordered: upload new order to GPU
	UpdateIBO();
#endif
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight, bool stop_previous_animation)
{
	const StdMeshAnimation* animation = Mesh->GetSkeleton().GetAnimationByName(animation_name);
	if (!animation) { delete position; delete weight; return nullptr; }

	return PlayAnimation(*animation, slot, sibling, position, weight, stop_previous_animation);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight, bool stop_previous_animation)
{
	position->Value = Clamp(position->Value, Fix0, ftofix(animation.Length));
	AnimationNode* child = new AnimationNode(&animation, position);
	InsertAnimationNode(child, slot, sibling, weight, stop_previous_animation);
	return child;
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdMeshBone* bone, const StdMeshTransformation& trans, int slot, AnimationNode* sibling, ValueProvider* weight, bool stop_previous_animation)
{
	AnimationNode* child = new AnimationNode(bone, trans);
	InsertAnimationNode(child, slot, sibling, weight, stop_previous_animation);
	return child;
}

void StdMeshInstance::StopAnimation(AnimationNode* node)
{
	ClearAnimationListRecursively(AnimationNodes, node);

	AnimationNode* parent = node->Parent;
	if (parent == nullptr)
	{
		AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
		assert(iter != AnimationStack.end() && *iter == node);
		AnimationStack.erase(iter);
		delete node;
	}
	else
	{
		assert(parent->Type == AnimationNode::LinearInterpolationNode);

		// Remove parent interpolation node and re-link
		AnimationNode* other_child;
		if (parent->LinearInterpolation.ChildLeft == node)
		{
			other_child = parent->LinearInterpolation.ChildRight;
			parent->LinearInterpolation.ChildRight = nullptr;
		}
		else
		{
			other_child = parent->LinearInterpolation.ChildLeft;
			parent->LinearInterpolation.ChildLeft = nullptr;
		}

		if (parent->Parent)
		{
			assert(parent->Parent->Type == AnimationNode::LinearInterpolationNode);
			if (parent->Parent->LinearInterpolation.ChildLeft == parent)
				parent->Parent->LinearInterpolation.ChildLeft = other_child;
			else
				parent->Parent->LinearInterpolation.ChildRight = other_child;
			other_child->Parent = parent->Parent;
		}
		else
		{
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
			assert(iter != AnimationStack.end() && *iter == parent);
			*iter = other_child;

			other_child->Parent = nullptr;
		}

		AnimationNodes[parent->Number] = nullptr;
		// Recursively deletes parent and its descendants
		delete parent;
	}

	while (!AnimationNodes.empty() && AnimationNodes.back() == nullptr)
		AnimationNodes.erase(AnimationNodes.end()-1);
	SetBoneTransformsDirty(true);
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetAnimationNodeByNumber(unsigned int number)
{
	if (number >= AnimationNodes.size()) return nullptr;
	return AnimationNodes[number];
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetRootAnimationForSlot(int slot)
{
	AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
	if (iter == AnimationStack.end()) return nullptr;
	return *iter;
}

void StdMeshInstance::SetAnimationPosition(AnimationNode* node, ValueProvider* position)
{
	assert(node->GetType() == AnimationNode::LeafNode);
	delete node->Leaf.Position;
	node->Leaf.Position = position;

	position->Value = Clamp(position->Value, Fix0, ftofix(node->Leaf.Animation->Length));

	SetBoneTransformsDirty(true);
}

void StdMeshInstance::SetAnimationBoneTransform(AnimationNode* node, const StdMeshTransformation& trans)
{
	assert(node->GetType() == AnimationNode::CustomNode);
	*node->Custom.Transformation = trans;
	SetBoneTransformsDirty(true);
}

void StdMeshInstance::SetAnimationWeight(AnimationNode* node, ValueProvider* weight)
{
	assert(node->GetType() == AnimationNode::LinearInterpolationNode);
	delete node->LinearInterpolation.Weight; node->LinearInterpolation.Weight = weight;

	weight->Value = Clamp(weight->Value, Fix0, itofix(1));

	SetBoneTransformsDirty(true);
}

void StdMeshInstance::ExecuteAnimation(float dt)
{
	// Iterate from the back since slots might be removed
	for (unsigned int i = AnimationStack.size(); i > 0; --i)
		if(!ExecuteAnimationNode(AnimationStack[i-1]))
			StopAnimation(AnimationStack[i-1]);

#ifndef USE_CONSOLE
	// Update animated textures
	for (auto & SubMeshInstance : SubMeshInstances)
	{
		StdSubMeshInstance& submesh = *SubMeshInstance;
		const StdMeshMaterial& material = submesh.GetMaterial();
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];
		for (unsigned int j = 0; j < submesh.PassData.size(); ++j)
		{
			StdSubMeshInstance::Pass& pass = submesh.PassData[j];
			for (unsigned int k = 0; k < pass.TexUnits.size(); ++k)
			{
				const StdMeshMaterialTextureUnit& texunit = technique.Passes[j].TextureUnits[k];
				StdSubMeshInstance::TexUnit& texunit_instance = submesh.PassData[j].TexUnits[k];
				if (texunit.HasFrameAnimation())
				{
					const unsigned int NumPhases = texunit.GetNumTextures();
					const float PhaseDuration = texunit.Duration / NumPhases;

					const float Position = texunit_instance.PhaseDelay + dt;
					const unsigned int AddPhases = static_cast<unsigned int>(Position / PhaseDuration);

					texunit_instance.Phase = (texunit_instance.Phase + AddPhases) % NumPhases;
					texunit_instance.PhaseDelay = Position - AddPhases * PhaseDuration;
				}

				if (texunit.HasTexCoordAnimation())
					texunit_instance.Position += dt;
			}
		}
	}
#endif

	// Update animation for attached meshes
	for (auto & iter : AttachChildren)
		iter->Child->ExecuteAnimation(dt);
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(const StdMesh& mesh, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags, unsigned int attach_number)
{
	std::unique_ptr<AttachedMesh::Denumerator> auto_denumerator(denumerator);

	StdMeshInstance* instance = new StdMeshInstance(mesh, 1.0f);
	AttachedMesh* attach = AttachMesh(*instance, auto_denumerator.release(), parent_bone, child_bone, transformation, flags, true, attach_number);
	if (!attach) { delete instance; return nullptr; }
	return attach;
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags, bool own_child, unsigned int attach_number)
{
	std::unique_ptr<AttachedMesh::Denumerator> auto_denumerator(denumerator);

	// Owned attach children must be set via the topmost instance, to ensure
	// attach number uniqueness.
	if (AttachParent && AttachParent->OwnChild) return nullptr;

	// Find free index.
	unsigned int number = 0;
	ScanAttachTree(AttachChildren.begin(), AttachChildren.end(), [&number](AttachedMeshList::const_iterator iter) { number = std::max(number, (*iter)->Number); return true; });
	number += 1; // One above highest

	StdMeshInstance* direct_parent = this;
	if (attach_number != 0)
	{
		AttachedMesh* attach = GetAttachedMeshByNumber(attach_number);
		if (attach == nullptr) return nullptr;
		direct_parent = attach->Child;
	}

	return direct_parent->AttachMeshImpl(instance, auto_denumerator.release(), parent_bone, child_bone, transformation, flags, own_child, number);
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMeshImpl(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags, bool own_child, unsigned int new_attach_number)
{
	std::unique_ptr<AttachedMesh::Denumerator> auto_denumerator(denumerator);

	// We don't allow an instance to be attached to multiple parent instances for now
	if (instance.AttachParent) return nullptr;

	// Make sure there are no cyclic attachments
	for (StdMeshInstance* Parent = this; Parent->AttachParent != nullptr; Parent = Parent->AttachParent->Parent)
		if (Parent == &instance)
			return nullptr;

	const StdMeshBone* parent_bone_obj = Mesh->GetSkeleton().GetBoneByName(parent_bone);
	const StdMeshBone* child_bone_obj = instance.Mesh->GetSkeleton().GetBoneByName(child_bone);
	if (!parent_bone_obj || !child_bone_obj) return nullptr;

	AttachedMesh* attach = new AttachedMesh(new_attach_number, this, &instance, own_child, auto_denumerator.release(), parent_bone_obj->Index, child_bone_obj->Index, transformation, flags);
	instance.AttachParent = attach;

	// If DrawInFront is set then sort before others so that drawing order is easy
	if(flags & AM_DrawBefore)
		AttachChildren.insert(AttachChildren.begin(), attach);
	else
		AttachChildren.insert(AttachChildren.end(), attach);

	return attach;
}

bool StdMeshInstance::DetachMesh(unsigned int number)
{
	return !ScanAttachTree(AttachChildren.begin(), AttachChildren.end(), [number](AttachedMeshList::iterator iter)
	{
		if ((*iter)->Number == number)
		{
			AttachedMesh* attached = *iter;

			// Reset attach parent of child so it does not try
			// to detach itself on destruction.
			attached->Child->AttachParent = nullptr;
			attached->Parent->AttachChildren.erase(iter);

			delete attached;

			// Finish scan
			return false;
		}

		// Continue scan
		return true;
	});
}

StdMeshInstance::AttachedMesh* StdMeshInstance::GetAttachedMeshByNumber(unsigned int number) const
{
	StdMeshInstance::AttachedMesh* result = nullptr;

	ScanAttachTree(AttachChildren.begin(), AttachChildren.end(), [number, &result](AttachedMeshList::const_iterator iter)
	{
		if ((*iter)->Number == number)
		{
			result = *iter;
			return false;
		}

		return true;
	});

	return result;
}

void StdMeshInstance::SetMaterial(size_t i, const StdMeshMaterial& material)
{
	assert(i < SubMeshInstances.size());
	SubMeshInstances[i]->SetMaterial(material);
#ifndef USE_CONSOLE
	std::stable_sort(SubMeshInstancesOrdered.begin(), SubMeshInstancesOrdered.end(), StdMeshSubMeshInstanceVisibilityCmpPred());
#endif
}

const StdMeshMatrix& StdMeshInstance::GetBoneTransform(size_t i) const
{
	if ((AttachParent != nullptr) && (AttachParent->GetFlags() & AM_MatchSkeleton))
	{
		assert(i < AttachParent->MatchedBoneInParentSkeleton.size());

		int parent_bone_index = AttachParent->MatchedBoneInParentSkeleton[i];

		if (parent_bone_index > -1)
		{
			return AttachParent->Parent->BoneTransforms[i];
		}
	}

	return BoneTransforms[i];
}

size_t StdMeshInstance::GetBoneCount() const
{
	if ((AttachParent != nullptr) && (AttachParent->GetFlags() & AM_MatchSkeleton))
		return AttachParent->MatchedBoneInParentSkeleton.size();
	else
		return BoneTransforms.size();
}

bool StdMeshInstance::UpdateBoneTransforms()
{
	bool was_dirty = BoneTransformsDirty;

	// Nothing changed since last time
	if (BoneTransformsDirty)
	{
		// Compute transformation matrix for each bone.
		for (unsigned int i = 0; i < BoneTransforms.size(); ++i)
		{
			StdMeshTransformation Transformation;

			const StdMeshBone& bone = Mesh->GetSkeleton().GetBone(i);
			const StdMeshBone* parent = bone.GetParent();
			assert(!parent || parent->Index < i);

			bool have_transform = false;
			for (auto & j : AnimationStack)
			{
				if (have_transform)
				{
					StdMeshTransformation other;
					if (j->GetBoneTransform(i, other))
						Transformation = StdMeshTransformation::Nlerp(Transformation, other, 1.0f); // TODO: Allow custom weighing for slot combination
				}
				else
				{
					have_transform = j->GetBoneTransform(i, Transformation);
				}
			}

			if (!have_transform)
			{
				if (parent)
					BoneTransforms[i] = BoneTransforms[parent->Index];
				else
					BoneTransforms[i] = StdMeshMatrix::Identity();
			}
			else
			{
				BoneTransforms[i] = StdMeshMatrix::Transform(bone.Transformation * Transformation * bone.InverseTransformation);
				if (parent) BoneTransforms[i] = BoneTransforms[parent->Index] * BoneTransforms[i];
			}
		}
	}

	// Update attachment's attach transformations. Note this is done recursively.
	for (auto attach : AttachChildren)
	{
		const bool ChildBoneTransformsDirty = attach->Child->BoneTransformsDirty;
		attach->Child->UpdateBoneTransforms();

		if (BoneTransformsDirty || ChildBoneTransformsDirty || attach->FinalTransformDirty)
		{
			was_dirty = true;

			// Compute matrix to change the coordinate system to the one of the attached bone:
			// The idea is that a vertex at the child bone's position transforms to the parent bone's position.
			// Therefore (read from right to left) we first apply the inverse of the child bone transformation,
			// then an optional scaling matrix, and finally the parent bone transformation

			// TODO: we can cache the three matrices in the middle since they don't change over time,
			// reducing this to two matrix multiplications instead of four each frame.
			// Might even be worth to compute the complete transformation directly when rendering then
			// (saves per-instance memory, but requires recomputation if the animation does not change).
			// TODO: We might also be able to cache child inverse, and only recomupte it if
			// child bone transforms are dirty (saves matrix inversion for unanimated attach children).
			attach->FinalTrans = GetBoneTransform(attach->ParentBone)
				* StdMeshMatrix::Transform(Mesh->GetSkeleton().GetBone(attach->ParentBone).Transformation)
				* attach->AttachTrans
				* StdMeshMatrix::Transform(attach->Child->Mesh->GetSkeleton().GetBone(attach->ChildBone).InverseTransformation)
				* StdMeshMatrix::Inverse(attach->Child->GetBoneTransform(attach->ChildBone));

			attach->FinalTransformDirty = false;
		}
	}

	SetBoneTransformsDirty(false);
	return was_dirty;
}

void StdMeshInstance::ReorderFaces(StdMeshMatrix* global_trans)
{
#ifndef USE_CONSOLE
	for (auto & SubMeshInstance : SubMeshInstances)
	{
		StdSubMeshInstance& inst = *SubMeshInstance;
		assert((inst.Faces.size() > 0) && "StdMeshInstance sub-mesh instance has zero faces");

		if(inst.Faces.size() > 0 && inst.CurrentFaceOrdering != StdSubMeshInstance::FO_Fixed)
		{
			const StdMeshVertex* vertices;
			if(inst.GetSubMesh().GetNumVertices() > 0)
				vertices = &inst.GetSubMesh().GetVertices()[0];
			else
				vertices = &GetSharedVertices()[0];
			SortFacesArray(vertices, inst.Faces, inst.CurrentFaceOrdering, global_trans ? *global_trans : StdMeshMatrix::Identity());
		}
	}

	// TODO: Also reorder submeshes, attached meshes and include AttachTransformation for attached meshes...

	// Faces have been reordered: upload new order to GPU
	UpdateIBO();
#endif
}

void StdMeshInstance::CompileFunc(StdCompiler* pComp, AttachedMesh::DenumeratorFactoryFunc Factory)
{
	if(pComp->isDeserializer())
	{
		// Only initially created instances can be compiled
		assert(!AttachParent);
		assert(AttachChildren.empty());
		assert(AnimationStack.empty());
		SetBoneTransformsDirty(true);

		bool valid;
		pComp->Value(mkNamingAdapt(valid, "Valid"));
		if(!valid) pComp->excCorrupt("Mesh instance is invalid");

		int32_t iSubMeshCnt;
		pComp->Value(mkNamingCountAdapt(iSubMeshCnt, "SubMesh"));
		if(static_cast<uint32_t>(iSubMeshCnt) != SubMeshInstances.size())
			pComp->excCorrupt("Invalid number of submeshes");
		for(int32_t i = 0; i < iSubMeshCnt; ++i)
			pComp->Value(mkNamingAdapt(*SubMeshInstances[i], "SubMesh"));
#ifndef USE_CONSOLE
		// The sorting predicate depends on having a gfx implementation.
		std::stable_sort(SubMeshInstancesOrdered.begin(), SubMeshInstancesOrdered.end(), StdMeshSubMeshInstanceVisibilityCmpPred());
#endif

		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(int32_t i = 0; i < iAnimCnt; ++i)
		{
			AnimationNode* node = nullptr;
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(node, "AnimationNode"), Mesh));
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, true);
			if(*iter != nullptr) { delete node; pComp->excCorrupt("Duplicate animation slot index"); }
			*iter = node;

			// Add nodes into lookup table
			std::vector<AnimationNode*> nodes(1, node);
			while(!nodes.empty())
			{
				node = nodes.back();
				nodes.erase(nodes.end()-1);

				if (AnimationNodes.size() <= node->Number)
					AnimationNodes.resize(node->Number+1);
				if(AnimationNodes[node->Number] != nullptr) pComp->excCorrupt("Duplicate animation node number");
				AnimationNodes[node->Number] = node;

				if(node->Type == AnimationNode::LinearInterpolationNode)
				{
					nodes.push_back(node->LinearInterpolation.ChildLeft);
					nodes.push_back(node->LinearInterpolation.ChildRight);
				}
			}
		}

		int32_t iAttachedCnt;
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));

		for(int32_t i = 0; i < iAttachedCnt; ++i)
		{
			AttachChildren.push_back(new AttachedMesh);
			AttachedMesh* attach = AttachChildren.back();

			attach->Parent = this;
			pComp->Value(mkNamingAdapt(mkParAdapt(*attach, Factory), "Attached"));
		}
	}
	else
	{
		// Write something to make sure that the parent
		// named section ([Mesh] or [ChildInstance]) is written.
		// StdCompilerIni does not make a difference between
		// non-existing and empty named sections.
		bool valid = true;
		pComp->Value(mkNamingAdapt(valid, "Valid"));

		int32_t iSubMeshCnt = SubMeshInstances.size();
		pComp->Value(mkNamingCountAdapt(iSubMeshCnt, "SubMesh"));
		for(int32_t i = 0; i < iSubMeshCnt; ++i)
			pComp->Value(mkNamingAdapt(*SubMeshInstances[i], "SubMesh"));

		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(auto & iter : AnimationStack)
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(iter, "AnimationNode"), Mesh));

		int32_t iAttachedCnt = AttachChildren.size();
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));
		
		for(auto & i : AttachChildren)
			pComp->Value(mkNamingAdapt(mkParAdapt(*i, Factory), "Attached"));
	}
}

void StdMeshInstance::DenumeratePointers()
{
	for(auto & AnimationNode : AnimationNodes)
		if(AnimationNode)
			AnimationNode->DenumeratePointers();

	for(auto & i : AttachChildren)
	{
		i->DenumeratePointers();
	}
}

void StdMeshInstance::ClearPointers(class C4Object* pObj)
{
	for(auto & AnimationNode : AnimationNodes)
		if(AnimationNode)
			AnimationNode->ClearPointers(pObj);

	std::vector<unsigned int> Removal;
	for(auto & i : AttachChildren)
		if(!i->ClearPointers(pObj))
			Removal.push_back(i->Number);

	for(unsigned int i : Removal)
		DetachMesh(i);
}

template<typename IteratorType, typename FuncObj>
bool StdMeshInstance::ScanAttachTree(IteratorType begin, IteratorType end, const FuncObj& obj)
{
	for (IteratorType iter = begin; iter != end; ++iter)
	{
		if (!obj(iter)) return false;

		// Scan attached tree of own children. For non-owned children,
		// we can't guarantee unique attach numbers.
		if( (*iter)->OwnChild)
			if (!ScanAttachTree((*iter)->Child->AttachChildren.begin(), (*iter)->Child->AttachChildren.end(), obj))
				return false;
	}

	return true;
}

StdMeshInstance::AnimationNodeList::iterator StdMeshInstance::GetStackIterForSlot(int slot, bool create)
{
	// TODO: bsearch
	for (AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
	{
		if ((*iter)->Slot == slot)
		{
			return iter;
		}
		else if ((*iter)->Slot > slot)
		{
			if (!create)
				return AnimationStack.end();
			else
				return AnimationStack.insert(iter, nullptr);
		}
	}

	if (!create)
		return AnimationStack.end();
	else
		return AnimationStack.insert(AnimationStack.end(), nullptr);
}

void StdMeshInstance::InsertAnimationNode(AnimationNode* node, int slot, AnimationNode* sibling, ValueProvider* weight, bool stop_previous_animation)
{
	assert(!sibling || !stop_previous_animation);
	// Default
	if (!sibling) sibling = GetRootAnimationForSlot(slot);
	assert(!sibling || sibling->Slot == slot);
	
	// Stop any animation already running in this slot?
	if (sibling && stop_previous_animation)
	{
		StopAnimation(sibling);
		sibling = nullptr;
	}

	// Find two subsequent numbers in case we need to create two nodes, so
	// script can deduce the second node.
	unsigned int Number1, Number2;
	for (Number1 = 0; Number1 < AnimationNodes.size(); ++Number1)
		if (AnimationNodes[Number1] == nullptr && (!sibling || Number1+1 == AnimationNodes.size() || AnimationNodes[Number1+1] == nullptr))
			break;
	/*  for(Number2 = Number1+1; Number2 < AnimationNodes.size(); ++Number2)
	    if(AnimationNodes[Number2] == nullptr)
	      break;*/
	Number2 = Number1 + 1;

	weight->Value = Clamp(weight->Value, Fix0, itofix(1));

	if (Number1 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) nullptr);
	if (sibling && Number2 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) nullptr);

	AnimationNodes[Number1] = node;
	node->Number = Number1;
	node->Slot = slot;

	if (sibling)
	{
		AnimationNode* parent = new AnimationNode(node, sibling, weight);
		AnimationNodes[Number2] = parent;
		parent->Number = Number2;
		parent->Slot = slot;

		node->Parent = parent;
		parent->Parent = sibling->Parent;
		parent->LinearInterpolation.ChildLeft = sibling;
		parent->LinearInterpolation.ChildRight = node;
		if (sibling->Parent)
		{
			if (sibling->Parent->LinearInterpolation.ChildLeft == sibling)
				sibling->Parent->LinearInterpolation.ChildLeft = parent;
			else
				sibling->Parent->LinearInterpolation.ChildRight = parent;
		}
		else
		{
			// set new parent
			AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
			// slot must not be empty, since sibling uses same slot
			assert(iter != AnimationStack.end() && *iter != nullptr);
			*iter = parent;
		}

		sibling->Parent = parent;
	}
	else
	{
		delete weight;
		AnimationNodeList::iterator iter = GetStackIterForSlot(slot, true);
		assert(!*iter); // we have a sibling if slot is not empty
		*iter = node;
	}

	SetBoneTransformsDirty(true);
}

bool StdMeshInstance::ExecuteAnimationNode(AnimationNode* node)
{
	ValueProvider* provider = nullptr;
	C4Real min;
	C4Real max;

	switch (node->GetType())
	{
	case AnimationNode::LeafNode:
		provider = node->GetPositionProvider();
		min = Fix0;
		max = ftofix(node->GetAnimation()->Length);
		break;
	case AnimationNode::CustomNode:
		// No execution necessary
		return true;
	case AnimationNode::LinearInterpolationNode:
		provider = node->GetWeightProvider();
		min = Fix0;
		max = itofix(1);
		break;
	default:
		assert(false);
		break;
	}

	assert(provider);
	const C4Real old_value = provider->Value;

	if (!provider->Execute())
	{
		if (node->GetType() == AnimationNode::LeafNode) return false;

		// Remove the child with less weight (normally weight reaches 0.0 or 1.0)
		if (node->GetWeight() > itofix(1, 2))
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetRightChild())) return false;
			// Remove left child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildLeft);
		}
		else
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetLeftChild())) return false;
			// Remove right child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildRight);
		}
	}
	else
	{
		if (provider->Value != old_value)
		{
			provider->Value = Clamp(provider->Value, min, max);
			SetBoneTransformsDirty(true);
		}

		if (node->GetType() == AnimationNode::LinearInterpolationNode)
		{
			const bool left_result = ExecuteAnimationNode(node->GetLeftChild());
			const bool right_result = ExecuteAnimationNode(node->GetRightChild());

			// Remove this node completely
			if (!left_result && !right_result)
				return false;

			// Note that either of this also removes node
			if (!left_result)
				StopAnimation(node->GetLeftChild());
			if (!right_result)
				StopAnimation(node->GetRightChild());
		}
	}

	return true;
}

void StdMeshInstance::SetBoneTransformsDirty(bool value)
{
	BoneTransformsDirty = value;

	// only if the value is true, so that updates happen
	if (value)
	{
		// Update attachment's attach transformations. Note this is done recursively.
		for (auto attach : AttachChildren)
		{
			if (attach->GetFlags() & AM_MatchSkeleton)
			{
				attach->Child->SetBoneTransformsDirty(value);
			}
		}
	}
}

#ifndef USE_CONSOLE
void StdMeshInstance::UpdateIBO()
{
	// First, find out whether we have fixed face ordering or not
	bool all_submeshes_fixed = true;
	for (StdSubMeshInstance* inst : SubMeshInstances)
	{
		all_submeshes_fixed = (inst->GetFaceOrdering() == StdSubMeshInstance::FO_Fixed);
		if (!all_submeshes_fixed) break;

		// If true, submesh is 100% complete
		all_submeshes_fixed = inst->GetNumFaces() == inst->GetSubMesh().GetNumFaces();
		if (!all_submeshes_fixed) break;
	}

	// If the face ordering is fixed, then we don't need a custom
	// IBO. This is typically the case for all meshes without transparency
	// and 100% completion.
	if (all_submeshes_fixed)
	{
		if (ibo) glDeleteBuffers(1, &ibo);
		if (vaoid) pGL->FreeVAOID(vaoid);
		ibo = 0; vaoid = 0;
	}
	else
	{
		// We have a custom face ordering, or we render only a subset
		// of our faces. Create a custom IBO and upload the index
		// data.
		if (ibo == 0)
		{
			// This is required, because the IBO binding is part
			// of the VAO state. If we create a new IBO we cannot
			// keep using any old VAO. But we always create and 
			// destroy them together, so we can assert here.
			assert(vaoid == 0);

			size_t total_faces = 0;
			for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
				total_faces += Mesh->GetSubMesh(i).GetNumFaces();

			glGenBuffers(1, &ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

			// TODO: Optimize mode. In many cases this is still fairly static.
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_faces * 3 * sizeof(GLuint), nullptr, GL_STREAM_DRAW);
		}
		else
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		}

		for (StdSubMeshInstance* inst : SubMeshInstances)
		{
			assert(inst->GetNumFaces() <= inst->GetSubMesh().GetNumFaces());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, inst->GetSubMesh().GetOffsetInIBO(), inst->GetNumFaces() * 3 * sizeof(GLuint), &inst->Faces[0]);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (vaoid == 0)
			vaoid = pGL->GenVAOID();
	}
}
#endif
