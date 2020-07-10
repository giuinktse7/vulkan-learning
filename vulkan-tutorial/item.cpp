#include "item.h"

#include "items.h"

Item::Item(uint32_t itemTypeId)
{
	this->itemType = &Items::items.getItemType(itemTypeId);
}
