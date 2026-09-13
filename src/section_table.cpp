#include "section_table.hpp"

Section &SectionTable::add(const std::string &name, uint32_t symbolIndex)
{
  Section newSection;
  newSection.name = name;
  newSection.symbolIndex = symbolIndex;
  sections.push_back(newSection);
  uint32_t index = sections.size() - 1;
  sectionByName[name] = index;
  return sections[index];
}

Section *SectionTable::find(const std::string &name)
{
  auto it = sectionByName.find(name);
  if(it==sectionByName.end()){
    return nullptr;
  }
  return &sections[it->second];
}

Section &SectionTable::at(uint32_t index)
{
  return sections[index];
}

uint32_t SectionTable::getIndex(const std::string &name)
{
  return sectionByName.at(name);
}

std::vector<Section> &SectionTable::getSections()
{
  return sections;
}

size_t SectionTable::size() const
{
  return sections.size();
}
