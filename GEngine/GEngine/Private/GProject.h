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
		//ar & UniqueFileName;
		//ar & bSrgb;
	}
};

class GProject
{

public:

	GProject();
	~GProject();

	std::list<GProjectTextureInfo> mTextureInfo;

	void SaveProject(std::wstring filename, std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>& pTextures)
	{
		mTextureInfo.clear();

		std::unordered_map<std::wstring, std::unique_ptr<GRiTexture>>::iterator it;
		for (it = pTextures.begin(); it != pTextures.end(); it++)
		{
			GProjectTextureInfo tInfo;
			tInfo.UniqueFileName = (*it).second->UniqueFileName;
			tInfo.bSrgb = (*it).second->bSrgb;
			mTextureInfo.push_back(tInfo);
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
		ar & BOOST_SERIALIZATION_NVP(mTextureInfo);
	}

};

