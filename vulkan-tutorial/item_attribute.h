#pragma once

#include <map>
#include <string>
#include <memory>

class ItemAttribute
{
public:
  ItemAttribute();
  ItemAttribute(const std::string &str);
  ItemAttribute(int32_t i);
  ItemAttribute(double f);
  ItemAttribute(bool b);
  ItemAttribute(const ItemAttribute &o);
  ItemAttribute &operator=(const ItemAttribute &o);
  ~ItemAttribute();

  enum Type
  {
    STRING = 1,
    INTEGER = 2,
    FLOAT = 3,
    BOOLEAN = 4,
    DOUBLE = 5,
    NONE = 0
  } type;

  void clear();

  void set(const std::string &str);
  void set(int32_t i);
  void set(double f);
  void set(bool b);

  const std::string *getString() const;
  const int32_t *getInteger() const;
  const double *getFloat() const;
  const bool *getBoolean() const;

private:
  char data[sizeof(std::string) > sizeof(double) ? sizeof(std::string) : sizeof(double)];
};

class ItemAttributes
{
public:
  ItemAttributes();
  ItemAttributes(const ItemAttributes &i);
  virtual ~ItemAttributes();

  void setAttribute(const std::string &key, const ItemAttribute &attr);
  void setAttribute(const std::string &key, const std::string &value);
  void setAttribute(const std::string &key, int32_t value);
  void setAttribute(const std::string &key, double value);
  void setAttribute(const std::string &key, bool set);

  // returns nullptr if the attribute is not set
  const std::string *getStringAttribute(const std::string &key) const;
  const int32_t *getIntegerAttribute(const std::string &key) const;
  const double *getFloatAttribute(const std::string &key) const;
  const bool *getBooleanAttribute(const std::string &key) const;

  // Returns true if the attribute (of that type) exists
  bool hasStringAttribute(const std::string &key) const;
  bool hasIntegerAttribute(const std::string &key) const;
  bool hasFloatAttribute(const std::string &key) const;
  bool hasBooleanAttribute(const std::string &key) const;

  void eraseAttribute(const std::string &key);

  size_t size() const
  {
    if (!attributes)
      return 0;
    return attributes->size();
  }

  bool isEmpty() const
  {
      return size() == 0;
  }

  void clearAllAttributes();
  std::map<std::string, ItemAttribute> &getAttributes();

private:
  std::unique_ptr<std::map<std::string, ItemAttribute>> attributes;

  void createAttributes();
};
