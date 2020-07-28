#include "item_attribute.h"

ItemAttributes::ItemAttributes() : attributes(nullptr)
{
  ////
}

ItemAttributes::ItemAttributes(const ItemAttributes &o)
{
  if (o.attributes)
    attributes = std::make_unique<std::map<std::string, ItemAttribute>>(*o.attributes.get());
}

ItemAttributes::~ItemAttributes()
{
  clearAllAttributes();
}

void ItemAttributes::createAttributes()
{
  if (!attributes)
    attributes = std::make_unique<std::map<std::string, ItemAttribute>>();
}

void ItemAttributes::clearAllAttributes()
{
  if (attributes)
    attributes = nullptr;
}

std::map<std::string, ItemAttribute> &ItemAttributes::getAttributes()
{
  if (!attributes)
  {
    createAttributes();
  }
  return *attributes;
}

void ItemAttributes::setAttribute(const std::string &key, const ItemAttribute &value)
{
  createAttributes();
  auto k = *attributes;
  k[key] = value;
  (*attributes)[key] = value;
}

void ItemAttributes::setAttribute(const std::string &key, const std::string &value)
{
  createAttributes();
  (*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string &key, int32_t value)
{
  createAttributes();
  (*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string &key, double value)
{
  createAttributes();
  (*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string &key, bool value)
{
  createAttributes();
  (*attributes)[key].set(value);
}

void ItemAttributes::eraseAttribute(const std::string &key)
{
  if (!attributes)
    return;

  auto iter = attributes->find(key);

  if (iter != attributes->end())
    attributes->erase(iter);
}

const std::string *ItemAttributes::getStringAttribute(const std::string &key) const
{
  if (!attributes)
    return nullptr;

  auto iter = attributes->find(key);
  if (iter != attributes->end())
    return iter->second.getString();
  return nullptr;
}

const int32_t *ItemAttributes::getIntegerAttribute(const std::string &key) const
{
  if (!attributes)
    return nullptr;

  auto iter = attributes->find(key);
  if (iter != attributes->end())
    return iter->second.getInteger();
  return nullptr;
}

const double *ItemAttributes::getFloatAttribute(const std::string &key) const
{
  if (!attributes)
    return nullptr;

  auto iter = attributes->find(key);
  if (iter != attributes->end())
    return iter->second.getFloat();
  return nullptr;
}

const bool *ItemAttributes::getBooleanAttribute(const std::string &key) const
{
  if (!attributes)
    return nullptr;

  auto iter = attributes->find(key);
  if (iter != attributes->end())
    return iter->second.getBoolean();
  return nullptr;
}

bool ItemAttributes::hasStringAttribute(const std::string &key) const
{
  return getStringAttribute(key) != nullptr;
}

bool ItemAttributes::hasIntegerAttribute(const std::string &key) const
{
  return getIntegerAttribute(key) != nullptr;
}

bool ItemAttributes::hasFloatAttribute(const std::string &key) const
{
  return getFloatAttribute(key) != nullptr;
}

bool ItemAttributes::hasBooleanAttribute(const std::string &key) const
{
  return getBooleanAttribute(key) != nullptr;
}

// Start of ItemAttribute

ItemAttribute::ItemAttribute() : type(ItemAttribute::NONE)
{
  // Empty
}

ItemAttribute::ItemAttribute(const std::string &str) : type(ItemAttribute::STRING)
{
  new (data) std::string(str);
}

ItemAttribute::ItemAttribute(int32_t i) : type(ItemAttribute::INTEGER)
{
  *reinterpret_cast<int *>(data) = i;
}

ItemAttribute::ItemAttribute(double f) : type(ItemAttribute::DOUBLE)
{
  *reinterpret_cast<double *>(data) = f;
}

ItemAttribute::ItemAttribute(bool b)
{
  *reinterpret_cast<bool *>(data) = b;
}

ItemAttribute::ItemAttribute(const ItemAttribute &o) : type(ItemAttribute::NONE)
{
  this->type = o.type;
  if (o.type == STRING)
  {
    this->set(o.getString());
  }
  else
  {
    memcpy(this->data, o.data, sizeof(o.data));
  }
}

ItemAttribute::~ItemAttribute()
{
  clear();
}

ItemAttribute &ItemAttribute::operator=(const ItemAttribute &o)
{
  if (&o == this)
    return *this;

  clear();
  type = o.type;
  if (type == STRING)
    new (data) std::string(*reinterpret_cast<const std::string *>(&o.data));
  else if (type == INTEGER)
    *reinterpret_cast<int32_t *>(data) = *reinterpret_cast<const int32_t *>(&o.data);
  else if (type == FLOAT)
    *reinterpret_cast<float *>(data) = *reinterpret_cast<const float *>(&o.data);
  else if (type == DOUBLE)
    *reinterpret_cast<double *>(data) = *reinterpret_cast<const double *>(&o.data);
  else if (type == BOOLEAN)
    *reinterpret_cast<bool *>(data) = *reinterpret_cast<const bool *>(&o.data);
  else
    type = NONE;

  return *this;
}

void ItemAttribute::set(const std::string &str)
{
  clear();
  type = STRING;
  new (data) std::string(str);
}

void ItemAttribute::set(int32_t i)
{
  clear();
  type = INTEGER;
  *reinterpret_cast<int32_t *>(&data) = i;
}

void ItemAttribute::set(double y)
{
  clear();
  type = DOUBLE;
  *reinterpret_cast<double *>(&data) = y;
}

void ItemAttribute::set(bool b)
{
  clear();
  type = BOOLEAN;
  *reinterpret_cast<bool *>(&data) = b;
}
#include <memory>
const std::string *ItemAttribute::getString() const
{
  if (type == STRING)
  {
    return reinterpret_cast<const std::string *>(&data);
  }
  return nullptr;
}

const int32_t *ItemAttribute::getInteger() const
{
  if (type == INTEGER)
    return reinterpret_cast<const int32_t *>(&data);
  return nullptr;
}

const double *ItemAttribute::getFloat() const
{
  if (type == DOUBLE)
    return reinterpret_cast<const double *>(&data);
  return nullptr;
}

const bool *ItemAttribute::getBoolean() const
{
  if (type == BOOLEAN)
    return reinterpret_cast<const bool *>(&data);
  return nullptr;
}

void ItemAttribute::clear()
{
  if (type == STRING)
  {
    (reinterpret_cast<std::string *>(&data))->~basic_string();
    type = NONE;
  }
}
