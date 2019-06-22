#pragma once
#include "GRiInclude.h"




struct GProjectTextureInfo
{
	std::wstring UniqueFileName;
	bool bSrgb;

private:

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(UniqueFileName);
		ar & BOOST_SERIALIZATION_NVP(bSrgb);
	}
};

struct GProjectSceneObjectInfo
{
	std::wstring UniqueName = L"none";
	//std::wstring MaterialUniqueName = L"none";
	std::wstring MeshUniqueName = L"none";
	float Location[3] = { 0.0f, 0.0f, 0.0f };
	float Rotation[3] = { 0.0f, 0.0f, 0.0f };
	float Scale[3] = { 1.0f, 1.0f, 1.0f };

private:

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(UniqueName);
		//ar & BOOST_SERIALIZATION_NVP(MaterialUniqueName);
		ar & BOOST_SERIALIZATION_NVP(MeshUniqueName);
		ar & BOOST_SERIALIZATION_NVP(Location);
		ar & BOOST_SERIALIZATION_NVP(Rotation);
		ar & BOOST_SERIALIZATION_NVP(Scale);
	}
};

struct GProjectMeshInfo
{
	std::wstring MeshUniqueName = L"none";
	std::map<std::wstring, std::wstring> MaterialUniqueName;

private:

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(MeshUniqueName);
		ar & BOOST_SERIALIZATION_NVP(MaterialUniqueName);
	}
};

class GProject
{

public:

	GProject();
	~GProject();

	std::wstring mSkyCubemapUniqueName;
	std::list<GProjectTextureInfo> mTextureInfo;
	std::list<GProjectSceneObjectInfo> mSceneObjectInfo;
	std::list<GProjectMeshInfo> mMeshInfo;

	void SaveProject(
		std::wstring filename,
		std::wstring skyCubemapUniqueName,
		std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& pTextures,
		std::vector<GRiSceneObject*>& pSceneObjects,
		std::unordered_map<std::wstring, std::unique_ptr<GRiMesh>>& pMeshes
	)
	{
		mSkyCubemapUniqueName = skyCubemapUniqueName;

		mTextureInfo.clear();

		for (auto it = pTextures.begin(); it != pTextures.end(); it++)
		{
			GProjectTextureInfo tInfo;
			tInfo.UniqueFileName = (*it).second->UniqueFileName;
			tInfo.bSrgb = (*it).second->bSrgb;
			mTextureInfo.push_back(tInfo);
		}

		mSceneObjectInfo.clear();

		for (auto i = 0u; i < pSceneObjects.size(); i++)
		{
			GProjectSceneObjectInfo soInfo;
			soInfo.UniqueName = pSceneObjects[i]->UniqueName;
			soInfo.MeshUniqueName = pSceneObjects[i]->GetMesh()->UniqueName;
			//soInfo.MaterialUniqueName = pSceneObjects[i]->GetMaterial()->UniqueName;
			std::vector<float> loc = pSceneObjects[i]->GetLocation();
			soInfo.Location[0] = loc[0];
			soInfo.Location[1] = loc[1];
			soInfo.Location[2] = loc[2];
			std::vector<float> rot = pSceneObjects[i]->GetRotation();
			soInfo.Rotation[0] = rot[0];
			soInfo.Rotation[1] = rot[1];
			soInfo.Rotation[2] = rot[2];
			std::vector<float> scale = pSceneObjects[i]->GetScale();
			soInfo.Scale[0] = scale[0];
			soInfo.Scale[1] = scale[1];
			soInfo.Scale[2] = scale[2];
			mSceneObjectInfo.push_back(soInfo);
		}

		mMeshInfo.clear();

		for (auto it = pMeshes.begin(); it != pMeshes.end(); it++)
		{
			GProjectMeshInfo mInfo;
			mInfo.MeshUniqueName = (*it).second->UniqueName;
			for (auto submesh : (*it).second->Submeshes)
			{
				mInfo.MaterialUniqueName[submesh.first] = submesh.second.mMaterial->UniqueName;
			}
			mMeshInfo.push_back(mInfo);
		}

		std::ofstream ofs;
		//ofs.open(filename, std::ios_base::out | std::ios_base::binary);
		ofs.open(filename);
		if (ofs.good()) 
		{
			{
				//boost::archive::binary_woarchive oa(ofs);
				boost::archive::xml_oarchive oa(ofs);
				//boost::archive::text_oarchive oa(ofs);

				oa << boost::serialization::make_nvp("Project", *this);
			}     
			ofs.close(); 
		}

	}

	void LoadProject(std::wstring filename)
	{
		std::ifstream ifs;
		ifs.open(filename);
		if (ifs.good()) {
			{
				try
				{
					//boost::archive::binary_wiarchive ia(ifs);
					boost::archive::xml_iarchive ia(ifs);
					//boost::archive::text_iarchive ia(ifs);
					try
					{
						ia >> boost::serialization::make_nvp("Project", *this);
					}
					catch (boost::archive::archive_exception const& e)
					{
						e.what();
						MessageBox(nullptr, L"Fail to load project file.", L"Other Exception", MB_OK);
						ifs.close();
						return;
					}
				}
				catch (std::exception& e)
				{
					e.what();
					MessageBox(nullptr, L"Fail to create archive. Project file may be empty or out of date.", L"Other Exception", MB_OK);
					ifs.close();
					return;
				}
			} 
			ifs.close(); 
		}
	}

private:

	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(mSkyCubemapUniqueName);
		ar & BOOST_SERIALIZATION_NVP(mTextureInfo);
		ar & BOOST_SERIALIZATION_NVP(mSceneObjectInfo);
		ar & BOOST_SERIALIZATION_NVP(mMeshInfo);
	}

};

