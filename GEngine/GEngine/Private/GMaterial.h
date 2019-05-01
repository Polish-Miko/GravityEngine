#pragma once
#include "GRiInclude.h"




class GMaterial
{
	
public:

	GMaterial() = delete;
	~GMaterial() {};

	GRiMaterial* pMaterial;

	std::wstring UniqueName;
	std::wstring Name;
	float MaterialScale[2];
	std::list<std::wstring> TextureNames;
	std::list<float> ScalarParams;
	std::list<float> VectorParams;

	GMaterial(GRiMaterial* mat)
	{
		pMaterial = mat;
		LoadMaterialData();
	}

	void LoadMaterialData()
	{
		TextureNames.clear();
		ScalarParams.clear();
		VectorParams.clear();

		UniqueName = pMaterial->UniqueName;
		Name = pMaterial->Name;
		MaterialScale[0] = pMaterial->GetScaleX();
		MaterialScale[1] = pMaterial->GetScaleY();
		auto texNum = pMaterial->GetTextureNum();
		auto scalarNum = pMaterial->GetScalarNum();
		auto vectorNum = pMaterial->GetVectorNum();
		size_t i;
		for (i = 0; i < texNum; i++)
		{
			TextureNames.push_back(pMaterial->GetTextureUniqueNameByIndex(i));
		}
		for (i = 0; i < scalarNum; i++)
		{
			ScalarParams.push_back(pMaterial->GetScalar((int)i));
		}
		for (i = 0; i < vectorNum; i++)
		{
			GGiFloat4 vector = pMaterial->GetVector((int)i);
			VectorParams.push_back(vector.GetX());
			VectorParams.push_back(vector.GetY());
			VectorParams.push_back(vector.GetZ());
			VectorParams.push_back(vector.GetW());
		}
		/*
		for (auto tex : pMaterial->pTextures)
		{
			TextureNames.push_back(tex->UniqueFileName);
		}
		for (auto scalar : pMaterial->ScalarParams)
		{
			ScalarParams.push_back(scalar);
		}
		for (auto vector : pMaterial->VectorParams)
		{
			VectorParams.push_back(vector.GetX());
			VectorParams.push_back(vector.GetY());
			VectorParams.push_back(vector.GetZ());
			VectorParams.push_back(vector.GetW());
		}
		*/
	}

	void SaveMaterial(std::wstring workDir)
	{
		LoadMaterialData();

		std::ofstream ofs;
		std::wstring filename = workDir + UniqueName;
		ofs.open(filename);
		if (ofs.good())
		{
			{
				boost::archive::xml_oarchive oa(ofs);
				oa << boost::serialization::make_nvp("Material", *this);
			}
			ofs.close();
		}

	}

	void LoadMaterial(std::wstring workDir)
	{
		std::ifstream ifs;
		std::wstring filename = workDir + UniqueName;
		ifs.open(filename);
		if (ifs.good()) {
			{
				try
				{
					boost::archive::xml_iarchive ia(ifs);
					try
					{
						ia >> boost::serialization::make_nvp("Material", *this);
					}
					catch (boost::archive::archive_exception const& e)
					{
						e.what();
						MessageBox(nullptr, L"Fail to load material file.", L"Other Exception", MB_OK);
						ifs.close();
						return;
					}
				}
				catch (std::exception& e)
				{
					e.what();
					MessageBox(nullptr, L"Fail to create archive. material file may be empty or out of date.", L"Other Exception", MB_OK);
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
		ar & BOOST_SERIALIZATION_NVP(UniqueName);
		ar & BOOST_SERIALIZATION_NVP(Name);
		ar & BOOST_SERIALIZATION_NVP(MaterialScale);
		ar & BOOST_SERIALIZATION_NVP(TextureNames);
		ar & BOOST_SERIALIZATION_NVP(ScalarParams);
		ar & BOOST_SERIALIZATION_NVP(VectorParams);
	}

};

