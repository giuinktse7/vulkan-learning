#include "appearances.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>

#include "texture_atlas.h"
#include "../items.h"
#include "engine.h"

#include <set>

using namespace std;
using namespace tibia::protobuf;

std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::objects;
std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::outfits;

std::set<uint32_t> Appearances::catalogIndex;
std::unordered_map<uint32_t, CatalogInfo> Appearances::catalogInfo;

bool Appearances::isLoaded;

void Appearances::loadFromFile(const std::filesystem::path path)
{
    auto start = std::chrono::high_resolution_clock::now();

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    appearances::Appearances parsed;

    {
        std::fstream input(path, std::ios::in | std::ios::binary);
        std::cout << path << std::endl;
        if (!parsed.ParseFromIstream(&input))
        {
            auto absolutePath = filesystem::absolute(filesystem::path(path));
            std::cerr << "Failed to parse appearances file at " << absolutePath << "." << endl;
        }

        google::protobuf::ShutdownProtobufLibrary();
    }

    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const appearances::Appearance &object = parsed.object(i);
        Appearances::objects[object.id()] = object;
    }

    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        const appearances::Appearance &outfit = parsed.outfit(i);
        Appearances::outfits[outfit.id()] = outfit;
    }

    Appearances::isLoaded = true;

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    std::cout << "Loaded appearances.dat in " << duration << " milliseconds." << std::endl;
}

void Appearances::addSpriteSheetInfo(CatalogInfo &info)
{
    Appearances::catalogIndex.insert(info.lastSpriteId);
    Appearances::catalogInfo[info.lastSpriteId] = info;
}

CatalogInfo Appearances::getCatalogInfo(uint32_t spriteId)
{
    auto found = lower_bound(catalogIndex.begin(), catalogIndex.end(), spriteId);
    if (found == catalogIndex.end())
    {
        cout << "Could not find a sprite sheet for sprite ID " << spriteId << "." << endl;
        exit(1);
    }

    uint32_t lastSpriteId = *found;

    return catalogInfo.at(lastSpriteId);
}

void Appearances::loadCatalog(const std::filesystem::path path)
{
    if (!filesystem::exists(path))
    {
        cout << "Could not locate the catalog JSON file. Failed to find file at path: " + filesystem::absolute(path).u8string() << endl;
        exit(1);
    }

    std::ifstream fileStream(path);
    auto json = make_unique<nlohmann::json>();
    fileStream >> (*json);

    std::filesystem::path basepath("C:/Users/giuin/AppData/Local/Tibia11/packages/Tibia/assets");

    std::set<std::string> types;

    for (const auto &x : *json)
    {
        if (x.at("type") == "sprite")
        {
            std::string file = x.at("file");
            std::filesystem::path filePath(file);

            CatalogInfo info{};
            info.area = x.at("area");
            info.file = basepath / filePath;
            info.spriteType = x.at("spritetype");
            info.firstSpriteId = x.at("firstspriteid");
            info.lastSpriteId = x.at("lastspriteid");
            addSpriteSheetInfo(info);
        }
        else
        {
            std::string s = x.at("type");
            types.insert(s);
        }
    }

    std::cout << "Types: " << std::endl;
    for (const auto &t : types)
    {
        std::cout << t << std::endl;
    }
}