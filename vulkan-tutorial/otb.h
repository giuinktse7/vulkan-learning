#pragma once

#include <exception>
#include <string>
#include <vector>
#include <array>

class PropStream;

namespace OTB
{
  enum class ClientVersion : uint32_t
  {
    CLIENT_VERSION_750 = 1,
    CLIENT_VERSION_755 = 2,
    CLIENT_VERSION_760 = 3,
    CLIENT_VERSION_770 = 3,
    CLIENT_VERSION_780 = 4,
    CLIENT_VERSION_790 = 5,
    CLIENT_VERSION_792 = 6,
    CLIENT_VERSION_800 = 7,
    CLIENT_VERSION_810 = 8,
    CLIENT_VERSION_811 = 9,
    CLIENT_VERSION_820 = 10,
    CLIENT_VERSION_830 = 11,
    CLIENT_VERSION_840 = 12,
    CLIENT_VERSION_841 = 13,
    CLIENT_VERSION_842 = 14,
    CLIENT_VERSION_850 = 15,
    CLIENT_VERSION_854_BAD = 16,
    CLIENT_VERSION_854 = 17,
    CLIENT_VERSION_855 = 18,
    CLIENT_VERSION_860_OLD = 19,
    CLIENT_VERSION_860 = 20,
    CLIENT_VERSION_861 = 21,
    CLIENT_VERSION_862 = 22,
    CLIENT_VERSION_870 = 23,
    CLIENT_VERSION_871 = 24,
    CLIENT_VERSION_872 = 25,
    CLIENT_VERSION_873 = 26,
    CLIENT_VERSION_900 = 27,
    CLIENT_VERSION_910 = 28,
    CLIENT_VERSION_920 = 29,
    CLIENT_VERSION_940 = 30,
    CLIENT_VERSION_944_V1 = 31,
    CLIENT_VERSION_944_V2 = 32,
    CLIENT_VERSION_944_V3 = 33,
    CLIENT_VERSION_944_V4 = 34,
    CLIENT_VERSION_946 = 35,
    CLIENT_VERSION_950 = 36,
    CLIENT_VERSION_952 = 37,
    CLIENT_VERSION_953 = 38,
    CLIENT_VERSION_954 = 39,
    CLIENT_VERSION_960 = 40,
    CLIENT_VERSION_961 = 41,
    CLIENT_VERSION_963 = 42,
    CLIENT_VERSION_970 = 43,
    CLIENT_VERSION_980 = 44,
    CLIENT_VERSION_981 = 45,
    CLIENT_VERSION_982 = 46,
    CLIENT_VERSION_983 = 47,
    CLIENT_VERSION_985 = 48,
    CLIENT_VERSION_986 = 49,
    CLIENT_VERSION_1010 = 50,
    CLIENT_VERSION_1020 = 51,
    CLIENT_VERSION_1021 = 52,
    CLIENT_VERSION_1030 = 53,
    CLIENT_VERSION_1031 = 54,
    CLIENT_VERSION_1035 = 55,
    CLIENT_VERSION_1076 = 56,
    CLIENT_VERSION_1098 = 57,
  };

  struct LightInfo
  {
    uint16_t level;
    uint16_t color;
  };

  enum class RootAttributes : uint8_t
  {
    Version = 0x01,
  };

  struct VersionInfo
  {
    uint32_t majorVersion = 0xFFFFFFFF;
    uint32_t minorVersion = 0x0;
    uint32_t buildNumber;
    uint8_t CSDVersion[128];
  };

  using Identifier = std::array<char, 4>;
  using ByteIterator = std::vector<uint8_t>::iterator;

  struct Node
  {

    std::vector<Node> children;
    ByteIterator propsBegin;
    ByteIterator propsEnd;
    uint8_t type;
    enum NodeChar : uint8_t
    {
      ESCAPE = 0xFD,
      START = 0xFE,
      END = 0xFF,
    };
  };

  struct LoadError : std::exception
  {
    const char *what() const noexcept override = 0;
  };

  struct InvalidOTBFormat final : LoadError
  {
    const char *what() const noexcept override
    {
      return "Invalid OTBM file format";
    }
  };

  class Loader
  {
  public:
    Loader(const std::string &fileName, const Identifier &acceptedIdentifier);
    bool getProps(const Node &node, PropStream &props);
    const Node &parseTree();

  private:
    std::vector<uint8_t> fileBuffer;
    Node root;
    std::vector<char> propBuffer;
  };
} // namespace OTB

class PropStream
{
public:
  void init(const char *a, size_t size)
  {
    cursor = a;
    end = a + size;
  }

  size_t size() const
  {
    return end - cursor;
  }

  template <typename T>
  bool read(T &ret)
  {
    if (size() < sizeof(T))
    {
      return false;
    }

    memcpy(&ret, cursor, sizeof(T));
    cursor += sizeof(T);
    return true;
  }

  bool readString(std::string &ret)
  {
    uint16_t strLen;
    if (!read<uint16_t>(strLen))
    {
      return false;
    }

    if (size() < strLen)
    {
      return false;
    }

    char *str = new char[strLen + 1];
    memcpy(str, cursor, strLen);
    str[strLen] = 0;
    ret.assign(str, strLen);
    delete[] str;
    cursor += strLen;
    return true;
  }

  bool skip(size_t n)
  {
    if (size() < n)
    {
      return false;
    }

    cursor += n;
    return true;
  }

private:
  const char *cursor = nullptr;
  const char *end = nullptr;
};

class PropWriteStream
{
public:
  PropWriteStream() = default;

  // non-copyable
  PropWriteStream(const PropWriteStream &) = delete;
  PropWriteStream &operator=(const PropWriteStream &) = delete;

  const char *getStream(size_t &size) const
  {
    size = buffer.size();
    return buffer.data();
  }

  void clear()
  {
    buffer.clear();
  }

  template <typename T>
  void write(T add)
  {
    char *addr = reinterpret_cast<char *>(&add);
    std::copy(addr, addr + sizeof(T), std::back_inserter(buffer));
  }

  void writeString(const std::string &str)
  {
    size_t strLength = str.size();
    if (strLength > std::numeric_limits<uint16_t>::max())
    {
      write<uint16_t>(0);
      return;
    }

    write(static_cast<uint16_t>(strLength));
    std::copy(str.begin(), str.end(), std::back_inserter(buffer));
  }

private:
  std::vector<char> buffer;
};