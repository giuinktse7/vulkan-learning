#include "items.h"

#include <iostream>
#include <utility>
#include <memory>

#include "otb.h"
#include "util.h"

#include "graphics/appearances.h"

using tl::unexpected;

Items Items::items;

constexpr uint32_t RESERVED_ITEM_COUNT = 30000;

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

using std::string;

constexpr auto OTBI = OTB::Identifier{{'O', 'T', 'B', 'I'}};

Items::Items(std::vector<ItemType> &items, std::unordered_map<ClientID, ServerID> clientIdToServerIdMap, OTB::VersionInfo otbVersionInfo)
		: itemTypes(std::move(items)), clientIdToServerIdMap(clientIdToServerIdMap), otbVersionInfo(otbVersionInfo)
{
}

ItemType::~ItemType()
{
	// Empty
}

ItemType &Items::getItemType(size_t id)
{
	return itemTypes.at(id);
}

tl::expected<void, std::string> unexpecting(const std::string s)
{
	return tl::make_unexpected(s);
}

tl::expected<void, std::string> Items::loadFromXml(const std::filesystem::path path)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());

	if (!result)
	{
		std::string pugiError = result.description();
		return unexpecting("Could not load items.xml (Syntax error?): " + pugiError);
	}

	pugi::xml_node node = doc.child("items");
	if (!node)
	{
		return unexpecting("items.xml, invalid root node.");
	}

	for (pugi::xml_node itemNode = node.first_child(); itemNode; itemNode = itemNode.next_sibling())
	{
		if (as_lower_str(itemNode.name()) != "item")
		{
			continue;
		}

		int32_t fromId = itemNode.attribute("fromid").as_int();
		int32_t toId = itemNode.attribute("toid").as_int();
		if (pugi::xml_attribute attribute = itemNode.attribute("id"))
		{
			fromId = toId = attribute.as_int();
		}

		if (fromId == 0 || toId == 0)
		{
			return unexpecting("Could not read item id from item node.");
		}

		for (size_t id = fromId; id <= toId; ++id)
		{
			auto result = loadItemFromXml(itemNode, id);
			if (!result.has_value())
			{
				std::cout << result.error() << std::endl;
			}
		}
	}

	std::cout << "Loaded items.xml." << std::endl;
	return {};
}

