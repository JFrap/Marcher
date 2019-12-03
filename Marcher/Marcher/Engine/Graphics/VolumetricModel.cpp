#include "VolumetricModel.h"

namespace marcher {
	VolumetricModel::VolumetricModel(std::string path = "-") : m_handle(0) {
		if (path != "-")
			LoadModel(path);
	}

	void VolumetricModel::LoadModel(std::string path) {
		std::string inputfile = "Resources/Models/"+path;
		std::ifstream file(inputfile);

		std::vector<float> distanceField;
		float VOXELSIZE = 1;
		std::string line;
		if (file.is_open()) {
			file >> VOXELSIZE;
			while (std::getline(file, line)) {
				if (line != "") {					
					float lval = stof(line);
					distanceField.push_back(lval);
					//distanceField.push_back(lval);
				}
			}
			file.close();
		}
		else std::printf("Failed to open file %s!", inputfile.c_str());

		glGenTextures(1, &m_handle);
		glBindTexture(GL_TEXTURE_3D, m_handle);
		
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, 2 / VOXELSIZE, 2 / VOXELSIZE, 2 / VOXELSIZE, 0, GL_RED, GL_FLOAT, &distanceField[0]);
		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void VolumetricModel::Bind(int unit) {
		if (unit >= 0)
			glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_3D, m_handle);
	}

	VolumetricModel::~VolumetricModel() {
		glDeleteTextures(1, &m_handle);
	}
}