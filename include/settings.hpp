#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <string>

namespace settings {
void Save();
void Load();

const std::string ScreenshotsPath();
const std::string TagsPath();
const bool ShowConsole();

const int GetExtraStereoOffset();
void SetExtraStereoOffset(int offset);
}  // namespace settings

#endif  // SETTINGS_HPP_