tl::expected<void, std::string> Items::loadItemFromXml(pugi::xml_node itemNode, uint32_t id)
{
	// TODO Fix versioning
	// TODO Why do we skip these in these cases?
	uint32_t clientVersion = 52;
	if (clientVersion < to_underlying(OTB::ClientVersion::CLIENT_VERSION_980) && id > 20000 && id < 20100)
	{
		itemNode = itemNode.next_sibling();
		return {};
	}
	else if (id > 30000 && id < 30100)
	{
		itemNode = itemNode.next_sibling();
		return {};
	}

	if (!Items::items.hasItemType(id))
	{
		// Fluids in current version, might change in the future
		if (id >= 40001 && id <= 40043)
		{
			return {};
		}
		return unexpecting("There is no itemType with server ID " + std::to_string(id));
	}

	ItemType &it = Items::items.getItemType(id);

	it.name = itemNode.attribute("name").as_string();
	it.editorsuffix = itemNode.attribute("editorsuffix").as_string();

	pugi::xml_attribute attribute;
	for (pugi::xml_node itemAttributesNode = itemNode.first_child(); itemAttributesNode; itemAttributesNode = itemAttributesNode.next_sibling())
	{
		if (!(attribute = itemAttributesNode.attribute("key")))
		{
			continue;
		}

		std::string key = attribute.as_string();
		to_lower_str(key);
		if (key == "type")
		{
			if (!(attribute = itemAttributesNode.attribute("value")))
			{
				continue;
			}

			std::string typeValue = attribute.as_string();
			to_lower_str(key);
			if (typeValue == "depot")
			{
				it.type = ITEM_TYPE_DEPOT;
			}
			else if (typeValue == "mailbox")
			{
				it.type = ITEM_TYPE_MAILBOX;
			}
			else if (typeValue == "trashholder")
			{
				it.type = ITEM_TYPE_TRASHHOLDER;
			}
			else if (typeValue == "container")
			{
				it.type = ITEM_TYPE_CONTAINER;
			}
			else if (typeValue == "door")
			{
				it.type = ITEM_TYPE_DOOR;
			}
			else if (typeValue == "magicfield")
			{
				it.group = ITEM_GROUP_MAGICFIELD;
				it.type = ITEM_TYPE_MAGICFIELD;
			}
			else if (typeValue == "teleport")
			{
				it.type = ITEM_TYPE_TELEPORT;
			}
			else if (typeValue == "bed")
			{
				it.type = ITEM_TYPE_BED;
			}
			else if (typeValue == "key")
			{
				it.type = ITEM_TYPE_KEY;
			}
		}
		else if (key == "name")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.name = attribute.as_string();
			}
		}
		else if (key == "description")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.description = attribute.as_string();
			}
		}
		else if (key == "runespellName")
		{
			/*if((attribute = itemAttributesNode.attribute("value"))) {
				it.runeSpellName = attribute.as_string();
			}*/
		}
		else if (key == "weight")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.weight = attribute.as_int() / 100.f;
			}
		}
		else if (key == "armor")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.armor = attribute.as_int();
			}
		}
		else if (key == "defense")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.defense = attribute.as_int();
			}
		}
		else if (key == "rotateto")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.rotateTo = attribute.as_int();
			}
		}
		else if (key == "containersize")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.volume = attribute.as_int();
			}
		}
		else if (key == "readable")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.canReadText = attribute.as_bool();
			}
		}
		else if (key == "writeable")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.canWriteText = it.canReadText = attribute.as_bool();
			}
		}
		else if (key == "decayto")
		{
			it.decays = true;
		}
		else if (key == "maxtextlen" || key == "maxtextlength")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.maxTextLen = attribute.as_int();
				it.canReadText = it.maxTextLen > 0;
			}
		}
		else if (key == "writeonceitemid")
		{
			/*if((attribute = itemAttributesNode.attribute("value"))) {
				it.writeOnceItemId = pugi::cast<int32_t>(attribute.value());
			}*/
		}
		else if (key == "allowdistread")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.allowDistRead = attribute.as_bool();
			}
		}
		else if (key == "charges")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.charges = attribute.as_int();
			}
		}
		else if (key == "floorchange")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				std::string value = attribute.as_string();
				if (value == "down")
				{
					it.floorChange = FloorChange::Down;
				}
				else if (value == "north")
				{
					it.floorChange = FloorChange::North;
				}
				else if (value == "south")
				{
					it.floorChange = FloorChange::South;
				}
				else if (value == "west")
				{
					it.floorChange = FloorChange::West;
				}
				else if (value == "east")
				{
					it.floorChange = FloorChange::East;
				}
				else if (value == "northex")
					it.floorChange = FloorChange::NorthEx;
				else if (value == "southex")
					it.floorChange = FloorChange::SouthEx;
				else if (value == "westex")
					it.floorChange = FloorChange::WestEx;
				else if (value == "eastex")
					it.floorChange = FloorChange::EastEx;
				else if (value == "southalt")
					it.floorChange = FloorChange::SouthAlt;
				else if (value == "eastalt")
					it.floorChange = FloorChange::EastAlt;
			}
		}
	}
	return {};
}

