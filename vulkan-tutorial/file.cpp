#include "file.h"
#include <stdexcept>
#include <fstream>

std::vector<uint8_t> File::read(const std::string &filepath)
{
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

	if (!ifs)
		throw std::runtime_error("Could not find file: " + filepath);

	auto end = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	auto size = std::size_t(end - ifs.tellg());

	if (size == 0) // avoid undefined behavior
		return {};

	std::vector<uint8_t> buffer(size);
	uint32_t offset = 0;

	if (!ifs.read((char *)buffer.data(), buffer.size()))
		throw std::runtime_error("Could not read file: " + filepath);

	return buffer;
}
