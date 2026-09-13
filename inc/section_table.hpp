#ifndef SECTION_TABLE_HPP
#define SECTION_TABLE_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <map>

struct Relocation{
  uint32_t offset = 0;
  uint32_t symbolIndex = 0;
  int32_t addend = 0;
};

struct Section{
  std::string name;
  uint32_t symbolIndex = 0;
  std::vector<uint8_t> data;
  std::vector<Relocation> relocations;

  uint32_t locationCounter() const{
    return data.size();
  }
};

class SectionTable{
public:
  Section& add(const std::string& name, uint32_t symbolIndex);
  Section* find(const std::string& name);
  Section& at(uint32_t index);

  uint32_t getIndex(const std::string& name);

  std::vector<Section>& getSections();
  size_t size() const;
private:
  std::vector<Section> sections;
  std::map<std::string, uint32_t> sectionByName;
};

#endif