tl::expected<void, std::string> Items::loadFromOtb(const std::string &file)
{
	if (!Appearances::isLoaded)
	{
		throw std::runtime_error("Appearances must be loaded before loading items.otb.");
	}

	// OTB::Loader loader{file, OTBI};
	std::unique_ptr<OTB::Loader> loader = std::make_unique<OTB::Loader>(file, OTBI);

	auto &root = loader->parseTree();

	PropStream props;
	OTB::VersionInfo versionInfo;
	std::unordered_map<ClientID, ServerID> clientIdToServerIdMap;
	std::unordered_multimap<std::string, ServerID> nameToItems;

	std::vector<ItemType> itemTypes;

	itemTypes.reserve(RESERVED_ITEM_COUNT);
	clientIdToServerIdMap.reserve(RESERVED_ITEM_COUNT);
	nameToItems.reserve(RESERVED_ITEM_COUNT);

	if (loader->getProps(root, props))
	{
		//4 byte flags
		//attributes
		//0x01 = version data
		uint32_t flags;
		if (!props.read<uint32_t>(flags))
		{
			return unexpecting("Could not read the OTB header flags.");
		}

		uint8_t attr;
		if (!props.read<uint8_t>(attr))
		{
			return unexpecting("Could not read the OTB header attr.");
		}

		if (attr == to_underlying(OTB::RootAttributes::Version))
		{
			uint16_t length;
			if (!props.read<uint16_t>(length))
			{
				return unexpecting("Could not read the OTB version length.");
			}

			if (length != sizeof(OTB::VersionInfo))
			{
				return unexpecting("The length of the version info is incorrect.");
			}

			if (!props.read(versionInfo))
			{
				return unexpecting("Could not read OTB version information.");
			}
		}
	}

	if (versionInfo.majorVersion == 0xFFFFFFFF)
	{
		std::cout << "[Warning - Items::loadFromOtb] items.otb using generic client version." << std::endl;
	}
	else if (versionInfo.majorVersion != 3)
	{
		std::cout << "Old version detected, a newer version of items.otb is required." << std::endl;
		return unexpecting("Old version detected, a newer version of items.otb is required.");
	}
	else if (versionInfo.minorVersion < to_underlying(OTB::ClientVersion::CLIENT_VERSION_1098))
	{
		std::cout << "A newer version of items.otb is required." << std::endl;
		return unexpecting("A newer version of items.otb is required.");
	}

	for (auto &itemNode : root.children)
	{
		PropStream stream;
		if (!loader->getProps(itemNode, stream))
		{
			return unexpecting("Could not get props for a node.");
		}

		uint32_t flags;
		if (!stream.read<uint32_t>(flags))
		{
			return unexpecting("Could not read flags for a node.");
		}

		uint16_t serverId = 0;
		uint16_t clientId = 0;
		uint16_t speed = 0;
		uint16_t wareId = 0;
		uint8_t lightLevel = 0;
		uint8_t lightColor = 0;
		uint8_t alwaysOnTopOrder = 0;

		uint8_t attrib;

		const auto DEFAULT_ERROR = unexpecting("Bad format.");

		while (stream.read<uint8_t>(attrib))
		{
			uint16_t length;
			if (!stream.read<uint16_t>(length))
			{
				return unexpecting("Could not read attribute length.");
			}

			switch (attrib)
			{
			case itemattribute_t::ITEM_ATTR_SERVERID:
			{
				if (length != sizeof(uint16_t))
				{
					return unexpecting("Invalid attribute length.");
				}

				if (!stream.read<uint16_t>(serverId))
				{
					return unexpecting("Could not read server ID.");
				}

				if (serverId > 30000 && serverId < 30100)
				{
					serverId -= 30000;
				}
				break;
			}

			case itemattribute_t::ITEM_ATTR_CLIENTID:
			{
				if (length != sizeof(uint16_t))
				{
					return DEFAULT_ERROR;
				}

				if (!stream.read<uint16_t>(clientId))
				{
					return DEFAULT_ERROR;
				}
				break;
			}

			case itemattribute_t::ITEM_ATTR_SPEED:
			{
				if (length != sizeof(uint16_t))
				{
					return DEFAULT_ERROR;
				}

				if (!stream.read<uint16_t>(speed))
				{
					return DEFAULT_ERROR;
				}
				break;
			}

			case itemattribute_t::ITEM_ATTR_LIGHT2:
			{
				OTB::LightInfo lightInfo;
				if (length != sizeof(OTB::LightInfo))
				{
					return DEFAULT_ERROR;
				}

				if (!stream.read(lightInfo))
				{
					return DEFAULT_ERROR;
				}

				lightLevel = static_cast<uint8_t>(lightInfo.level);
				lightColor = static_cast<uint8_t>(lightInfo.color);
				break;
			}

			case itemattribute_t::ITEM_ATTR_TOPORDER:
			{
				if (length != sizeof(uint8_t))
				{
					return unexpecting("Bad length.");
				}

				if (!stream.read<uint8_t>(alwaysOnTopOrder))
				{
					return DEFAULT_ERROR;
				}
				break;
			}

			case itemattribute_t::ITEM_ATTR_WAREID:
			{
				if (length != sizeof(uint16_t))
				{
					return DEFAULT_ERROR;
				}

				if (!stream.read<uint16_t>(wareId))
				{
					return DEFAULT_ERROR;
				}
				break;
			}

			default:
			{
				//skip unknown attributes
				if (!stream.skip(length))
				{
					return DEFAULT_ERROR;
				}
				break;
			}
			}
		}

		clientIdToServerIdMap.emplace(clientId, serverId);

		// store the found item
		if (serverId >= itemTypes.size())
		{
			itemTypes.resize(serverId + 1);
		}
		ItemType &iType = itemTypes[serverId];

		iType.group = static_cast<itemgroup_t>(itemNode.type);
		switch (itemNode.type)
		{
		case itemgroup_t::ITEM_GROUP_CONTAINER:
			iType.type = ITEM_TYPE_CONTAINER;
			break;
		case ITEM_GROUP_DOOR:
			//not used
			iType.type = ITEM_TYPE_DOOR;
			break;
		case itemgroup_t::ITEM_GROUP_MAGICFIELD:
			//not used
			iType.type = ItemTypes_t::ITEM_TYPE_MAGICFIELD;
			break;
		case itemgroup_t::ITEM_GROUP_TELEPORT:
			//not used
			iType.type = ItemTypes_t::ITEM_TYPE_TELEPORT;
			break;
		case itemgroup_t::ITEM_GROUP_NONE:
		case itemgroup_t::ITEM_GROUP_GROUND:
		case itemgroup_t::ITEM_GROUP_SPLASH:
		case itemgroup_t::ITEM_GROUP_FLUID:
		case itemgroup_t::ITEM_GROUP_CHARGES:
		case itemgroup_t::ITEM_GROUP_DEPRECATED:
			break;
		default:
			return DEFAULT_ERROR;
		}

		iType.blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType.blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType.blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType.hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType.useable = hasBitSet(FLAG_USEABLE, flags);
		iType.pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType.moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType.stackable = hasBitSet(FLAG_STACKABLE, flags);

		iType.alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType.isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType.isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType.isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType.allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType.rotatable = hasBitSet(FLAG_ROTATABLE, flags);
		iType.canReadText = hasBitSet(FLAG_READABLE, flags);
		iType.lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);
		iType.isAnimation = hasBitSet(FLAG_ANIMATION, flags);
		// iType.walkStack = !hasBitSet(FLAG_FULLTILE, flags);
		iType.forceUse = hasBitSet(FLAG_FORCEUSE, flags);

		iType.id = serverId;
		iType.clientId = clientId;
		iType.speed = speed;
		iType.lightLevel = lightLevel;
		iType.lightColor = lightColor;
		iType.wareId = wareId;
		iType.alwaysOnTopOrder = alwaysOnTopOrder;

		if (Appearances::contains(clientId))
		{
			iType.name = Appearances::getById(clientId).name();
			iType.appearance = &Appearances::getById(clientId);
		}
	}

	itemTypes.shrink_to_fit();

	Items::items = Items(itemTypes, clientIdToServerIdMap, versionInfo);

	return {};
}